#pragma once

#include "CoreMinimal.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Module/GameInstanceModule.h"

#include "FGLightweightBuildableSubsystem.h"

#include "UE5Coro/UE5Coro.h"

#include "ActualMapGameWorldModule.generated.h"


class UCanvasRenderTarget2D;

class AFGBuildable;
class AFGLightweightBuildableSubsystem;


struct FBuildingData
{
    TSubclassOf<AFGBuildable> BuildableClass;
	FTransform Transform;
	FFactoryCustomizationData CustomizationData;

	bool operator==(const FBuildingData& Other) const noexcept;
	auto operator<=>(const FBuildingData& Other) const noexcept;
};


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
	//UE5Coro::TCoroutine<> InitialBuildableGatherCoroutine(AFGLightweightBuildableSubsystem* Instance, FForceLatentCoroutine = {});
	void InitialBuildableGather(AFGLightweightBuildableSubsystem* Instance, FForceLatentCoroutine = {});
    void RedrawMap(AFGLightweightBuildableSubsystem* Instance);
	UE5Coro::TCoroutine<> RedrawMapCoroutine(FForceLatentCoroutine = {});

	void OnCoroutineFinishedOrCancelled();


protected:
	UPROPERTY(EditDefaultsOnly)
    TMap<TSoftClassPtr<AFGBuildable>, TSoftObjectPtr<UTexture2D>> BuildableToIconMap;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UCanvasRenderTarget2D> RenderTarget;

	UE5Coro::TCoroutine<> Coroutine = UE5Coro::TCoroutine<>::CompletedCoroutine;
	FDrawToRenderTargetContext RenderContext;
	TArray<FBuildingData> CurrentBuildingData;

	bool IsPendingRedraw = false;
	TArray<FBuildingData> PendingAddBuildingData;
	TArray<FBuildingData> PendingRemoveBuildingData;
};
