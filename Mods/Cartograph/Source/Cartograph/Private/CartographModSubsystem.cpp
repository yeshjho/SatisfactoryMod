#include "CartographModSubsystem.h"

#include "GameInstanceModuleManager.h"


ACartographModSubsystem::ACartographModSubsystem()
{
    ReplicationPolicy = ESubsystemReplicationPolicy::SpawnLocal;
    bReplicates = true;
}


void ACartographModSubsystem::BeginDestroy()
{
	Super::BeginDestroy();

    Instance = nullptr;
}


void ACartographModSubsystem::Init()
{
	Super::Init();

    UGameInstanceModule* Module = GetWorld()->GetGameInstance()->GetSubsystem<UGameInstanceModuleManager>()->FindModule("Cartograph");
    GameInstanceModule = Cast<UCartographGameInstanceModule>(Module);

    CARTO_LOG_DEBUG(TEXT("CartographModSubsystem::Init"));

    Instance = this;
}


void ACartographModSubsystem::ClientUpdateBuildingData_Implementation(const TArray<FBuildingData>& AddedBuildings, const TArray<FBuildingData>& RemovedBuildings)
{
    if (HasAuthority())
    {
        CARTO_LOG_DEBUG(TEXT("Sending Update Data"));
        return;
    }

    CARTO_LOG_DEBUG(TEXT("Received Update Data"));

    GameInstanceModule->PendingAddBuildingData.Append(AddedBuildings);
    GameInstanceModule->PendingRemoveBuildingData.Append(RemovedBuildings);

    GameInstanceModule->RedrawMap();
}
