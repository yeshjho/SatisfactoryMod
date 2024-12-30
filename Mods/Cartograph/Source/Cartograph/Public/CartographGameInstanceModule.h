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


constexpr bool ENABLE_DEBUG_LOG = false;
constexpr bool ENABLE_VERBOSE_LOG = false;
constexpr bool ENABLE_VERY_VERBOSE_LOG = false;
#define CARTO_LOG_DEBUG(format, ...) if constexpr (ENABLE_DEBUG_LOG) UE_LOG(LogCartograph, Display, TEXT(format) __VA_OPT__(, __VA_ARGS__))
#define CARTO_LOG_VERBOSE(format, ...) if constexpr (ENABLE_VERBOSE_LOG) UE_LOG(LogCartograph, Display, TEXT(format) __VA_OPT__(, __VA_ARGS__))
#define CARTO_LOG_VERY_VERBOSE(format, ...) if constexpr (ENABLE_VERY_VERBOSE_LOG) UE_LOG(LogCartograph, Display, TEXT(format) __VA_OPT__(, __VA_ARGS__))


struct FSplineExtraData
{
	TArray<FVector2D> SplinePoints;
	TOptional<std::pair<TArray<FVector2D>, TArray<FVector2D>>> Tangents;

	bool operator==(const FSplineExtraData& Other) const noexcept = default;
};


struct FWireExtraData
{
	FVector2D End;

	bool operator==(const FWireExtraData& Other) const noexcept = default;
};


struct FBeamExtraData
{
	float Length;

	bool operator==(const FBeamExtraData& Other) const noexcept = default;
};


USTRUCT()
struct FBuildingData
{
	GENERATED_BODY()

    TSubclassOf<AFGBuildable> BuildableClass;
	FTransform Transform;
	//FFactoryCustomizationData CustomizationData;
	std::variant<std::monostate, FSplineExtraData, FWireExtraData, FBeamExtraData> BuildableExtraData;

    bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);

	bool operator==(const FBuildingData& Other) const noexcept;
	auto operator<=>(const FBuildingData& Other) const noexcept;
};


FArchive& operator<<(FArchive& Ar, FBuildingData& BuildingData);


template<>
struct TStructOpsTypeTraits<FBuildingData> : public TStructOpsTypeTraitsBase2<FBuildingData>
{
	enum
	{
		WithNetSerializer = true
	};
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
	FName SegmentsConfigName;

	UPROPERTY(EditDefaultsOnly)
	bool UseTangents;

	int SegmentsCached;
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

	void AddExtraData(FBuildingData& BuildingData, AFGBuildable* Buildable);

#if WITH_EDITOR
	virtual void PostCDOContruct() override;
#endif


public:
	inline static UCartographGameInstanceModule* Instance = nullptr;

	UPROPERTY(EditDefaultsOnly)
    TMap<uint32, TSubclassOf<AFGBuildable>> ClassIDToClassPtrMap;
	UPROPERTY(EditDefaultsOnly)
    TMap<TSubclassOf<AFGBuildable>, uint32> ClassPtrToClassIDMap;


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

protected:
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UCanvasRenderTarget2D> RenderTarget;


	bool ShouldInitialize = false;
	UPROPERTY(BlueprintReadOnly)
	bool IsInitializing = false;
	UPROPERTY(BlueprintReadOnly)
	float InitializeProgress = 0;

	UE5Coro::TCoroutine<> Coroutine = UE5Coro::TCoroutine<>::CompletedCoroutine;
	FDrawToRenderTargetContext RenderContext;
	TArray<FBuildingData> CurrentBuildingData;

	bool IsPendingRedraw = false;
	TArray<FBuildingData> PendingAddBuildingData;
	TArray<FBuildingData> PendingRemoveBuildingData;


    bool IsClient = false;
};
