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
#define CARTO_LOG_DEBUG(format, ...) if constexpr (ENABLE_DEBUG_LOG) UE_LOG(LogCartograph, Display, TEXT(format) __VA_OPT__(, __VA_ARGS__))
#define CARTO_LOG_VERBOSE(format, ...) if constexpr (ENABLE_VERBOSE_LOG) UE_LOG(LogCartograph, Display, TEXT(format) __VA_OPT__(, __VA_ARGS__))
#define CARTO_LOG_VERY_VERBOSE(format, ...) if constexpr (ENABLE_VERY_VERBOSE_LOG) UE_LOG(LogCartograph, Display, TEXT(format) __VA_OPT__(, __VA_ARGS__))

#define CARTO_LOG_ERROR_RETURN_IF_NULL(ptr) if (!ptr) { UE_LOG(LogCartograph, Error, TEXT("'%s' is null"), TEXT(#ptr)); return; }


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


enum class EBuildingDataType
{
	Invalid = 0,

	Icon = 1 << 0,
	Rectangle = 1 << 1,
	Spline = 1 << 2,
	Wire = 1 << 3,
	Beam = 1 << 4,

    Normal = Icon | Rectangle,
    Special = Spline | Wire | Beam,
};
ENUM_CLASS_FLAGS(EBuildingDataType)


struct FRectangleDataCache
{
    const struct FCategoryData* CategoryData;
	FVector LocalCorners[4];
};


struct FNormalDataCache
{
	FVector2D ScreenPosition;
	FVector2D Size;
	FRotator Rotation;
	std::variant<TSoftObjectPtr<UTexture2D>, FRectangleDataCache> IconOrRectangleData;
};


struct FSplineDataCache
{
    const struct FSplineData* SplineData;
	TArray<FVector2D> StartPoints;
    TArray<FVector2D> EndPoints;
};


USTRUCT()
struct FBuildingData
{
	GENERATED_BODY()

	uint32 BuildableClassHash = 0;
	FTransform Transform;
	//FFactoryCustomizationData CustomizationData;
	std::variant<std::monostate, FSplineExtraData, FWireExtraData, FBeamExtraData> BuildableExtraData;


    EBuildingDataType DataType = EBuildingDataType::Invalid;
	std::variant<FNormalDataCache, FSplineDataCache, const struct FWireData*> DataCache;
    const struct FBuildLayerData* LayerDataCache;


    bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);

	bool operator==(const FBuildingData& Other) const noexcept;
	std::partial_ordering operator<=>(const FBuildingData& Other) const noexcept;
    std::partial_ordering operator<=>(float Z) const noexcept;

	void FillInCache(TSubclassOf<AFGBuildable> OriginalBuildableClass);  // Call it after filling in the extra data
    void FillInHash(TSubclassOf<AFGBuildable> OriginalBuildableClass);  // Call it after filling in the extra data
	void FillInHashAndCache(TSubclassOf<AFGBuildable> BuildableClass);  // Call it after filling in the extra data
    void CalculateSplinePoints();  // Call it after filling in the extra data & cache
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


USTRUCT()
struct FLayerSubCategoryData
{
	GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly)
    FName Name;

	UPROPERTY(EditDefaultsOnly)
	FText DisplayName;

	/** Lower = Earlier in the list **/
	UPROPERTY(EditDefaultsOnly)
	int Priority;
};


USTRUCT()
struct FLayerCategoryData : public FLayerSubCategoryData
{
    GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly)
	TArray<FLayerSubCategoryData> SubCategories;
};


USTRUCT()
struct FBuildLayerData
{
	GENERATED_BODY()

    UPROPERTY(EditDefaultsOnly, meta = (GetOptions = "GetLayerCategoryOptions"))
	FString Category;

	FName MainCategoryCache;
    FName SubCategoryCache;
};


struct FRuntimeConfig
{
	TSet<FName> DisabledLayerMainCategory;
	TMap<FName, TSet<FName>> DisabledLayerSubCategory;
	TSet<uint32> DisabledLayerBuildable;
};


/**
 * 
 */
UCLASS(PrioritizeCategories=("Draw Data", "Layer Data", "UI", "Advanced", "Default", "Generated Data"))
class CARTOGRAPH_API UCartographGameInstanceModule : public UGameInstanceModule
{
	GENERATED_BODY()

    friend class ACartographModSubsystem;
	friend class UCartographRemoteCallObject;

public:
	virtual void DispatchLifecycleEvent(ELifecyclePhase Phase) override;

	void OnWorldLoaded();
	void OnWorldUnloaded();

	void OnLayerConfigChanged();

	const FBuildLayerData* GetBuildLayerData(uint32 ClassHash);

	bool DoesBuildingExist(uint32 ClassHash) const;

private:
	void RedrawMap();
	UE5Coro::TCoroutine<> InitialBuildableGather(TArray<TWeakObjectPtr<AFGBuildable>> Factories, TMap<TSubclassOf<AFGBuildable>, TArray<FRuntimeBuildableInstanceData>> Buildings, FForceLatentCoroutine = {});
	UE5Coro::TCoroutine<> RedrawMapCoroutine(TArray<FBuildingData> AddedBuildings, TArray<FBuildingData> RemovedBuildings, FForceLatentCoroutine = {});

	void OnCoroutineFinishedOrCancelled();

	void ExecuteRedrawMapCoroutine();

	void AddExtraData(FBuildingData& BuildingData, AFGBuildable* Buildable);

	void RegisterMenuButton() const;

	void LoadRuntimeConfig();
    void SaveRuntimeConfig();

	void FillBuildLayerDataCache();

#if WITH_EDITOR
	virtual void PostCDOContruct() override;
#endif

	// For blueprint use only
private:
	UFUNCTION()
	void AfterSplineSegmentsModified();

	UFUNCTION(BlueprintCallable)
	void OnZFilterUpdated(float Min, float Max);

	UFUNCTION(BlueprintCallable)
	void OnCartographMenuButtonClicked(UUserWidget* Widget, bool IsOpen);

	UFUNCTION(BlueprintCallable)
	void OnShowBuildingsCheckboxChanged(bool DoShow);

	UFUNCTION()
	TArray<FString> GetLayerCategoryOptions() const;


public:
	inline static UCartographGameInstanceModule* Instance = nullptr;

	FRuntimeConfig RuntimeConfig;


	// Made it editable since it doesn't get cleared properly sometimes.
	UPROPERTY(EditDefaultsOnly, Category = "Generated Data")
    TMap<uint32, TSubclassOf<AFGBuildable>> ClassIDToClassPtrMap;
	UPROPERTY(EditDefaultsOnly, Category = "Generated Data")
    TMap<TSubclassOf<AFGBuildable>, uint32> ClassPtrToClassIDMap;


	UPROPERTY(EditDefaultsOnly, Category = "Draw Data/Category Data")
	TMap<TSoftClassPtr<UFGBuildCategory>, FCategoryData> BuildCategoryDataMap;

	UPROPERTY(EditDefaultsOnly, Category = "Draw Data/Category Data")
	TMap<TSoftClassPtr<AFGBuildable>, FCategoryData> BuildableBuildCategoryDataOverrideMap;

	UPROPERTY(EditDefaultsOnly, Category = "Draw Data/Category Data")
	TMap<TSubclassOf<UFGFactoryCustomizationDescriptor_Material>, FCategoryData> MaterialBuildCategoryDataOverrideMap;


	UPROPERTY(EditDefaultsOnly, Category = "Draw Data/Override")
	TMap<TSoftClassPtr<AFGBuildable>, TSoftObjectPtr<UTexture2D>> BuildableIconOverrideMap;

	UPROPERTY(EditDefaultsOnly, Category = "Draw Data/Override")
	TMap<TSoftClassPtr<AFGBuildable>, FVector2D> BuildableSizeOverrideMap;

	UPROPERTY(EditDefaultsOnly, Category = "Draw Data/Override")
	TMap<TSoftClassPtr<AFGBuildable>, FRotator> BuildableExtraRotationMap;


	UPROPERTY(EditDefaultsOnly, Category = "Draw Data/Special Data")
	TMap<TSoftClassPtr<AFGBuildable>, FSplineData> BuildableSplineDataMap;

	UPROPERTY(EditDefaultsOnly, Category = "Draw Data/Special Data")
    TMap<TSoftClassPtr<AFGBuildable>, FWireData> BuildableWireDataMap;


	UPROPERTY(EditDefaultsOnly, Category = "Default")
	TMap<TSoftClassPtr<AFGBuildable>, TSoftClassPtr<AFGBuildable>> BuildableClassRedirectMap;


	UPROPERTY(EditDefaultsOnly, Category = "Layer Data")
	TArray<FLayerCategoryData> LayerCategories;

	UPROPERTY(EditDefaultsOnly, Category = "Layer Data")
	TMap<TSoftClassPtr<UFGBuildCategory>, FBuildLayerData> BuildLayerDataMap;

	UPROPERTY(EditDefaultsOnly, Category = "Layer Data")
	TMap<TSoftClassPtr<AFGBuildable>, FBuildLayerData> BuildableBuildLayerDataOverrideMap;

	UPROPERTY(EditDefaultsOnly, Category = "Layer Data")
	TMap<TSubclassOf<UFGFactoryCustomizationDescriptor_Material>, FBuildLayerData> MaterialBuildLayerDataOverrideMap;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TObjectPtr<UCanvasRenderTarget2D> RenderTarget;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSoftClassPtr<UUserWidget> MapContainerWidget;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> MenuShowHideButtonWidget;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> MenuWidget;


	TMap<uint32, const FBuildLayerData*> BuildLayerDataMapCache;
	TMap<uint32, uint32> BuildingCountMap;


	bool ShouldInitialize = false;
	UPROPERTY(BlueprintReadOnly)
	bool IsInitializing = false;

	UE5Coro::TCoroutine<> Coroutine = UE5Coro::TCoroutine<>::CompletedCoroutine;
	FDrawToRenderTargetContext RenderContext;
	TArray<FBuildingData> CurrentBuildingData;

	bool IsPendingRedraw = false;
	TArray<FBuildingData> PendingAddBuildingData;
	TArray<FBuildingData> PendingRemoveBuildingData;

	bool IsInWorld = false;
    bool IsClient = false;

	float MinZFilter = -std::numeric_limits<float>::max();
    float MaxZFilter = std::numeric_limits<float>::max();

	// For blueprint use only
protected:
	UPROPERTY(BlueprintReadOnly)
	float InitializeProgress = 0;

	UPROPERTY(BlueprintReadOnly)
	float MinHeight = -100;
	UPROPERTY(BlueprintReadOnly)
	float MaxHeight = 100;

    UPROPERTY(BlueprintReadOnly)
    bool DoShowBuildings = true;
};
