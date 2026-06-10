#pragma once

#include "FGRemoteCallObject.h"

#include "CoreMinimal.h"
#include "BufferWriter.h"

#include "CartographGameInstanceModule.h"

#include "CartographRemoteCallObject.generated.h"


struct FInitialBuildingDataToSend
{
	FBufferWriter InitialBuildingData;
	int64_t Slices;
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


// We'll take up all the bandwidth if this value is not small enough,
// resulting the client not being able to do anything while initializing.
constexpr int BuildingDataBufferMaxSize = std::numeric_limits<uint16_t>::max() / 32;


USTRUCT()
struct FBuildingDataBuffer
{
	GENERATED_BODY()

    uint8 Data[BuildingDataBufferMaxSize];

	bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess);
};


template<>
struct TStructOpsTypeTraits<FBuildingDataBuffer> : public TStructOpsTypeTraitsBase2<FBuildingDataBuffer>
{
	enum
	{
		WithNetSerializer = true
	};
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
	void ClientReceiveInitialBuildingData(const FBuildingDataBuffer& Array, int16 Size, int16 TotalSliceCount);
	void ClientReceiveInitialBuildingData_Implementation(const FBuildingDataBuffer& Array, int16 Size, int16 TotalSliceCount);


	UE5Coro::TCoroutine<> InitialBuildableDeserialize(FForceLatentCoroutine = {});


protected:
	UPROPERTY(Replicated)
	bool bDummy = true;

	// Server side
	TMap<APlayerController*, FInitialBuildingDataToSend> InitialBuildingDataToSendPerPlayer;

	// Client side
	int16 ReceivedSliceCount = 0;
    TArray<uint8> Buffer;
};
