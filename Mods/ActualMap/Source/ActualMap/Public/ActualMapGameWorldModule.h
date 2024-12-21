#pragma once

#include "CoreMinimal.h"
#include "Module/GameInstanceModule.h"

#include "UE5Coro/UE5Coro.h"

#include "ActualMapGameWorldModule.generated.h"


class UCanvasRenderTarget2D;

class AFGBuildable;
class AFGLightweightBuildableSubsystem;


/**
 * 
 */
UCLASS()
class ACTUALMAP_API UActualMapGameInstanceModule : public UGameInstanceModule
{
	GENERATED_BODY()

public:
	virtual void DispatchLifecycleEvent(ELifecyclePhase Phase) override;

private:
	UE5Coro::TCoroutine<> RedrawMap(AFGLightweightBuildableSubsystem* Instance, FForceLatentCoroutine);


protected:
	UPROPERTY(EditDefaultsOnly)
    TMap<TSoftClassPtr<AFGBuildable>, TSoftObjectPtr<UTexture2D>> BuildableToIconMap;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UCanvasRenderTarget2D> RenderTarget;
};
