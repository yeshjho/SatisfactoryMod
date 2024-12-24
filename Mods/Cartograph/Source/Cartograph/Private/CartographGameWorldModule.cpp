#include "CartographGameWorldModule.h"

#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/CanvasRenderTarget2D.h"

#include "FGLightweightBuildableSubsystem.h"
#include "FGBuildable.h"
#include "FGBuildableFoundation.h"
#include "FGBuildableSubsystem.h"
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


FVector2D world_position_to_screen_position(const FVector& WorldPosition, const FVector& Size)
{
	return FVector2D{
		// TODO: Width / 2 & Height / 2: Only verified for foundations
		ORIGIN_UV[0] + (WorldPosition.X - Size.X / 2) / MAP_WIDTH_CENTIMETERS,
		ORIGIN_UV[1] + (WorldPosition.Y - Size.Y / 2) / MAP_HEIGHT_CENTIMETERS
	} * RENDER_TEXTURE_SIZE;
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

			PendingAddBuildingData.Add(
				{
                    .Buildable = Buildable,
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

			PendingRemoveBuildingData.Add(
				{
                    .Buildable = Buildable,
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

    OnInitializationStarted.Broadcast();

	CurrentBuildingData.Empty(Factories.Num() + Buildings.Num());

	UKismetRenderingLibrary::ClearRenderTarget2D(this, RenderTarget, { 0, 0, 0, 0 });

	const float TimeBudget = FCartograph_ConfigStruct::GetActiveConfig(GetWorld()).InitializeTimeBudget;
	UE5Coro::Latent::FTickTimeBudget Budget = UE5Coro::Latent::FTickTimeBudget::Milliseconds(TimeBudget);

	for (auto* Object : Factories)
	{
		if (!Object->IsA<AFGBuildable>())
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

		const int32 Pos = Algo::LowerBound(CurrentBuildingData, NewBuildingData);
		CurrentBuildingData.Insert(std::move(NewBuildingData), Pos);

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

			const int32 Pos = Algo::LowerBound(CurrentBuildingData, NewBuildingData);
			CurrentBuildingData.Insert(std::move(NewBuildingData), Pos);

			co_await Budget;
		}
	}

	CARTO_LOG(TEXT("InitialBuildableGather Finished"));

    OnInitializationFinished.Broadcast();

	IsInitializing = false;
	IsPendingRedraw = false;
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

			const int32 Pos = Algo::LowerBound(CurrentBuildingData, AddedBuildingData);
			CurrentBuildingData.Insert(std::move(AddedBuildingData), Pos);

			co_await Budget;
		}

	    for (const FBuildingData& RemovedBuildingData : RemovedBuildings)
	    {
	        CARTO_LOG(TEXT("RemovedBuilding: %s"), *RemovedBuildingData.BuildableClass->GetName());

	        const int32 Start = Algo::LowerBound(CurrentBuildingData, RemovedBuildingData);
	        const int32 End = Algo::UpperBound(CurrentBuildingData, RemovedBuildingData);

	        for (int32 i = Start; i < End; ++i)
	        {
	            if (CurrentBuildingData[i] == RemovedBuildingData)
	            {
	                CurrentBuildingData.RemoveAt(i);
	                break;
	            }
	        }

	        co_await Budget;
	    }

        CARTO_LOG(TEXT("Buildings Change Processed"));
	}

	UKismetRenderingLibrary::ClearRenderTarget2D(this, RenderTarget, { 0, 0, 0, 0 });

	// Sometimes lines go crazy (goes to the top or far right) if we don't delay.
	// My guess is because EndDraw and BeginDraw are called in the same frame, so I'm putting it here.
	co_await UE5Coro::Latent::NextTick();

	UCanvas* Canvas = nullptr;
	FVector2D Size;
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, RenderTarget, Canvas, Size, RenderContext);

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

	for (const auto& [Buildable, BuildableClass, Transform, _] : CurrentBuildingData)
	{
		CARTO_LOG(TEXT("Buildable: %s, Transform: %s"), *BuildableClass->GetName(), *Transform.ToString());
		if (BuildableClass->ImplementsInterface(UFGSplineBuildableInterface::StaticClass()))
		{
            const FSplineData* SplineData = BuildableSplineDataMap.Find(BuildableClass.Get());
			if (!SplineData || !IsValid(Buildable))
			{
				CARTO_LOG(TEXT("Buildable is invalid"));
				continue;
			}

            const auto* SplineBuildable = Cast<IFGSplineBuildableInterface>(Buildable);
			USplineComponent* SplineComponent = SplineBuildable->GetSplineComponent();

			const float SplineLength = SplineComponent->GetSplineLength();
			const int Segments = FMath::Max(2, FMath::CeilToInt(SplineLength / SplineData->SparsityCached));
			const float Step = SplineComponent->SplineCurves.ReparamTable.Points.Num() / static_cast<float>(Segments);
			for (int i = 0; i < Segments; ++i)
			{
				const FVector Start = SplineComponent->GetLocationAtSplineInputKey(i * Step, ESplineCoordinateSpace::World);
				const FVector End = SplineComponent->GetLocationAtSplineInputKey((i + 1) * Step, ESplineCoordinateSpace::World);
				const FVector2D StartScreenPosition = world_position_to_screen_position(Start, FVector::ZeroVector);
				const FVector2D EndScreenPosition = world_position_to_screen_position(End, FVector::ZeroVector);
				FCanvasLineItem LineItem{
					StartScreenPosition,
					EndScreenPosition
				};
				LineItem.LineThickness = SplineData->Thickness;
				LineItem.SetColor(SplineData->Color);
				// Only opaque lines are supported
				// LineItem.BlendMode = FCanvas::BlendToSimpleElementBlend
				Canvas->DrawItem(LineItem);

				co_await Budget;
			}

			continue;
		}

		const TSoftObjectPtr<UTexture2D>* Texture = BuildableToIconMap.Find(BuildableClass.Get());
		if (!Texture || Texture->IsNull())
		{
			continue;
		}

		FVector BuildableSize = Transform.GetScale3D();
		if (const FVector* GivenSize = BuildableSizeMap.Find(BuildableClass.Get()))
		{
            BuildableSize *= *GivenSize;
		}
		else if (BuildableClass->IsChildOf(AFGBuildableFoundation::StaticClass()))
		{
			const AFGBuildableFoundation* Foundation = Cast<AFGBuildableFoundation>(BuildableClass->ClassDefaultObject);
            BuildableSize *= { Foundation->mWidth, Foundation->mDepth, Foundation->mHeight };
		}
		else
		{
			continue;
		}

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
