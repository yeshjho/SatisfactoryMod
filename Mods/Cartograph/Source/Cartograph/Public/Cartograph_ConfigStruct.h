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

    /* Retrieves active configuration value and returns object of this struct containing it */
    static FCartograph_ConfigStruct GetActiveConfig(UObject* WorldContext) {
        FCartograph_ConfigStruct ConfigStruct{};
        FConfigId ConfigId{"Cartograph", ""};
        if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull)) {
            UConfigManager* ConfigManager = World->GetGameInstance()->GetSubsystem<UConfigManager>();
            ConfigManager->FillConfigurationStruct(ConfigId, FDynamicStructInfo{FCartograph_ConfigStruct::StaticStruct(), &ConfigStruct});
        }
        return ConfigStruct;
    }
};

