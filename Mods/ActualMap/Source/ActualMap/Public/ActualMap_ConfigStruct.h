#pragma once
#include "CoreMinimal.h"
#include "Configuration/ConfigManager.h"
#include "Engine/Engine.h"
#include "ActualMap_ConfigStruct.generated.h"

/* Struct generated from Mod Configuration Asset '/ActualMap/ActualMap_Config' */
USTRUCT(BlueprintType)
struct FActualMap_ConfigStruct {
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite)
    float InitializeTimeBudget{};

    UPROPERTY(BlueprintReadWrite)
    float RedrawTimeBudget{};

    /* Retrieves active configuration value and returns object of this struct containing it */
    static FActualMap_ConfigStruct GetActiveConfig(UObject* WorldContext) {
        FActualMap_ConfigStruct ConfigStruct{};
        FConfigId ConfigId{"ActualMap", ""};
        if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull)) {
            UConfigManager* ConfigManager = World->GetGameInstance()->GetSubsystem<UConfigManager>();
            ConfigManager->FillConfigurationStruct(ConfigId, FDynamicStructInfo{FActualMap_ConfigStruct::StaticStruct(), &ConfigStruct});
        }
        return ConfigStruct;
    }
};

