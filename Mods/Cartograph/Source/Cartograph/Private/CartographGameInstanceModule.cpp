#include "CartographGameInstanceModule.h"

#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/CanvasRenderTarget2D.h"

#include "FGLightweightBuildableSubsystem.h"
#include "FGBuildable.h"
#include "FGBuildableBeam.h"
#include "FGBuildableSubsystem.h"
#include "FGBuildableWire.h"
#include "FGBuildingDescriptor.h"
#include "FGBuildCategory.h"
#include "FGBuildSubCategory.h"
#include "FGPlayerController.h"
#include "FGRecipeManager.h"
#include "FGSaveSession.h"
#include "FGSplineBuildableInterface.h"

#include "Patching/NativeHookManager.h"

#include "CartographModSubsystem.h"
#include "CartographRemoteCallObject.h"
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


template<typename T, typename U>
    requires
		(std::is_same_v<T, FVector> || std::is_same_v<T, FVector2D>) &&
		(std::is_same_v<U, FVector> || std::is_same_v<U, FVector2D>)
FVector2D world_position_to_screen_position(const T& WorldPosition, const U& Size)
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


	for (const auto& [Material, CategoryData] : MaterialBuildCategoryDataOverrideMap)
	{
		for (const auto& [_, Recipe] : Cast<UFGFactoryCustomizationDescriptor_Material>(Material->ClassDefaultObject)->GetBuildableMap())
		{
			TSubclassOf<AFGBuildable> Buildable = Cast<UFGBuildingDescriptor>(UFGRecipe::GetDescriptorForRecipe(Recipe)->ClassDefaultObject)->mBuildableClass;
			if (!BuildableBuildCategoryDataOverrideMap.Contains(Buildable.Get()))
			{
				BuildableBuildCategoryDataOverrideMap.Add(Buildable.Get(), CategoryData);
			}
		}
	}
	

	const auto LambdaAfterLoadGame =
		[this](bool ReturnValue, UFGSaveSession* Instance, const FString& SaveName)
		{
			CARTO_LOG_DEBUG(TEXT("LoadGame"));

			ShouldInitialize = true;
            // Wait for ACartographModSubsystem to initialize
			GetWorld()->GetTimerManager().SetTimerForNextTick(
				[this, Instance]()
				{
					if (!ShouldInitialize)
					{
						return;
					}

					ShouldInitialize = false;
					IsInitializing = true;

					TArray<TWeakObjectPtr<AFGBuildable>> Factories;
					Algo::Transform(AFGBuildableSubsystem::Get(Instance)->GetAllBuildablesRef(), Factories,
						[](AFGBuildable* Buildable) { return Buildable; });
					Coroutine = InitialBuildableGather(
						std::move(Factories),
						AFGLightweightBuildableSubsystem::Get(Instance)->mBuildableClassToInstanceArray
					);
				});
		};


    const auto LambdaAfterAddFromBuildableInstanceData = 
        [this](int32 ReturnValue, AFGLightweightBuildableSubsystem* Instance, TSubclassOf<AFGBuildable> BuildableClass, 
            FRuntimeBuildableInstanceData& BuildableInstanceData, bool FromSaveData = false, int32 SaveDataBuildableIndex = INDEX_NONE, 
            uint16 ConstructId = MAX_uint16, AActor* BuildEffectInstigator = nullptr, int32 BlueprintBuildEffectIndex = INDEX_NONE)
        {
			CARTO_LOG_DEBUG(TEXT("AddFromBuildableInstanceData: %s, Skip: %d"), *BuildableClass->GetName(), ShouldInitialize || FromSaveData || IsClient);

			if (ShouldInitialize || FromSaveData || IsClient)
			{
				return;
			}

			PendingAddBuildingData.Add(
				{
                    .Buildable = nullptr,
					.BuildableClass = BuildableClass,
					.Transform = BuildableInstanceData.Transform,
					//.CustomizationData = BuildableInstanceData.CustomizationData,
				}
			);

			RedrawMap();
        };


	const auto LambdaAfterAddFromReplicatedData =
		[this](AFGLightweightBuildableSubsystem* Instance, TSubclassOf<AFGBuildable> BuildableClass, TSubclassOf<UFGRecipe> BuiltWithRecipe, 
			const FLightweightBuildableReplicationItem& ReplicationData, int32 MaxSize, 
			AActor* BuildEffectInstigator, int32 BlueprintBuildIndex)
		{
			CARTO_LOG_DEBUG(TEXT("AddFromReplicatedData: %s, Skip: %d"), *BuildableClass->GetName(), ShouldInitialize || IsClient);

			if (ShouldInitialize || IsClient)
			{
				return;
			}

			PendingAddBuildingData.Add(
				{
					.Buildable = nullptr,
					.BuildableClass = BuildableClass,
					.Transform = ReplicationData.Transform,
					//.CustomizationData = ReplicationData.CustomizationData,
				}
			);

			RedrawMap();
		};


	const auto LambdaAfterAddBuildable =
		[this](AFGBuildableSubsystem* Instance, AFGBuildable* Buildable)
		{
			CARTO_LOG_DEBUG(TEXT("AddBuildable: %s, Skip: %d"), *Buildable->GetClass()->GetName(), ShouldInitialize || IsClient);

			if (ShouldInitialize || IsClient)
			{
				return;
			}

			PendingAddBuildingData.Add(
				{
                    .Buildable = Buildable,
					.BuildableClass = Buildable->GetClass(),
					.Transform = Buildable->GetTransform(),
					//.CustomizationData = Buildable->GetCustomizationData_Native(),
				}
			);

			RedrawMap();
		};


	const auto LambdaAfterInvalidateRuntimeInstanceDataForIndex =
		[this](AFGLightweightBuildableSubsystem* Instance, TSubclassOf<AFGBuildable> BuildableClass, int32 Index)
		{
			CARTO_LOG_DEBUG(TEXT("InvalidateRuntimeInstanceDataForIndex: %s, Skip: %d"), *BuildableClass->GetName(), ShouldInitialize || IsClient);

			if (ShouldInitialize || IsClient)
			{
				return;
			}

			const FRuntimeBuildableInstanceData* Data = Instance->GetRuntimeDataForBuildableClassAndIndex(BuildableClass, Index);

            PendingRemoveBuildingData.Add(
                {
					.Buildable = nullptr,
                    .BuildableClass = BuildableClass,
                    .Transform = Data->Transform,
                    //.CustomizationData = Data->CustomizationData,
                }
            );

			RedrawMap();
		};


	const auto LambdaAfterRemoveBuildable =
		[this](AFGBuildableSubsystem* Instance, AFGBuildable* Buildable)
		{
			CARTO_LOG_DEBUG(TEXT("RemoveBuildable: %s, Skip: %d"), *Buildable->GetClass()->GetName(), ShouldInitialize || IsClient);

			if (ShouldInitialize || IsClient)
			{
				return;
			}

			PendingRemoveBuildingData.Add(
				{
                    .Buildable = Buildable,
					.BuildableClass = Buildable->GetClass(),
					.Transform = Buildable->GetTransform(),
					//.CustomizationData = Buildable->GetCustomizationData_Native(),
				}
			);

			RedrawMap();
		};


	const auto LambdaAfterBeginPlay =
        [this](AFGPlayerController* Instance)
        {
	        AFGPlayerController* PlayerController = Cast<AFGPlayerController>(GetWorld()->GetFirstPlayerController());
			if (PlayerController->HasAuthority())
			{
				return;
			}

			auto* RCO = PlayerController->GetRemoteCallObjectOfClass<UCartographRemoteCallObject>();
			if (RCO)
			{
				IsInitializing = true;
				RCO->ServerRequestInitialBuildingData(PlayerController, EInitialDataSendPhase::Initial);
			}
			else
			{
                UE_LOG(LogCartograph, Error, TEXT("Failed to get RemoteCallObject"));
			}
        };


	// Single player or non-dedicated host
	SUBSCRIBE_UOBJECT_METHOD_AFTER(UFGSaveSession, LoadGame, LambdaAfterLoadGame);

	const auto* PlayerController = GetMutableDefault<AFGPlayerController>();
	SUBSCRIBE_METHOD_VIRTUAL_AFTER(AFGPlayerController::BeginPlay, PlayerController, LambdaAfterBeginPlay);


	SUBSCRIBE_UOBJECT_METHOD_AFTER(AFGLightweightBuildableSubsystem, AddFromBuildableInstanceData, LambdaAfterAddFromBuildableInstanceData);
    SUBSCRIBE_UOBJECT_METHOD_AFTER(AFGLightweightBuildableSubsystem, AddFromReplicatedData, LambdaAfterAddFromReplicatedData);

	SUBSCRIBE_UOBJECT_METHOD_AFTER(AFGBuildableSubsystem, AddBuildable, LambdaAfterAddBuildable);


	SUBSCRIBE_UOBJECT_METHOD_AFTER(AFGLightweightBuildableSubsystem, InvalidateRuntimeInstanceDataForIndex, LambdaAfterInvalidateRuntimeInstanceDataForIndex);

	SUBSCRIBE_UOBJECT_METHOD_AFTER(AFGBuildableSubsystem, RemoveBuildable, LambdaAfterRemoveBuildable);
}


void UCartographGameInstanceModule::OnWorldLoaded()
{
	CARTO_LOG_DEBUG(TEXT("OnWorldLoaded"));

	IsClient = GetWorld()->IsNetMode(NM_Client);

	if (!FPlatformProperties::IsServerOnly())
	{
		UKismetRenderingLibrary::ClearRenderTarget2D(this, RenderTarget, { 0, 0, 0, 0 });
	}
}


// Factories/Buildings: Intentional copies
UE5Coro::TCoroutine<> UCartographGameInstanceModule::InitialBuildableGather(
	TArray<TWeakObjectPtr<AFGBuildable>> Factories, TMap<TSubclassOf<AFGBuildable>, TArray<FRuntimeBuildableInstanceData>> Buildings, FForceLatentCoroutine)
{
    CARTO_LOG_DEBUG(TEXT("InitialBuildableGather Started. Factories: %d, Buildings: %d"), Factories.Num(), Buildings.Num());

	CurrentBuildingData.Empty(Factories.Num() + Buildings.Num());

	const float TimeBudget = FCartograph_ConfigStruct::GetActiveConfig(GetWorld()).InitializeTimeBudget;
	UE5Coro::Latent::FTickTimeBudget Budget = UE5Coro::Latent::FTickTimeBudget::Milliseconds(TimeBudget);

	for (const TWeakObjectPtr<AFGBuildable>& Factory : Factories)
	{
		if (!Factory.IsValid())
		{
			continue;
		}
		
		FBuildingData NewBuildingData{
            .Buildable = Factory,
			.BuildableClass = Factory->GetClass(),
			.Transform = Factory->GetTransform(),
            //.CustomizationData = Factory->GetCustomizationData_Native(),
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
				//.CustomizationData = InstanceData.CustomizationData,
			};

			const int32 Pos = Algo::LowerBound(CurrentBuildingData, NewBuildingData);
			CurrentBuildingData.Insert(std::move(NewBuildingData), Pos);

			co_await Budget;
		}
	}

	CARTO_LOG_DEBUG(TEXT("InitialBuildableGather Finished"));

	IsInitializing = false;
	IsPendingRedraw = false;
	ExecuteRedrawMapCoroutine();
}


void UCartographGameInstanceModule::RedrawMap()
{
	if (!Coroutine.IsDone())
	{
		if (!IsInitializing)
		{
			CARTO_LOG_DEBUG(TEXT("RedrawMapCoroutine Cancel Requested"));
			Coroutine.Cancel();
		}
		IsPendingRedraw = true;
	}
	else
	{
		ExecuteRedrawMapCoroutine();
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

	CARTO_LOG_DEBUG(TEXT("RedrawMapCoroutine Started"));

	const float TimeBudget = FCartograph_ConfigStruct::GetActiveConfig(GetWorld()).RedrawTimeBudget;
	UE5Coro::Latent::FTickTimeBudget Budget = UE5Coro::Latent::FTickTimeBudget::Milliseconds(TimeBudget);

	{
        UE5Coro::FCancellationGuard Guard{};  // We'll lose added/removed building information if the coroutine is cancelled

		for (FBuildingData& AddedBuildingData : AddedBuildings)
		{
	        CARTO_LOG_DEBUG(TEXT("AddedBuilding: %s"), *AddedBuildingData.BuildableClass->GetName());

			const int32 Pos = Algo::LowerBound(CurrentBuildingData, AddedBuildingData);
			CurrentBuildingData.Insert(std::move(AddedBuildingData), Pos);

			co_await Budget;
		}

	    for (const FBuildingData& RemovedBuildingData : RemovedBuildings)
	    {
	        CARTO_LOG_DEBUG(TEXT("RemovedBuilding: %s"), *RemovedBuildingData.BuildableClass->GetName());

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

        CARTO_LOG_DEBUG(TEXT("Buildings Change Processed"));
	}

	if (FPlatformProperties::IsServerOnly())
	{
		co_return;
	}

	UKismetRenderingLibrary::ClearRenderTarget2D(this, RenderTarget, { 0, 0, 0, 0 });

	// Sometimes lines go crazy (goes to the top or far right) if we don't delay.
	// My guess is because EndDraw and BeginDraw are called in the same frame, so I'm putting it here.
	co_await UE5Coro::Latent::NextTick();

	UCanvas* Canvas = nullptr;
	FVector2D _;
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, RenderTarget, Canvas, _, RenderContext);

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

	const AFGRecipeManager* RecipeManager = AFGRecipeManager::Get(GetWorld());

	for (const auto& [Buildable, OriginalBuildableClass, Transform/*, CustomizationData*/] : CurrentBuildingData)
	{
		CARTO_LOG_VERBOSE(TEXT("Buildable: %s, Transform: %s"), *OriginalBuildableClass->GetName(), *Transform.ToString());

		TSoftClassPtr<AFGBuildable>* RedirectClass = BuildableClassRedirectMap.Find(OriginalBuildableClass.Get());
        const TSubclassOf<AFGBuildable> BuildableClass = RedirectClass ? RedirectClass->Get() : OriginalBuildableClass.Get();

		if (BuildableClass->ImplementsInterface(UFGSplineBuildableInterface::StaticClass()))
		{
			if (!Buildable.IsValid())
			{
				continue;
			}

            const FSplineData* SplineData = BuildableSplineDataMap.Find(BuildableClass.Get());
			if (!SplineData)
			{
                UE_LOG(LogCartograph, Warning, TEXT("Can't find spline data for %s"), *BuildableClass->GetName());
				continue;
			}

			if (SplineData->Thickness < 0)
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
			if (!Buildable.IsValid())
			{
				continue;
			}

            const FWireData* WireData = BuildableWireDataMap.Find(BuildableClass.Get());
            if (!WireData)
            {
                UE_LOG(LogCartograph, Warning, TEXT("Can't find wire data for %s"), *BuildableClass->GetName());
                continue;
            }

			if (WireData->Thickness < 0)
			{
				continue;
			}

            const auto* Wire = Cast<AFGBuildableWire>(Buildable);
            const FVector Start = Wire->GetConnectionLocation(0);
            const FVector End = Wire->GetConnectionLocation(1);
            draw_line(Canvas, Start, End, WireData->Color, WireData->Thickness);

            co_await Budget;
			continue;
        }

		if (BuildableClass->IsChildOf(AFGBuildableBeam::StaticClass()))
		{
			if (!Buildable.IsValid())
			{
				continue;
			}

			const FWireData* BeamData = BuildableWireDataMap.Find(BuildableClass.Get());
			if (!BeamData)
			{
                UE_LOG(LogCartograph, Warning, TEXT("Can't find beam data for %s"), *BuildableClass->GetName());
				continue;
			}

			const auto* Beam = Cast<AFGBuildableBeam>(Buildable);
            const float Length = Beam->GetLength();
			const FVector Start = Transform.GetLocation();
            const FVector End = Start + Transform.GetRotation().Vector() * Length;
			draw_line(Canvas, Start, End, BeamData->Color, BeamData->Thickness);

            co_await Budget;
			continue;
		}


		FVector2D* Size = BuildableSizeOverrideMap.Find(BuildableClass.Get());
		if (!Size)
		{
			const FBox ClearanceBox = Cast<AFGBuildable>(BuildableClass->ClassDefaultObject)->GetCombinedClearanceBox();
			if (!ClearanceBox.IsValid)
			{
                UE_LOG(LogCartograph, Warning, TEXT("Can't find size for %s"), *BuildableClass->GetName());
				continue;
			}

			FVector2D ClearanceBoxSize{ ClearanceBox.GetSize() };
			Size = &ClearanceBoxSize;
		}
        if (Size->X == 0.f || Size->Y == 0.f)
        {
            continue;
        }
		*Size *= FVector2D{ Transform.GetScale3D() };
		const FVector2D ScreenPosition = world_position_to_screen_position(Transform.GetLocation(), *Size);
		const FRotator* ExtraRotation = BuildableExtraRotationMap.Find(BuildableClass.Get());

		if (const TSoftObjectPtr<UTexture2D>* Texture = BuildableIconOverrideMap.Find(BuildableClass.Get());
			Texture && !Texture->IsNull())
		{
			const UTexture2D* LoadedTexture = Texture->Get();
			if (!LoadedTexture)
			{
				LoadedTexture = co_await UE5Coro::Latent::AsyncLoadObject(*Texture);
			}

			FCanvasTileItem TileItem{
				ScreenPosition,
				LoadedTexture->GetResource(),
				{ Size->X * PIXEL_PER_CENTIMETER[0], Size->Y * PIXEL_PER_CENTIMETER[1] },
				{ 0, 0 },
				{ 1, 1 },
				FLinearColor::White
			};
			TileItem.PivotPoint = { 0.5, 0.5 };
			TileItem.BlendMode = FCanvas::BlendToSimpleElementBlend(EBlendMode::BLEND_Translucent);
			TileItem.Rotation = Transform.GetRotation().Rotator();
			if (ExtraRotation)
			{
				TileItem.Rotation += *ExtraRotation;
			}

			Canvas->DrawItem(TileItem);
		}
		else
		{
            const FCategoryData* CategoryData = BuildableBuildCategoryDataOverrideMap.Find(BuildableClass.Get());
			if (!CategoryData)
			{
				const TSubclassOf<UFGBuildingDescriptor> Descriptor = RecipeManager->FindBuildingDescriptorByClass(BuildableClass);
				if (TArray<TSubclassOf<UFGCategory>> Subcategories = UFGItemDescriptor::GetSubCategoriesOfClass(Descriptor, UFGBuildSubCategory::StaticClass());
					!Subcategories.IsEmpty())
				{
					CategoryData = BuildCategoryDataMap.Find(Subcategories[0].Get());
				}
				if (!CategoryData)
				{
					const TSubclassOf<UFGBuildCategory> Category = UFGBuildingDescriptor::GetBuildCategory(Descriptor);
					CategoryData = BuildCategoryDataMap.Find(Category.Get());
				}
			}
			if (!CategoryData)
			{
				UE_LOG(LogCartograph, Warning, TEXT("Can't find category data for %s"), *BuildableClass->GetName());
				continue;
			}

			FCanvasTileItem TileItem{
				ScreenPosition,
				{ Size->X * PIXEL_PER_CENTIMETER[0], Size->Y * PIXEL_PER_CENTIMETER[1] },
				CategoryData->MainColor
			};
			TileItem.PivotPoint = { 0.5, 0.5 };
			TileItem.BlendMode = FCanvas::BlendToSimpleElementBlend(EBlendMode::BLEND_Translucent);
			TileItem.Rotation = Transform.GetRotation().Rotator();
			if (ExtraRotation)
			{
				TileItem.Rotation += *ExtraRotation;
			}

			Canvas->DrawItem(TileItem);
			co_await Budget;

			const float HalfWidth = Size->X / 2;
			const float HalfHeight = Size->Y / 2;
			FVector LocalCorners[] = {
				{ -HalfWidth, -HalfHeight, 0 },
				{ HalfWidth, -HalfHeight, 0 },
				{ HalfWidth,  HalfHeight, 0 },
				{ -HalfWidth,  HalfHeight, 0 },
			};

			FTransform TransformNoScale = Transform;
            TransformNoScale.SetScale3D(FVector::OneVector);
			for (FVector& Corner : LocalCorners)
			{
				Corner = TransformNoScale.TransformPosition(Corner);
			}

			if (CategoryData->OutlineThickness > 0)
			{
				draw_line(Canvas, LocalCorners[0], LocalCorners[1], CategoryData->OutlineColor, CategoryData->OutlineThickness);
				draw_line(Canvas, LocalCorners[1], LocalCorners[2], CategoryData->OutlineColor, CategoryData->OutlineThickness);
				draw_line(Canvas, LocalCorners[2], LocalCorners[3], CategoryData->OutlineColor, CategoryData->OutlineThickness);
				draw_line(Canvas, LocalCorners[3], LocalCorners[0], CategoryData->OutlineColor, CategoryData->OutlineThickness);
			}
		}

		co_await Budget;
	}

	CARTO_LOG_DEBUG(TEXT("RedrawMapCoroutine Finished"));
}


void UCartographGameInstanceModule::OnCoroutineFinishedOrCancelled()
{
    CARTO_LOG_DEBUG(TEXT("OnCoroutineFinishedOrCancelled"));

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
	ExecuteRedrawMapCoroutine();
}


void UCartographGameInstanceModule::ExecuteRedrawMapCoroutine()
{
	Coroutine = RedrawMapCoroutine(PendingAddBuildingData, PendingRemoveBuildingData);
	ACartographModSubsystem::Instance->ClientUpdateBuildingData(PendingAddBuildingData, PendingRemoveBuildingData);
	PendingAddBuildingData.Empty();
	PendingRemoveBuildingData.Empty();
}
