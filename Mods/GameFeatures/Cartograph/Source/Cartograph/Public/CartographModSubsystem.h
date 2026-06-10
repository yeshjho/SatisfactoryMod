#pragma once

#include "Subsystem/ModSubsystem.h"

#include "CoreMinimal.h"

#include "CartographGameInstanceModule.h"

#include "CartographModSubsystem.generated.h"


/**
 * 
 */
UCLASS(Transient)
class CARTOGRAPH_API ACartographModSubsystem : public AModSubsystem
{
	GENERATED_BODY()

    friend class UCartographGameInstanceModule;

public:
	ACartographModSubsystem();

protected:
	virtual void BeginDestroy() override;

protected:
	virtual void Init() override;

private:
	UFUNCTION(NetMulticast, Reliable)
	void ClientUpdateBuildingData(const TArray<FBuildingData>& AddedBuildings, const TArray<FBuildingData>& RemovedBuildings);
    void ClientUpdateBuildingData_Implementation(const TArray<FBuildingData>& AddedBuildings, const TArray<FBuildingData>& RemovedBuildings);


protected:
	inline static ACartographModSubsystem* Instance = nullptr;
};
