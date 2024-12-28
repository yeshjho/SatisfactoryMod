#include "CartographRemoteCallObject.h"

#include "UnrealNetwork.h"

#include "GameInstanceModuleManager.h"

#include "CartographGameInstanceModule.h"


void UCartographRemoteCallObject::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UCartographRemoteCallObject, bDummy);
}


void UCartographRemoteCallObject::ServerRequestInitialBuildingData_Implementation(APlayerController* PlayerController, EInitialDataSendPhase SendPhase)
{
    constexpr int Slice = std::numeric_limits<uint16_t>::max() / sizeof(FBuildingData) * 0.9;

    if (!GameInstanceModule)
    {
        UGameInstanceModule* Module = GetWorld()->GetGameInstance()->GetSubsystem<UGameInstanceModuleManager>()->FindModule("Cartograph");
        GameInstanceModule = Cast<UCartographGameInstanceModule>(Module);
    }

    switch (SendPhase)
    {
    case EInitialDataSendPhase::Initial:
        InitialBuildingDataToSendPerPlayer.Add(PlayerController, FInitialBuildingDataToSend{
            .InitialBuildingData = GameInstanceModule->CurrentBuildingData,
            .Slices = GameInstanceModule->CurrentBuildingData.Num() / Slice + 1,
            .LastSentSlice = -1,
        });
        break;

    case EInitialDataSendPhase::Normal:
        break;

    case EInitialDataSendPhase::Finished:
        InitialBuildingDataToSendPerPlayer.Remove(PlayerController);
        return;
    }

    auto& [InitialBuildingData, Slices, LastSentSlice] = *InitialBuildingDataToSendPerPlayer.Find(PlayerController);

    const int i = ++LastSentSlice;
    const int Start = i * Slice;
    const int End = FMath::Min((i + 1) * Slice, InitialBuildingData.Num());
    CARTO_LOG_DEBUG(TEXT("Sending Initial Data (%d/%d)"), i + 1, Slices);
    ClientReceiveInitialBuildingData(TArray<FBuildingData>{ 
			InitialBuildingData.GetData() + Start,
    		End - Start
		},
        i == Slices - 1);
}


void UCartographRemoteCallObject::ClientReceiveInitialBuildingData_Implementation(const TArray<FBuildingData>& Array, bool IsLast)
{
    if (!GameInstanceModule)
    {
        UGameInstanceModule* Module = GetWorld()->GetGameInstance()->GetSubsystem<UGameInstanceModuleManager>()->FindModule("Cartograph");
        GameInstanceModule = Cast<UCartographGameInstanceModule>(Module);
    }

    CARTO_LOG_DEBUG(TEXT("Received Initial Data"));
    GameInstanceModule->CurrentBuildingData.Append(Array);

    if (IsLast)
    {
        CARTO_LOG_DEBUG(TEXT("Was Last. Redraw Map"));
        GameInstanceModule->IsInitializing = false;
        GameInstanceModule->RedrawMap();
    }

    ServerRequestInitialBuildingData(GetWorld()->GetFirstPlayerController(), 
        IsLast ? EInitialDataSendPhase::Finished : EInitialDataSendPhase::Normal);
}
