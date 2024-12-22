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
			//for (const auto [obj, ver] : UFGSaveSession::mObjectToSerailizedVersion)
			//{
			//	if (!obj->IsA<AFGBuildable>())
			//	{
			//		continue;
			//	}
			//}

			RedrawMap(AFGLightweightBuildableSubsystem::Get(Instance));
		};


    const auto LambdaAfterAddFromBuildableInstanceData = 
        [this](int32 ReturnValue, AFGLightweightBuildableSubsystem* Instance, TSubclassOf< class AFGBuildable > BuildableClass, 
            FRuntimeBuildableInstanceData& BuildableInstanceData, bool FromSaveData = false, int32 SaveDataBuildableIndex = INDEX_NONE, 
            uint16 ConstructId = MAX_uint16, AActor* BuildEffectInstigator = nullptr, int32 BlueprintBuildEffectIndex = INDEX_NONE)
        {
            if (!FromSaveData)
            {
				RedrawMap(Instance);
            }
        };


	SUBSCRIBE_UOBJECT_METHOD_AFTER(UFGSaveSession, LoadGame, LambdaAfterLoadGame);
    SUBSCRIBE_UOBJECT_METHOD_AFTER(AFGLightweightBuildableSubsystem, AddFromBuildableInstanceData, LambdaAfterAddFromBuildableInstanceData);
}


void UActualMapGameInstanceModule::RedrawMap(AFGLightweightBuildableSubsystem* Instance)
{
	if (!Instance)
	{
		UE_LOG(LogTemp, Error, TEXT("ActualMap: Instance is null"));
		return;
	}

	if (!Coroutine.IsDone())
	{
        Coroutine.Cancel();
        IsPendingRedraw = true;
        PendingBuildingData.LightweightBuildables = Instance->mBuildableClassToInstanceArray;
	}
	else
	{
        CurrentBuildingData.LightweightBuildables = Instance->mBuildableClassToInstanceArray;
		Coroutine = RedrawMapCoroutine();
	}
}


UE5Coro::TCoroutine<> UActualMapGameInstanceModule::RedrawMapCoroutine(FForceLatentCoroutine)
{
	ON_SCOPE_EXIT
	{
        OnCoroutineFinishedOrCancelled();
	};

    // Copying the buildable data would have spent some time, so yielding here.
	co_await UE5Coro::Latent::NextTick();

	UE5Coro::Latent::FTickTimeBudget Budget = UE5Coro::Latent::FTickTimeBudget::Milliseconds(0.5);

	UKismetRenderingLibrary::ClearRenderTarget2D(this, RenderTarget, { 0, 0, 0, 0 });

	UCanvas* Canvas = nullptr;
	FVector2D Size;
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, RenderTarget, Canvas, Size, RenderContext);

	for (const auto& [Type, Arr] : CurrentBuildingData.LightweightBuildables)
	{
		float Width = 0;
		float Height = 0;
		if (Type->IsChildOf(AFGBuildableFoundation::StaticClass()))
		{
			AFGBuildableFoundation* Foundation = Cast<AFGBuildableFoundation>(Type->ClassDefaultObject);
			Width = Foundation->mWidth;
			Height = Foundation->mDepth;
		}
		else
		{
			continue;
		}

		const TSoftObjectPtr<UTexture2D> Texture = BuildableToIconMap.FindRef(Type.Get());
		if (Texture.IsNull())
		{
			continue;
		}
		const UTexture2D* LoadedTexture = co_await UE5Coro::Latent::AsyncLoadObject(Texture);

		for (const FRuntimeBuildableInstanceData& InstanceData : Arr)
		{
			const auto& Scale = InstanceData.Transform.GetScale3D();
            Width *= Scale.X;
            Height *= Scale.Y;

			const FVector2D ScreenPosition = world_position_to_screen_position(InstanceData.Transform.GetLocation(), Width, Height);
			FCanvasTileItem TileItem{
				ScreenPosition,
				LoadedTexture->GetResource(),
				{ Width * PIXEL_PER_CENTIMETER[0], Height * PIXEL_PER_CENTIMETER[1] },
				{ 0, 0 },
				{ 1, 1 },
				FLinearColor::White
			};
			TileItem.Rotation = InstanceData.Transform.GetRotation().Rotator();
			TileItem.PivotPoint = { 0.5, 0.5 };
			TileItem.BlendMode = FCanvas::BlendToSimpleElementBlend(EBlendMode::BLEND_Translucent);
			Canvas->DrawItem(TileItem);

			co_await Budget;
		}
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
    CurrentBuildingData = std::move(PendingBuildingData);
	Coroutine = RedrawMapCoroutine();
}
