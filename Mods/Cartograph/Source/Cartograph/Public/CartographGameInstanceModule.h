#pragma once

#include "CoreMinimal.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Module/GameInstanceModule.h"

#include "FGLightweightBuildableSubsystem.h"

#include "UE5Coro/UE5Coro.h"

#include "CartographGameInstanceModule.generated.h"


class AFGBuildable;
class AFGLightweightBuildableSubsystem;
class UCanvasRenderTarget2D;
class UFGBuildCategory;


DECLARE_LOG_CATEGORY_EXTERN(LogCartograph, Display, All);


constexpr bool ENABLE_DEBUG_LOG = true;
constexpr bool ENABLE_VERBOSE_LOG = false;
constexpr bool ENABLE_VERY_VERBOSE_LOG = false;
#define CARTO_LOG_DEBUG(...) if constexpr (ENABLE_DEBUG_LOG) UE_LOG(LogCartograph, Display, __VA_ARGS__)
#define CARTO_LOG_VERBOSE(...) if constexpr (ENABLE_VERBOSE_LOG) UE_LOG(LogCartograph, Display, __VA_ARGS__)
#define CARTO_LOG_VERY_VERBOSE(...) if constexpr (ENABLE_VERY_VERBOSE_LOG) UE_LOG(LogCartograph, Display, __VA_ARGS__)


USTRUCT()
struct FBuildingData
{
	GENERATED_BODY()

	UPROPERTY()
    TWeakObjectPtr<AFGBuildable> Buildable;  // nullptr for LightweightBuildables.
	UPROPERTY()
    TSubclassOf<AFGBuildable> BuildableClass;
	UPROPERTY()
	FTransform Transform;
	//UPROPERTY()
	//FFactoryCustomizationData CustomizationData;

	bool operator==(const FBuildingData& Other) const noexcept;
	auto operator<=>(const FBuildingData& Other) const noexcept;
};


USTRUCT()
struct FCategoryData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	FLinearColor MainColor;

	UPROPERTY(EditDefaultsOnly)
	FLinearColor OutlineColor;

	UPROPERTY(EditDefaultsOnly)
	float OutlineThickness;
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


USTRUCT()
struct FWireData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
    FLinearColor Color;

	UPROPERTY(EditDefaultsOnly)
	float Thickness;
};


/**
 * 
 */
UCLASS()
class CARTOGRAPH_API UCartographGameInstanceModule : public UGameInstanceModule
{
	GENERATED_BODY()

    friend class ACartographModSubsystem;
	friend class UCartographRemoteCallObject;

public:
	virtual void DispatchLifecycleEvent(ELifecyclePhase Phase) override;

	void OnWorldLoaded();

private:
	UE5Coro::TCoroutine<> InitialBuildableGather(TArray<TWeakObjectPtr<AFGBuildable>> Factories, TMap<TSubclassOf<AFGBuildable>, TArray<FRuntimeBuildableInstanceData>> Buildings, FForceLatentCoroutine = {});
    void RedrawMap();
	UE5Coro::TCoroutine<> RedrawMapCoroutine(TArray<FBuildingData> AddedBuildings, TArray<FBuildingData> RemovedBuildings, FForceLatentCoroutine = {});

	void OnCoroutineFinishedOrCancelled();

	void ExecuteRedrawMapCoroutine();


protected:
	UPROPERTY(EditDefaultsOnly)
	TMap<TSoftClassPtr<UFGBuildCategory>, FCategoryData> BuildCategoryDataMap;

	UPROPERTY(EditDefaultsOnly)
	TMap<TSoftClassPtr<AFGBuildable>, FCategoryData> BuildableBuildCategoryDataOverrideMap;

	UPROPERTY(EditDefaultsOnly)
	TMap<TSubclassOf<UFGFactoryCustomizationDescriptor_Material>, FCategoryData> MaterialBuildCategoryDataOverrideMap;

	UPROPERTY(EditDefaultsOnly)
	TMap<TSoftClassPtr<AFGBuildable>, TSoftObjectPtr<UTexture2D>> BuildableIconOverrideMap;

	UPROPERTY(EditDefaultsOnly)
	TMap<TSoftClassPtr<AFGBuildable>, FVector2D> BuildableSizeOverrideMap;

	UPROPERTY(EditDefaultsOnly)
	TMap<TSoftClassPtr<AFGBuildable>, FRotator> BuildableExtraRotationMap;

	UPROPERTY(EditDefaultsOnly)
	TMap<TSoftClassPtr<AFGBuildable>, FSplineData> BuildableSplineDataMap;

	UPROPERTY(EditDefaultsOnly)
    TMap<TSoftClassPtr<AFGBuildable>, FWireData> BuildableWireDataMap;

	UPROPERTY(EditDefaultsOnly)
	TMap<TSoftClassPtr<AFGBuildable>, TSoftClassPtr<AFGBuildable>> BuildableClassRedirectMap;

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


    bool IsClient = false;
};
