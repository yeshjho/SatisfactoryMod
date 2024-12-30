#include "CartographGameWorldModule.h"

#include "CartographGameInstanceModule.h"
#include "GameInstanceModuleManager.h"


void UCartographGameWorldModule::DispatchLifecycleEvent(ELifecyclePhase Phase)
{
    Super::DispatchLifecycleEvent(Phase);

    if (Phase != ELifecyclePhase::POST_INITIALIZATION)
    {
        return;
    }

    UGameInstanceModule* Module = GetWorld()->GetGameInstance()->GetSubsystem<UGameInstanceModuleManager>()->FindModule("Cartograph");
    Cast<UCartographGameInstanceModule>(Module)->OnWorldLoaded();
}


void UCartographGameWorldModule::BeginDestroy()
{
	Super::BeginDestroy();

    if (const UWorld* World = GetWorld())
    {
        if (const auto* ModuleManager = World->GetGameInstance()->GetSubsystem<UGameInstanceModuleManager>())
        {
            if (UGameInstanceModule* Module = ModuleManager->FindModule("Cartograph"))
            {
                Cast<UCartographGameInstanceModule>(Module)->OnWorldUnloaded();
            }
        }
    }
}
