#include "CartographGameWorldModule.h"

#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/CanvasRenderTarget2D.h"

#include "FGLightweightBuildableSubsystem.h"
#include "FGBuildable.h"
#include "FGBuildableFoundation.h"
#include "FGBuildableSubsystem.h"
#include "FGBuildableWire.h"
#include "FGSaveSession.h"
#include "FGSplineBuildableInterface.h"

#include "Patching/NativeHookManager.h"

#include "Cartograph_ConfigStruct.h"


constexpr int RENDER_TEXTURE_SIZE = 1024 * 8;

constexpr double WEST_BOUND_CENTIMETERS = -324'698.832031;
constexpr double EAST_BOUND_CENTIMETERS = 425'301.832031;
constexpr double NORTH_BOUND_CENTIMETERS = -375'000;
constexpr double SOUTH_BOUND_CENTIMETERS = 375'000;
constexpr double MAP_WIDTH_CENTIMETERS = EAST_BOUND_CENTIMETERS - WEST_BOUND_CENTIMETERS;
constexpr double MAP_HEIGHT_CENTIMETERS = SOUTH_BOUND_CENTIMETERS - NORTH_BOUND_CENTIMETERS;
constexpr double ORIGIN_UV[] = { -WEST_BOUND_CENTIMETERS / MAP_WIDTH_CENTIMETERS, -NORTH_BOUND_CENTIMETERS / MAP_HEIGHT_CENTIMETERS };
constexpr double PIXEL_PER_CENTIMETER[] = { RENDER_TEXTURE_SIZE / MAP_WIDTH_CENTIMETERS, RENDER_TEXTURE_SIZE / MAP_HEIGHT_CENTIMETERS };


DEFINE_LOG_CATEGORY(LogCartograph);


constexpr bool ENABLE_LOG = false;
#define CARTO_LOG(...) if constexpr (ENABLE_LOG) UE_LOG(LogCartograph, Display, __VA_ARGS__)


template<typename T>
    requires std::is_same_v<T, FVector> || std::is_same_v<T, FVector2D>
FVector2D world_position_to_screen_position(const T& WorldPosition, const T& Size)
{
	return FVector2D{
		// TODO: Width / 2 & Height / 2: Only verified for foundations
		ORIGIN_UV[0] + (WorldPosition.X - Size.X / 2) / MAP_WIDTH_CENTIMETERS,
		ORIGIN_UV[1] + (WorldPosition.Y - Size.Y / 2) / MAP_HEIGHT_CENTIMETERS
	} * RENDER_TEXTURE_SIZE;
}


void draw_line(UCanvas* Canvas, const FVector& WorldStart, const FVector& WorldEnd, const FLinearColor& Color, float Thickness)
{
    const FVector2D StartScreenPosition = world_position_to_screen_position(WorldStart, FVector::ZeroVector);
    const FVector2D EndScreenPosition = world_position_to_screen_position(WorldEnd, FVector::ZeroVector);
	FCanvasLineItem LineItem{
		StartScreenPosition,
		EndScreenPosition
	};
	LineItem.LineThickness = Thickness;
	LineItem.SetColor(Color);
	// Only opaque lines are supported
	// LineItem.BlendMode = FCanvas::BlendToSimpleElementBlend
	Canvas->DrawItem(LineItem);
}


void clear_render_target_portion(UCanvas* Canvas, const FBox2D& Region)
{
	const FVector2D Size = Region.GetSize();
	const FVector2D ScreenPosition = world_position_to_screen_position(Region.GetCenter(), Size);
	FCanvasTileItem TileItem{
		ScreenPosition,
		{ Size.X * PIXEL_PER_CENTIMETER[0], Size.Y * PIXEL_PER_CENTIMETER[1] },
		{ 0, 0, 0, 0 }
	};
	TileItem.PivotPoint = { 0.5, 0.5 };
	TileItem.BlendMode = SE_BLEND_Opaque;

	Canvas->DrawItem(TileItem);
}


auto FBuildingData::operator<=>(const FBuildingData& Other) const noexcept
{
	return Transform.GetLocation().Z <=> Other.Transform.GetLocation().Z;
}


bool FBuildingData::operator==(const FBuildingData& Other) const noexcept
{
    return BuildableClass == Other.BuildableClass && Transform.Equals(Other.Transform);
    // Ignoring CustomizationData on purpose
}


UCartographGameInstanceModule::UCartographGameInstanceModule()
	: CurrentBuildingQuadTree(FBox2D{ { WEST_BOUND_CENTIMETERS, NORTH_BOUND_CENTIMETERS }, { EAST_BOUND_CENTIMETERS, SOUTH_BOUND_CENTIMETERS } })
{}


void UCartographGameInstanceModule::DispatchLifecycleEvent(ELifecyclePhase Phase)
{
	Super::DispatchLifecycleEvent(Phase);

    if (Phase != ELifecyclePhase::POST_INITIALIZATION)
    {
        return;
    }


	const auto LambdaAfterLoadGame =
		[this](bool ReturnValue, UFGSaveSession* Instance, const FString& SaveName)
		{
			ShouldInitialize = true;

			// Calling it at the same tick sometimes causes the coroutine to just disappear
			GetWorld()->GetTimerManager().SetTimerForNextTick([this, Instance]()
				{
					ShouldInitialize = false;
					IsInitializing = true;

					Coroutine = InitialBuildableGather(
						AFGBuildableSubsystem::Get(Instance)->GetAllBuildablesRef(),
                        AFGLightweightBuildableSubsystem::Get(Instance)->mBuildableClassToInstanceArray
					);
				});
		};


    const auto LambdaAfterAddFromBuildableInstanceData = 
        [this](int32 ReturnValue, AFGLightweightBuildableSubsystem* Instance, TSubclassOf<AFGBuildable> BuildableClass, 
            FRuntimeBuildableInstanceData& BuildableInstanceData, bool FromSaveData = false, int32 SaveDataBuildableIndex = INDEX_NONE, 
            uint16 ConstructId = MAX_uint16, AActor* BuildEffectInstigator = nullptr, int32 BlueprintBuildEffectIndex = INDEX_NONE)
        {
			if (ShouldInitialize || FromSaveData)
			{
				return;
			}

			CARTO_LOG(TEXT("AddFromBuildableInstanceData: %s"), *BuildableClass->GetName());

			PendingAddBuildingData.Add(
				{
                    .Buildable = nullptr,
					.BuildableClass = BuildableClass,
					.Transform = BuildableInstanceData.Transform,
					.CustomizationData = BuildableInstanceData.CustomizationData,
				}
			);
        	RedrawMap();
        };


	const auto LambdaAfterAddFromReplicatedData =
		[this](AFGLightweightBuildableSubsystem* Instance, TSubclassOf<AFGBuildable> BuildableClass, TSubclassOf<UFGRecipe> BuiltWithRecipe, 
			const FLightweightBuildableReplicationItem& ReplicationData, int32 MaxSize, 
			AActor* BuildEffectInstigator, int32 BlueprintBuildIndex)
		{
			if (ShouldInitialize)
			{
				return;
			}

			CARTO_LOG(TEXT("AddFromReplicatedData: %s"), *BuildableClass->GetName());

			PendingAddBuildingData.Add(
				{
					.Buildable = nullptr,
					.BuildableClass = BuildableClass,
					.Transform = ReplicationData.Transform,
					.CustomizationData = ReplicationData.CustomizationData,
				}
			);
			RedrawMap();
		};


	const auto LambdaAfterAddBuildable =
		[this](AFGBuildableSubsystem* Instance, AFGBuildable* Buildable)
		{
			if (ShouldInitialize)
			{
				return;
			}

			CARTO_LOG(TEXT("AddBuildable: %s"), *Buildable->GetClass()->GetName());

			FVector Origin;
			FVector Extent;
			Buildable->GetActorBounds(false, Origin, Extent, true);
			PendingAddBuildingData.Add(
				{
                    .Buildable = Buildable,
					.BoundingBox = FBox2D{ FVector2D{ Origin - Extent }, FVector2D{ Origin + Extent } },
					.BuildableClass = Buildable->GetClass(),
					.Transform = Buildable->GetTransform(),
					.CustomizationData = Buildable->GetCustomizationData_Native(),
				}
			);
			RedrawMap();
		};


	const auto LambdaAfterInvalidateRuntimeInstanceDataForIndex =
		[this](AFGLightweightBuildableSubsystem* Instance, TSubclassOf<AFGBuildable> BuildableClass, int32 Index)
		{
			if (ShouldInitialize)
			{
				return;
			}

			CARTO_LOG(TEXT("InvalidateRuntimeInstanceDataForIndex: %s"), *BuildableClass->GetName());

			const FRuntimeBuildableInstanceData* Data = Instance->GetRuntimeDataForBuildableClassAndIndex(BuildableClass, Index);

            PendingRemoveBuildingData.Add(
                {
					.Buildable = nullptr,
                    .BuildableClass = BuildableClass,
                    .Transform = Data->Transform,
                    .CustomizationData = Data->CustomizationData,
                }
            );
            RedrawMap();
		};


	const auto LambdaAfterRemoveBuildable =
		[this](AFGBuildableSubsystem* Instance, AFGBuildable* Buildable)
		{
			if (ShouldInitialize)
			{
				return;
			}

			CARTO_LOG(TEXT("RemoveBuildable: %s"), *Buildable->GetClass()->GetName());

			FVector Origin;
            FVector Extent;
			Buildable->GetActorBounds(false, Origin, Extent, true);
			PendingRemoveBuildingData.Add(
				{
					.Buildable = Buildable,
					.BoundingBox = FBox2D{ FVector2D{ Origin - Extent }, FVector2D{ Origin + Extent } },
					.BuildableClass = Buildable->GetClass(),
					.Transform = Buildable->GetTransform(),
					.CustomizationData = Buildable->GetCustomizationData_Native(),
				}
			);
			RedrawMap();
		};


	SUBSCRIBE_UOBJECT_METHOD_AFTER(UFGSaveSession, LoadGame, LambdaAfterLoadGame);


	SUBSCRIBE_UOBJECT_METHOD_AFTER(AFGLightweightBuildableSubsystem, AddFromBuildableInstanceData, LambdaAfterAddFromBuildableInstanceData);
    SUBSCRIBE_UOBJECT_METHOD_AFTER(AFGLightweightBuildableSubsystem, AddFromReplicatedData, LambdaAfterAddFromReplicatedData);

	SUBSCRIBE_UOBJECT_METHOD_AFTER(AFGBuildableSubsystem, AddBuildable, LambdaAfterAddBuildable);


	SUBSCRIBE_UOBJECT_METHOD_AFTER(AFGLightweightBuildableSubsystem, InvalidateRuntimeInstanceDataForIndex, LambdaAfterInvalidateRuntimeInstanceDataForIndex);

	SUBSCRIBE_UOBJECT_METHOD_AFTER(AFGBuildableSubsystem, RemoveBuildable, LambdaAfterRemoveBuildable);
}


// Factories/Buildings: Intentional copies
UE5Coro::TCoroutine<> UCartographGameInstanceModule::InitialBuildableGather(
	TArray<AFGBuildable*> Factories, TMap<TSubclassOf<AFGBuildable>, TArray<FRuntimeBuildableInstanceData>> Buildings, FForceLatentCoroutine)
{
	CARTO_LOG(TEXT("InitialBuildableGather Started"));

	CurrentBuildingData.Empty(Factories.Num() + Buildings.Num());

	UKismetRenderingLibrary::ClearRenderTarget2D(this, RenderTarget, { 0, 0, 0, 0 });

	const float TimeBudget = FCartograph_ConfigStruct::GetActiveConfig(GetWorld()).InitializeTimeBudget;
	UE5Coro::Latent::FTickTimeBudget Budget = UE5Coro::Latent::FTickTimeBudget::Milliseconds(TimeBudget);

	for (AFGBuildable* Object : Factories)
	{
		if (!IsValid(Object) || !Object->IsA<AFGBuildable>())
		{
			continue;
		}
		
		auto* Buildable = Cast<AFGBuildable>(Object);

		FBuildingData NewBuildingData{
            .Buildable = Buildable,
			.BuildableClass = Buildable->GetClass(),
			.Transform = Buildable->GetTransform(),
            .CustomizationData = Buildable->GetCustomizationData_Native(),
		};

        AddToCurrentBuildingData(NewBuildingData);

		co_await Budget;
	}

	for (const auto& [Type, Arr] : Buildings)
	{
		for (const FRuntimeBuildableInstanceData& InstanceData : Arr)
		{
			FBuildingData NewBuildingData{
				.Buildable = nullptr,
				.BuildableClass = Type,
				.Transform = InstanceData.Transform,
				.CustomizationData = InstanceData.CustomizationData,
			};

			AddToCurrentBuildingData(NewBuildingData);

			co_await Budget;
		}
	}

	CARTO_LOG(TEXT("InitialBuildableGather Finished"));

	Coroutine = RedrawMapCoroutine(PendingAddBuildingData, PendingRemoveBuildingData);
	PendingAddBuildingData.Empty();
    PendingRemoveBuildingData.Empty();
}


void UCartographGameInstanceModule::RedrawMap()
{
	if (!Coroutine.IsDone())
	{
		if (!IsInitializing)
		{
			CARTO_LOG(TEXT("RedrawMapCoroutine Cancel Requested"));
			Coroutine.Cancel();
		}
		IsPendingRedraw = true;
	}
	else
	{
		Coroutine = RedrawMapCoroutine(PendingAddBuildingData, PendingRemoveBuildingData);
		PendingAddBuildingData.Empty();
		PendingRemoveBuildingData.Empty();
	}
}


// AddedBuildings/RemovedBuildings: Intentional copies
UE5Coro::TCoroutine<> UCartographGameInstanceModule::RedrawMapCoroutine(
	TArray<FBuildingData> AddedBuildings, TArray<FBuildingData> RemovedBuildings, FForceLatentCoroutine)
{
	ON_SCOPE_EXIT
	{
        OnCoroutineFinishedOrCancelled();
	};

	CARTO_LOG(TEXT("RedrawMapCoroutine Started"));

	const float TimeBudget = FCartograph_ConfigStruct::GetActiveConfig(GetWorld()).RedrawTimeBudget;
	UE5Coro::Latent::FTickTimeBudget Budget = UE5Coro::Latent::FTickTimeBudget::Milliseconds(TimeBudget);

	{
        UE5Coro::FCancellationGuard Guard{};  // We'll lose added/removed building information if the coroutine is cancelled

		for (FBuildingData& AddedBuildingData : AddedBuildings)
		{
	        CARTO_LOG(TEXT("AddedBuilding: %s"), *AddedBuildingData.BuildableClass->GetName());
            AddToCurrentBuildingData(AddedBuildingData);
			co_await Budget;
		}

	    for (const FBuildingData& RemovedBuildingData : RemovedBuildings)
	    {
	        CARTO_LOG(TEXT("RemovedBuilding: %s"), *RemovedBuildingData.BuildableClass->GetName());
            RemoveFromCurrentBuildingData(RemovedBuildingData);
	        co_await Budget;
	    }

        CARTO_LOG(TEXT("Buildings Change Processed"));
	}

	// Sometimes lines go crazy (goes to the top or far right) if we don't delay.
	// My guess is because EndDraw and BeginDraw are called in the same frame, so I'm putting it here.
	co_await UE5Coro::Latent::NextTick();

	UCanvas* Canvas = nullptr;
	FVector2D Size;
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, RenderTarget, Canvas, Size, RenderContext);

	clear_render_target_portion(Canvas, FBox2D{ { WEST_BOUND_CENTIMETERS, NORTH_BOUND_CENTIMETERS }, { EAST_BOUND_CENTIMETERS, SOUTH_BOUND_CENTIMETERS } });

	FCartograph_ConfigStruct ConfigInstance = FCartograph_ConfigStruct::GetActiveConfig(GetWorld());
    for (auto& [_, SplineData] : BuildableSplineDataMap)
    {
		FProperty* Property = FCartograph_ConfigStruct::StaticStruct()->FindPropertyByName(SplineData.SparsityConfigName);
        if (!Property)
        {
            UE_LOG(LogCartograph, Error, TEXT("SparsityConfigName not found: %s"), *SplineData.SparsityConfigName.ToString());
            continue;
        }
		SplineData.SparsityCached = *Property->ContainerPtrToValuePtr<int>(&ConfigInstance);
    }

	for (const auto& [Buildable, BoundingBox, BuildableClass, Transform, _] : CurrentBuildingData)
	{
		CARTO_LOG(TEXT("Buildable: %s, Transform: %s"), *BuildableClass->GetName(), *Transform.ToString());

		if (BuildableClass->ImplementsInterface(UFGSplineBuildableInterface::StaticClass()))
		{
            const FSplineData* SplineData = BuildableSplineDataMap.Find(BuildableClass.Get());
			if (!SplineData || !Buildable.IsValid())
			{
				continue;
			}

            const auto* SplineBuildable = Cast<IFGSplineBuildableInterface>(Buildable.Get());
			USplineComponent* SplineComponent = SplineBuildable->GetSplineComponent();

			const int SplinePointCount = SplineComponent->SplineCurves.ReparamTable.Points.Num();
			if (SplinePointCount < 2)
			{
				continue;
			}

			float PrevInVal = 0;
			for (int i = 1; i < SplinePointCount; i++)
			{
				const FInterpCurvePoint<float>& SplinePoint = SplineComponent->SplineCurves.ReparamTable.Points[i];
				const float InVal = SplinePoint.InVal;
				const float SegmentLength = InVal - PrevInVal;
                PrevInVal = InVal;

				const int Segments = FMath::CeilToInt(SegmentLength / SplineData->SparsityCached);
				const float Step = 1.f / Segments;

				for (int j = 0; j < Segments; j++)
				{
                    const float StartKey = (i - 1) + j * Step;
                    const float EndKey = (i - 1) + (j + 1) * Step;

					const FVector Start = SplineComponent->GetLocationAtSplineInputKey(StartKey, ESplineCoordinateSpace::World);
					const FVector End = SplineComponent->GetLocationAtSplineInputKey(EndKey, ESplineCoordinateSpace::World);
                    draw_line(Canvas, Start, End, SplineData->Color, SplineData->Thickness);

					// Not co_awaiting here because the Buildable and SplineComponent might become invalid after resuming.
                    // I could copy the relevant data, but let's hope it doesn't take too long.
				}
			}

			co_await Budget;
			continue;
		}

		if (BuildableClass->IsChildOf(AFGBuildableWire::StaticClass()))
        {
            const FWireData* WireData = BuildableWireDataMap.Find(BuildableClass.Get());
            if (!WireData || !Buildable.IsValid())
            {
                continue;
            }

            const auto* Wire = Cast<AFGBuildableWire>(Buildable);
            const FVector Start = Wire->GetConnectionLocation(0);
            const FVector End = Wire->GetConnectionLocation(1);
            draw_line(Canvas, Start, End, WireData->Color, WireData->Thickness);

            co_await Budget;
        }

		const TSoftObjectPtr<UTexture2D>* Texture = BuildableToIconMap.Find(BuildableClass.Get());
		if (!Texture || Texture->IsNull())
		{
			continue;
		}

		const TOptional<FVector> BuildableSizeOpt = GetBuildableSize(BuildableClass);
        if (!BuildableSizeOpt)
        {
            continue;
        }
		const FVector BuildableSize = BuildableSizeOpt.GetValue() * Transform.GetScale3D();

		const UTexture2D* LoadedTexture = Texture->Get();
		if (!LoadedTexture)
		{
			LoadedTexture = co_await UE5Coro::Latent::AsyncLoadObject(*Texture);
		}

		const FVector2D ScreenPosition = world_position_to_screen_position(Transform.GetLocation(), BuildableSize);
		FCanvasTileItem TileItem{
			ScreenPosition,
			LoadedTexture->GetResource(),
			{ BuildableSize.X * PIXEL_PER_CENTIMETER[0], BuildableSize.Y * PIXEL_PER_CENTIMETER[1] },
			{ 0, 0 },
			{ 1, 1 },
			FLinearColor::White
		};

		TileItem.Rotation = Transform.GetRotation().Rotator();
		TileItem.PivotPoint = { 0.5, 0.5 };
		TileItem.BlendMode = FCanvas::BlendToSimpleElementBlend(EBlendMode::BLEND_Translucent);

		if (const FRotator* Extra = BuildableExtraRotationMap.Find(BuildableClass.Get()))
        {
            TileItem.Rotation += *Extra;
        }

		Canvas->DrawItem(TileItem);

		co_await Budget;
	}

	IsInitializing = false;

	CARTO_LOG(TEXT("RedrawMapCoroutine Finished"));
}


void UCartographGameInstanceModule::OnCoroutineFinishedOrCancelled()
{
    CARTO_LOG(TEXT("OnCoroutineFinishedOrCancelled"));

    if (RenderContext.RenderTarget)
    {
        UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, RenderContext);
		RenderContext = {};
    }

	if (!IsPendingRedraw)
	{
		return;
	}

	// The coroutine is also on the game thread, so I think no data race here.
    IsPendingRedraw = false;
	Coroutine = RedrawMapCoroutine(PendingAddBuildingData, PendingRemoveBuildingData);
	PendingAddBuildingData.Empty();
	PendingRemoveBuildingData.Empty();
}


TOptional<FVector> UCartographGameInstanceModule::GetBuildableSize(TSubclassOf<AFGBuildable> BuildableClass) const
{
	if (const FVector* GivenSize = BuildableSizeMap.Find(BuildableClass.Get()))
	{
		return *GivenSize;
	}

	if (BuildableClass->IsChildOf(AFGBuildableFoundation::StaticClass()))
	{
		const AFGBuildableFoundation* Foundation = Cast<AFGBuildableFoundation>(BuildableClass->ClassDefaultObject);
		return FVector{ Foundation->mWidth, Foundation->mDepth, Foundation->mHeight };
	}

	return {};
}


TOptional<FBox2D> UCartographGameInstanceModule::GetBuildableBounds(const FBuildingData& BuildingData) const
{
	constexpr float BoundInflation = 1.5f;

	if (BuildingData.BoundingBox)
	{
        return *BuildingData.BoundingBox;
	}

	const TOptional<FVector> BuildableSize = GetBuildableSize(BuildingData.BuildableClass);
	if (!BuildableSize)
	{
		return {};
	}

	const float HalfWidth = BuildableSize->X / 2;
	const float HalfHeight = BuildableSize->Y / 2;
	FVector2D LocalCorners[] = {
		{ -HalfWidth, -HalfHeight },
		{ HalfWidth, -HalfHeight },
		{ HalfWidth,  HalfHeight },
		{ -HalfWidth,  HalfHeight },
	};
	for (FVector2D& Corner : LocalCorners)
	{
		Corner = FVector2D{ BuildingData.Transform.TransformPosition(FVector{ Corner, 0 }) };
	}

	return FBox2D{ LocalCorners, 4 }.ExpandBy(BoundInflation);
}


void UCartographGameInstanceModule::AddToCurrentBuildingData(FBuildingData& BuildingData)
{
	const int32 Pos = Algo::LowerBound(CurrentBuildingData, BuildingData);
	CurrentBuildingData.Insert(std::move(BuildingData), Pos);

	if (const TOptional<FBox2D> Bounds = GetBuildableBounds(BuildingData))
    {
		CurrentBuildingQuadTree.Insert(BuildingData, *Bounds);
    }
}


void UCartographGameInstanceModule::RemoveFromCurrentBuildingData(const FBuildingData& BuildingData)
{
	const int32 Start = Algo::LowerBound(CurrentBuildingData, BuildingData);
	const int32 End = Algo::UpperBound(CurrentBuildingData, BuildingData);

	for (int32 i = Start; i < End; ++i)
	{
		if (CurrentBuildingData[i] == BuildingData)
		{
			CurrentBuildingData.RemoveAt(i);
			break;
		}
	}

	if (const TOptional<FBox2D> Bounds = GetBuildableBounds(BuildingData))
	{
		CurrentBuildingQuadTree.Remove(BuildingData, *Bounds);
	}
}
