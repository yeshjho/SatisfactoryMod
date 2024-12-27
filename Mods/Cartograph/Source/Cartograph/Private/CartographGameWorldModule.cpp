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
