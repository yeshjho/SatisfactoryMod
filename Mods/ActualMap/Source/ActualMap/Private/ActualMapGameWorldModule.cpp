#include "ActualMapGameWorldModule.h"

#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"

#include "FGLightweightBuildableSubsystem.h"

#include "Patching/NativeHookManager.h"


constexpr int RENDER_TEXTURE_SIZE = 4096;

constexpr double WEST_BOUND_CENTIMETERS = -324'698.832031;
constexpr double EAST_BOUND_CENTIMETERS = 425'301.832031;
constexpr double NORTH_BOUND_CENTIMETERS = -375'000;
constexpr double SOUTH_BOUND_CENTIMETERS = 375'000;
constexpr double MAP_WIDTH_CENTIMETERS = EAST_BOUND_CENTIMETERS - WEST_BOUND_CENTIMETERS;
constexpr double MAP_HEIGHT_CENTIMETERS = SOUTH_BOUND_CENTIMETERS - NORTH_BOUND_CENTIMETERS;
constexpr double ORIGIN_UV[] = { -WEST_BOUND_CENTIMETERS / MAP_WIDTH_CENTIMETERS, -NORTH_BOUND_CENTIMETERS / MAP_HEIGHT_CENTIMETERS };


UE5Coro::TCoroutine<> UActualMapGameInstanceModule::RedrawMap(AFGLightweightBuildableSubsystem* Instance, FForceLatentCoroutine)
{
	UKismetRenderingLibrary::ClearRenderTarget2D(this, RenderTarget, { 0, 0, 0, 0 });

	UCanvas* Canvas = nullptr;
	FVector2D Size;
	FDrawToRenderTargetContext Context;
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(this, RenderTarget, Canvas, Size, Context);

	auto Budget = UE5Coro::Latent::FTickTimeBudget::Milliseconds(1);

	for (const auto& [type, arr] : Instance->mBuildableClassToInstanceArray)
	{
		for (const auto& data : arr)
		{
			//FVector loc = data.Transform.GetLocation();
			//FRotator rot = data.Transform.GetRotation().Rotator();

			//FVector2D ScreenPosition = FVector2D(originUV[0] + loc.X / mapWorldSize, originUV[1] + loc.Y / mapWorldSize) * renderTextureSize;
			//float Rotation = rot.Yaw;
			//FCanvasTileItem TileItem(ScreenPosition, RenderTexture->GetResource(), { 800 * pixelPerMeter, 800 * pixelPerMeter }, { 0, 0 }, { 1, 1 }, FLinearColor::White);
			//TileItem.Rotation = FRotator(0, Rotation, 0);
			//TileItem.PivotPoint = { 0.5, 0.5 };
			//TileItem.BlendMode = FCanvas::BlendToSimpleElementBlend(EBlendMode::BLEND_Translucent);
			////TileItem.Z = loc.Z;
			//Canvas->DrawItem(TileItem);

			co_await Budget;
		}
	}

	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(this, Context);
}


void UActualMapGameInstanceModule::DispatchLifecycleEvent(ELifecyclePhase Phase)
{
	Super::DispatchLifecycleEvent(Phase);

    if (Phase != ELifecyclePhase::POST_INITIALIZATION)
    {
        return;
    }


    const auto LambdaAfterAddFromBuildableInstanceData = 
        [](int32 ReturnValue, AFGLightweightBuildableSubsystem* Instance, TSubclassOf< class AFGBuildable > BuildableClass, 
            FRuntimeBuildableInstanceData& BuildableInstanceData, bool FromSaveData = false, int32 SaveDataBuildableIndex = INDEX_NONE, 
            uint16 ConstructId = MAX_uint16, AActor* BuildEffectInstigator = nullptr, int32 BlueprintBuildEffectIndex = INDEX_NONE)
        {
            UE_LOG(LogTemp, Warning, TEXT("Added buildable instance data"));
        };

    SUBSCRIBE_UOBJECT_METHOD_AFTER(AFGLightweightBuildableSubsystem, AddFromBuildableInstanceData, LambdaAfterAddFromBuildableInstanceData);
}
