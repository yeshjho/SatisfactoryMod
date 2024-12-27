#include "CartographRemoteCallObject.h"

#include "UnrealNetwork.h"

#include "GameInstanceModuleManager.h"

#include "CartographGameInstanceModule.h"


void UCartographRemoteCallObject::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UCartographRemoteCallObject, bDummy);
}


void UCartographRemoteCallObject::ServerRequestInitialBuildingData_Implementation()
{
    if (!GameInstanceModule)
    {
        UGameInstanceModule* Module = GetWorld()->GetGameInstance()->GetSubsystem<UGameInstanceModuleManager>()->FindModule("Cartograph");
        GameInstanceModule = Cast<UCartographGameInstanceModule>(Module);
    }

    GameInstanceModule->IsInitializing = true;
    SendInitialBuildingData(GameInstanceModule->CurrentBuildingData, FForceLatentCoroutine{});
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
}


UE5Coro::TCoroutine<> UCartographRemoteCallObject::SendInitialBuildingData(TArray<FBuildingData> BuildingData, FForceLatentCoroutine)
{
    const APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    UNetConnection* Connection = PlayerController->GetNetConnection();
    const int Slice = Connection->GetMaxSingleBunchSizeBits() / sizeof(FBuildingData);

    const int Count = GameInstanceModule->CurrentBuildingData.Num();
    const int Slices = Count / Slice + 1;

    for (int i = 0; i < Slices; i++)
    {
        CARTO_LOG_DEBUG(TEXT("Sending Initial Data"));

        const int Start = i * Slice;
        const int End = FMath::Min((i + 1) * Slice, Count);
        ClientReceiveInitialBuildingData(TArray<FBuildingData>{ GameInstanceModule->CurrentBuildingData.GetData() + Start, End - Start }, i == Slices - 1);

        co_await UE5Coro::Latent::Until(
            [Connection]()
            {
                const int OutgoingBunchesNum = Connection->GetOutgoingBunches().Num() + 1;
                // DataChannel.cpp: 1330
                return !(OutgoingBunchesNum >= 8/*GCVarNetPartialBunchReliableThreshold*/ && !Connection->IsInternalAck());
            });
    }
}
