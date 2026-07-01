#pragma once
#include "CoreMinimal.h"
#include "Configuration/ConfigManager.h"
#include "Engine/Engine.h"
#include "Cartograph_ConfigStruct.generated.h"

/* Struct generated from Mod Configuration Asset '/Cartograph/Cartograph_Config' */
USTRUCT(BlueprintType)
struct FCartograph_ConfigStruct {
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite)
    float InitializeTimeBudget{};

    UPROPERTY(BlueprintReadWrite)
    float RedrawTimeBudget{};

    UPROPERTY(BlueprintReadWrite)
    FString MainCategoryToggle{};

    UPROPERTY(BlueprintReadWrite)
    FString SubCategoryToggle{};

    UPROPERTY(BlueprintReadWrite)
    FString BuildingToggle{};

    /* Render-target resolution, as the ECartographRenderResolution value (0=2K, 1=4K, 2=6K, 3=8K).
       In the Cartograph_Config asset this is an Integer property using the "Enum" widget type pointed at
       ECartographRenderResolution, which renders as a dropdown. Lower = less VRAM, blurrier map.
       Falls back to 1 (4K) — matching DEFAULT_RENDER_TEXTURE_SIZE — if the property is missing from the
       asset or the config read fails, so a missing property doesn't silently drop to 2K. */
    UPROPERTY(BlueprintReadWrite)
    int32 RenderResolution{1};

    /* Generate a mip chain for the map render target. Cleaner when fully zoomed out, ~+33% VRAM.
       In the Cartograph_Config asset this is a Boolean property. */
    UPROPERTY(BlueprintReadWrite)
    bool GenerateMapMips{};

    /* Free the render target's VRAM whenever the map is closed (reallocated + repainted on open).
       Recommended default: true. In the Cartograph_Config asset this is a Boolean property.
       Defaults to true here too, so a missing/unfilled property doesn't silently disable the feature. */
    UPROPERTY(BlueprintReadWrite)
    bool FreeRenderTargetWhenMapClosed{true};

    /* Retrieves active configuration value and returns object of this struct containing it */
    static FCartograph_ConfigStruct GetActiveConfig(UObject* WorldContext) {
        FCartograph_ConfigStruct ConfigStruct{};
        FConfigId ConfigId{"Cartograph", ""};
        if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull)) {
            // Null-check the subsystem: OnWorldLoaded now reads config early in world load, where the
            // ConfigManager subsystem may not be initialized yet. Fall back to the struct's defaults
            // rather than dereferencing null.
            if (UConfigManager* ConfigManager = World->GetGameInstance()->GetSubsystem<UConfigManager>()) {
                ConfigManager->FillConfigurationStruct(ConfigId, FDynamicStructInfo{FCartograph_ConfigStruct::StaticStruct(), &ConfigStruct});
            }
        }
        return ConfigStruct;
    }
};

