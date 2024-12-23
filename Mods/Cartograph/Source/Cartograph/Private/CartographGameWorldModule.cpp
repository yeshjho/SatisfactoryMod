#include "CartographGameWorldModule.h"

#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/CanvasRenderTarget2D.h"

#include "FGLightweightBuildableSubsystem.h"
#include "FGBuildable.h"
#include "FGBuildableFoundation.h"
#include "FGBuildableSubsystem.h"
#include "FGSaveSession.h"

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

            UE_LOG(LogCartograph, Log, TEXT("AddFromBuildableInstanceData"));

			PendingAddBuildingData.Add(
				{
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

			UE_LOG(LogCartograph, Log, TEXT("AddFromReplicatedData"));

			PendingAddBuildingData.Add(
				{
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

            UE_LOG(LogCartograph, Log, TEXT("AddBuildable"));

			PendingAddBuildingData.Add(
				{
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

            UE_LOG(LogCartograph, Log, TEXT("InvalidateRuntimeInstanceDataForIndex"));

			const FRuntimeBuildableInstanceData* Data = Instance->GetRuntimeDataForBuildableClassAndIndex(BuildableClass, Index);

            PendingRemoveBuildingData.Add(
                {
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

            UE_LOG(LogCartograph, Log, TEXT("RemoveBuildable"));

			PendingRemoveBuildingData.Add(
				{
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
    UE_LOG(LogCartograph, Display, TEXT("InitialBuildableGather Started"));

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
				.BuildableClass = Type,
				.Transform = InstanceData.Transform,
				.CustomizationData = InstanceData.CustomizationData,
			};

			const int32 Pos = Algo::LowerBound(CurrentBuildingData, NewBuildingData);
			CurrentBuildingData.Insert(std::move(NewBuildingData), Pos);

			co_await Budget;
		}
	}

	UE_LOG(LogCartograph, Display, TEXT("InitialBuildableGather Finished"));

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
            UE_LOG(LogCartograph, Verbose, TEXT("RedrawMapCoroutine Cancelled"));
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

    UE_LOG(LogCartograph, Display, TEXT("RedrawMapCoroutine Started"));

	RenderContext = {};

	const float TimeBudget = FCartograph_ConfigStruct::GetActiveConfig(GetWorld()).RedrawTimeBudget;
	UE5Coro::Latent::FTickTimeBudget Budget = UE5Coro::Latent::FTickTimeBudget::Milliseconds(TimeBudget);

	for (FBuildingData& AddedBuildingData : AddedBuildings)
	{
		const int32 Pos = Algo::LowerBound(CurrentBuildingData, AddedBuildingData);
		CurrentBuildingData.Insert(std::move(AddedBuildingData), Pos);

		co_await Budget;
	}

    for (const FBuildingData& RemovedBuildingData : RemovedBuildings)
    {
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

	UKismetRenderingLibrary::ClearRenderTarget2D(this, RenderTarget, { 0, 0, 0, 0 });

	UCanvas* Canvas = nullptr;
	FVector2D Size;
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, RenderTarget, Canvas, Size, RenderContext);

	for (const auto& [BuildableClass, Transform, _] : CurrentBuildingData)
	{
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

	UE_LOG(LogCartograph, Display, TEXT("RedrawMapCoroutine Finished"));
}


void UCartographGameInstanceModule::OnCoroutineFinishedOrCancelled()
{
    if (RenderContext.RenderTarget)
    {
        UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, RenderContext);
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