#pragma once

#include "CoreMinimal.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Module/GameInstanceModule.h"

#include "FGLightweightBuildableSubsystem.h"

#include "UE5Coro/UE5Coro.h"

#include "CartographGameWorldModule.generated.h"


class UCanvasRenderTarget2D;

class AFGBuildable;
class AFGLightweightBuildableSubsystem;


DECLARE_LOG_CATEGORY_EXTERN(LogCartograph, Display, All);


struct FBuildingData
{
    AFGBuildable* Buildable;  // nullptr for LightweightBuildables. Make sure to check IsValid(Buildable) before using it.
    TSubclassOf<AFGBuildable> BuildableClass;
	FTransform Transform;
	FFactoryCustomizationData CustomizationData;

	bool operator==(const FBuildingData& Other) const noexcept;
	auto operator<=>(const FBuildingData& Other) const noexcept;
};


USTRUCT()
struct FSplineData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	FLinearColor Color;

	UPROPERTY(EditDefaultsOnly)
    float Thickness;

	UPROPERTY(EditDefaultsOnly)
	FName SparsityConfigName;

	int SparsityCached;
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInitializationStarted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInitializationFinished);


/**
 * 
 */
UCLASS()
class CARTOGRAPH_API UCartographGameInstanceModule : public UGameInstanceModule
{
	GENERATED_BODY()

public:
	virtual void DispatchLifecycleEvent(ELifecyclePhase Phase) override;

private:
	UE5Coro::TCoroutine<> InitialBuildableGather(TArray<AFGBuildable*> Factories, TMap<TSubclassOf<AFGBuildable>, TArray<FRuntimeBuildableInstanceData>> Buildings, FForceLatentCoroutine = {});
    void RedrawMap();
	UE5Coro::TCoroutine<> RedrawMapCoroutine(TArray<FBuildingData> AddedBuildings, TArray<FBuildingData> RemovedBuildings, FForceLatentCoroutine = {});

	void OnCoroutineFinishedOrCancelled();


public:
	UPROPERTY(BlueprintAssignable)
    FOnInitializationStarted OnInitializationStarted;
	UPROPERTY(BlueprintAssignable)
    FOnInitializationFinished OnInitializationFinished;

protected:
	UPROPERTY(EditDefaultsOnly)
    TMap<TSoftClassPtr<AFGBuildable>, TSoftObjectPtr<UTexture2D>> BuildableToIconMap;

	UPROPERTY(EditDefaultsOnly)
	TMap<TSoftClassPtr<AFGBuildable>, FVector> BuildableSizeMap;

	UPROPERTY(EditDefaultsOnly)
	TMap<TSoftClassPtr<AFGBuildable>, FRotator> BuildableExtraRotationMap;

	UPROPERTY(EditDefaultsOnly)
	TMap<TSoftClassPtr<AFGBuildable>, FSplineData> BuildableSplineDataMap;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UCanvasRenderTarget2D> RenderTarget;

	bool ShouldInitialize = false;
	UPROPERTY(BlueprintReadOnly)
	bool IsInitializing = false;

	UE5Coro::TCoroutine<> Coroutine = UE5Coro::TCoroutine<>::CompletedCoroutine;
	FDrawToRenderTargetContext RenderContext;
	TArray<FBuildingData> CurrentBuildingData;

	bool IsPendingRedraw = false;
	TArray<FBuildingData> PendingAddBuildingData;
	TArray<FBuildingData> PendingRemoveBuildingData;
};
