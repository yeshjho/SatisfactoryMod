#pragma once

#include "FGRemoteCallObject.h"

#include "CoreMinimal.h"

#include "UE5Coro/UE5Coro.h"

#include "CartographGameInstanceModule.h"

#include "CartographRemoteCallObject.generated.h"


/**
 * 
 */
UCLASS()
class CARTOGRAPH_API UCartographRemoteCallObject : public UFGRemoteCallObject
{
	GENERATED_BODY()

    friend class UCartographGameInstanceModule;

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION(Server, Reliable)
	void ServerRequestInitialBuildingData();
	void ServerRequestInitialBuildingData_Implementation();

	UFUNCTION(Client, Reliable)
	void ClientReceiveInitialBuildingData(const TArray<FBuildingData>& Array, bool IsLast);
	void ClientReceiveInitialBuildingData_Implementation(const TArray<FBuildingData>& Array, bool IsLast);

	UE5Coro::TCoroutine<> SendInitialBuildingData(TArray<FBuildingData> BuildingData, FForceLatentCoroutine = {});


protected:
	UPROPERTY(Replicated)
	bool bDummy = true;

    UCartographGameInstanceModule* GameInstanceModule = nullptr;
};
