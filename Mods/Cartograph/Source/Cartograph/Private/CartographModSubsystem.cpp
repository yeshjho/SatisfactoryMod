#include "CartographModSubsystem.h"


ACartographModSubsystem::ACartographModSubsystem()
{
    ReplicationPolicy = ESubsystemReplicationPolicy::SpawnLocal;
    bReplicates = true;
}


void ACartographModSubsystem::BeginDestroy()
{
	Super::BeginDestroy();

    CARTO_LOG("CartographModSubsystem::BeginDestroy");

    Instance = nullptr;
}


void ACartographModSubsystem::Init()
{
	Super::Init();

    CARTO_LOG("CartographModSubsystem::Init");

    Instance = this;
    UCartographGameInstanceModule::Instance->ShouldInitialize = true;
}


void ACartographModSubsystem::ClientUpdateBuildingData_Implementation(const TArray<FBuildingData>& AddedBuildings, const TArray<FBuildingData>& RemovedBuildings)
{
    if (HasAuthority())
    {
        CARTO_LOG_DEBUG("Sending Update Data");
        return;
    }

    CARTO_LOG_DEBUG("Received Update Data");

    CARTO_LOG_ERROR_RETURN_IF_NULL(UCartographGameInstanceModule::Instance);
    UCartographGameInstanceModule::Instance->PendingAddBuildingData.Append(AddedBuildings);
    UCartographGameInstanceModule::Instance->PendingRemoveBuildingData.Append(RemovedBuildings);

    UCartographGameInstanceModule::Instance->RedrawMap(false);
}
