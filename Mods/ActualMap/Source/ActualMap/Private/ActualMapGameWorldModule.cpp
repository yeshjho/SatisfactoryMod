#include "ActualMapGameWorldModule.h"

#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/CanvasRenderTarget2D.h"

#include "FGLightweightBuildableSubsystem.h"
#include "FGBuildable.h"
#include "FGBuildableFoundation.h"
#include "FGSaveSession.h"

#include "Patching/NativeHookManager.h"


constexpr int RENDER_TEXTURE_SIZE = 1024 * 8;

constexpr double WEST_BOUND_CENTIMETERS = -324'698.832031;
constexpr double EAST_BOUND_CENTIMETERS = 425'301.832031;
constexpr double NORTH_BOUND_CENTIMETERS = -375'000;
constexpr double SOUTH_BOUND_CENTIMETERS = 375'000;
constexpr double MAP_WIDTH_CENTIMETERS = EAST_BOUND_CENTIMETERS - WEST_BOUND_CENTIMETERS;
constexpr double MAP_HEIGHT_CENTIMETERS = SOUTH_BOUND_CENTIMETERS - NORTH_BOUND_CENTIMETERS;
constexpr double ORIGIN_UV[] = { -WEST_BOUND_CENTIMETERS / MAP_WIDTH_CENTIMETERS, -NORTH_BOUND_CENTIMETERS / MAP_HEIGHT_CENTIMETERS };
constexpr double PIXEL_PER_CENTIMETER[] = { RENDER_TEXTURE_SIZE / MAP_WIDTH_CENTIMETERS, RENDER_TEXTURE_SIZE / MAP_HEIGHT_CENTIMETERS };


FVector2D world_position_to_screen_position(const FVector& WorldPosition, float Width, float Height)
{
	return FVector2D{
		// TODO: Width / 2 & Height / 2: Only verified for foundations
		ORIGIN_UV[0] + (WorldPosition.X - Width / 2) / MAP_WIDTH_CENTIMETERS,
		ORIGIN_UV[1] + (WorldPosition.Y - Height / 2) / MAP_HEIGHT_CENTIMETERS
	} * RENDER_TEXTURE_SIZE;
}


auto FBuildingData::operator<=>(const FBuildingData& Other) const noexcept
{
	return Transform.GetLocation().Z <=> Other.Transform.GetLocation().Z;
}


bool FBuildingData::operator==(const FBuildingData& Other) const noexcept
{
    return BuildableClass == Other.BuildableClass && Transform.Equals(Other.Transform, 0);
    // Ignoring CustomizationData on purpose
}


void UActualMapGameInstanceModule::DispatchLifecycleEvent(ELifecyclePhase Phase)
{
	Super::DispatchLifecycleEvent(Phase);

    if (Phase != ELifecyclePhase::POST_INITIALIZATION)
    {
        return;
    }


	const auto LambdaAfterLoadGame =
		[this](bool ReturnValue, UFGSaveSession* Instance, const FString& SaveName)
		{
			InitialBuildableGather(AFGLightweightBuildableSubsystem::Get(Instance));
			RedrawMap(AFGLightweightBuildableSubsystem::Get(Instance));
		};


    const auto LambdaAfterAddFromBuildableInstanceData = 
        [this](int32 ReturnValue, AFGLightweightBuildableSubsystem* Instance, TSubclassOf<AFGBuildable> BuildableClass, 
            FRuntimeBuildableInstanceData& BuildableInstanceData, bool FromSaveData = false, int32 SaveDataBuildableIndex = INDEX_NONE, 
            uint16 ConstructId = MAX_uint16, AActor* BuildEffectInstigator = nullptr, int32 BlueprintBuildEffectIndex = INDEX_NONE)
        {
			if (FromSaveData)
			{
				return;
			}

			PendingAddBuildingData.Add(
				{
					.BuildableClass = BuildableClass,
					.Transform = BuildableInstanceData.Transform,
					.CustomizationData = BuildableInstanceData.CustomizationData,
				}
			);
        	RedrawMap(Instance);
        };


	const auto LambdaAfterAddFromReplicatedData =
		[this](AFGLightweightBuildableSubsystem* Instance, TSubclassOf<AFGBuildable> BuildableClass, TSubclassOf<UFGRecipe> BuiltWithRecipe, 
			const FLightweightBuildableReplicationItem& ReplicationData, int32 MaxSize, 
			AActor* BuildEffectInstigator, int32 BlueprintBuildIndex)
		{
			PendingAddBuildingData.Add(
				{
					.BuildableClass = BuildableClass,
					.Transform = ReplicationData.Transform,
					.CustomizationData = ReplicationData.CustomizationData,
				}
			);
			RedrawMap(Instance);
		};


	const auto LambdaOnRemoveByBuildable = 
		[this](auto& Scope, AFGLightweightBuildableSubsystem* Instance, AFGBuildable* Buildable)
		{
			const int32 Index = Instance->GetRuntimeDataIndexForBuildable(Buildable);
			FRuntimeBuildableInstanceData* Data = Instance->GetRuntimeDataForBuildableClassAndIndex(Buildable->GetClass(), Index);

            PendingRemoveBuildingData.Add(
                {
                    .BuildableClass = Buildable->GetClass(),
                    .Transform = Data->Transform,
                    .CustomizationData = Data->CustomizationData,
                }
            );
            RedrawMap(Instance);
		};

	const auto l2
		= [](AFGLightweightBuildableSubsystem* Instance, TSubclassOf<  AFGBuildable > buildableClass, int32)
		{
			UE_LOG(LogTemp, Warning, TEXT("ActualMap: RemoveByInstanceIndex"));
		};


	SUBSCRIBE_UOBJECT_METHOD_AFTER(UFGSaveSession, LoadGame, LambdaAfterLoadGame);

	SUBSCRIBE_UOBJECT_METHOD_AFTER(AFGLightweightBuildableSubsystem, AddFromBuildableInstanceData, LambdaAfterAddFromBuildableInstanceData);
    SUBSCRIBE_UOBJECT_METHOD_AFTER(AFGLightweightBuildableSubsystem, AddFromReplicatedData, LambdaAfterAddFromReplicatedData);

	SUBSCRIBE_UOBJECT_METHOD(AFGLightweightBuildableSubsystem, RemoveByBuildable, LambdaOnRemoveByBuildable);
}


void UActualMapGameInstanceModule::InitialBuildableGather(AFGLightweightBuildableSubsystem* Instance, FForceLatentCoroutine)
{
	CurrentBuildingData.Empty(UFGSaveSession::mObjectToSerailizedVersion.Num() + Instance->mBuildableClassToInstanceArray.Num());

	for (const auto [Object, _] : UFGSaveSession::mObjectToSerailizedVersion)
	{
		if (!Object->IsA<AFGBuildable>())
		{
			continue;
		}

		const auto* Buildable = Cast<AFGBuildable>(Object);

		FBuildingData NewBuildingData{
			.BuildableClass = Buildable->GetClass(),
			.Transform = Buildable->GetTransform(),
		};

		const int32 Pos = Algo::LowerBound(CurrentBuildingData, NewBuildingData);
		CurrentBuildingData.Insert(std::move(NewBuildingData), Pos);
	}

	for (const auto& [Type, Arr] : Instance->mBuildableClassToInstanceArray)
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
		}
	}
}


void UActualMapGameInstanceModule::RedrawMap(AFGLightweightBuildableSubsystem* Instance)
{
	if (!Coroutine.IsDone())
	{
        UE_LOG(LogTemp, Warning, TEXT("RedrawMap is already running. Cancelling the previous one."));
		Coroutine.Cancel();
		IsPendingRedraw = true;
	}
	else
	{
        UE_LOG(LogTemp, Warning, TEXT("RedrawMap is not running. Starting a new one."));
		Coroutine = RedrawMapCoroutine();
	}
}


UE5Coro::TCoroutine<> UActualMapGameInstanceModule::RedrawMapCoroutine(FForceLatentCoroutine)
{
	ON_SCOPE_EXIT
	{
        OnCoroutineFinishedOrCancelled();
	};

	TArray<FBuildingData> PendingAddBuildingDataCopy = PendingAddBuildingData;
	PendingAddBuildingData.Empty();

    TArray<FBuildingData> PendingRemoveBuildingDataCopy = PendingRemoveBuildingData;
    PendingRemoveBuildingData.Empty();


	UE5Coro::Latent::FTickTimeBudget Budget = UE5Coro::Latent::FTickTimeBudget::Milliseconds(1);

	for (FBuildingData& NewBuildingData : PendingAddBuildingDataCopy)
	{
		const int32 Pos = Algo::LowerBound(CurrentBuildingData, NewBuildingData);
		CurrentBuildingData.Insert(std::move(NewBuildingData), Pos);

		co_await Budget;
	}

    for (const FBuildingData& RemoveBuildingData : PendingRemoveBuildingDataCopy)
    {
        const int32 Start = Algo::LowerBound(CurrentBuildingData, RemoveBuildingData);
        const int32 End = Algo::UpperBound(CurrentBuildingData, RemoveBuildingData);

        for (int32 i = Start; i < End; ++i)
        {
            if (CurrentBuildingData[i] == RemoveBuildingData)
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
		const TSoftObjectPtr<UTexture2D> Texture = BuildableToIconMap.FindRef(BuildableClass.Get());
		if (Texture.IsNull())
		{
			continue;
		}

		float Width = 0;
		float Height = 0;
		if (BuildableClass->IsChildOf(AFGBuildableFoundation::StaticClass()))
		{
			AFGBuildableFoundation* Foundation = Cast<AFGBuildableFoundation>(BuildableClass->ClassDefaultObject);
			Width = Foundation->mWidth;
			Height = Foundation->mDepth;
		}
		else
		{
			continue;
		}

		const UTexture2D* LoadedTexture = Texture.Get();
		if (!LoadedTexture)
		{
			LoadedTexture = co_await UE5Coro::Latent::AsyncLoadObject(Texture);
		}

		const auto& Scale = Transform.GetScale3D();
		Width *= Scale.X;
		Height *= Scale.Y;

		const FVector2D ScreenPosition = world_position_to_screen_position(Transform.GetLocation(), Width, Height);
		FCanvasTileItem TileItem{
			ScreenPosition,
			LoadedTexture->GetResource(),
			{ Width * PIXEL_PER_CENTIMETER[0], Height * PIXEL_PER_CENTIMETER[1] },
			{ 0, 0 },
			{ 1, 1 },
			FLinearColor::White
		};
		TileItem.Rotation = Transform.GetRotation().Rotator();
		TileItem.PivotPoint = { 0.5, 0.5 };
		TileItem.BlendMode = FCanvas::BlendToSimpleElementBlend(EBlendMode::BLEND_Translucent);
		Canvas->DrawItem(TileItem);

		co_await Budget;
	}
}


void UActualMapGameInstanceModule::OnCoroutineFinishedOrCancelled()
{
	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, RenderContext);

	if (!IsPendingRedraw)
	{
		return;
	}

	// The coroutine is also on the game thread, so I think no data race here.
    IsPendingRedraw = false;
	Coroutine = RedrawMapCoroutine();
}
