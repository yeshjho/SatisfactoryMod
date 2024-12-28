#pragma once

#include "FGRemoteCallObject.h"

#include "CoreMinimal.h"

#include "UE5Coro/UE5Coro.h"

#include "CartographGameInstanceModule.h"

#include "CartographRemoteCallObject.generated.h"


struct FInitialBuildingDataToSend
{
    TArray<FBuildingData> InitialBuildingData;
	int Slices;
	int LastSentSlice;
    FTimerHandle TimerHandle;
};


UENUM()
enum class EInitialDataSendPhase
{
	Initial,
	Normal,
	Finished
};


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
	void ServerRequestInitialBuildingData(APlayerController* PlayerController, EInitialDataSendPhase SendPhase);
	void ServerRequestInitialBuildingData_Implementation(APlayerController* PlayerController, EInitialDataSendPhase SendPhase);

	UFUNCTION(Client, Reliable)
	void ClientReceiveInitialBuildingData(const TArray<FBuildingData>& Array, bool IsLast);
	void ClientReceiveInitialBuildingData_Implementation(const TArray<FBuildingData>& Array, bool IsLast);

	UE5Coro::TCoroutine<> SendInitialBuildingData(TArray<FBuildingData> BuildingData, FForceLatentCoroutine = {});


protected:
	UPROPERTY(Replicated)
	bool bDummy = true;

	TMap<APlayerController*, FInitialBuildingDataToSend> InitialBuildingDataToSendPerPlayer;

    UCartographGameInstanceModule* GameInstanceModule = nullptr;
};
