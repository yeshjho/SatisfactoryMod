// This file has been automatically generated by the Unreal Header Implementation tool

#include "FGConveyorChainSubsystem.h"
#include "Net/UnrealNetwork.h"

bool FConveyorChainItemRPCData::NetSerialize(FArchive& Ar,  UPackageMap* Map, bool& bOutSuccess){ return bool(); }
bool FConveyorChainSegmentRPCData::NetSerialize(FArchive& Ar,  UPackageMap* Map, bool& bOutSuccess){ return bool(); }
void UFGConveyorChainRemoteCallObject::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UFGConveyorChainRemoteCallObject, mForceNetField_UFGConveyorChainRemoteCallObject);
}
void UFGConveyorChainRemoteCallObject::Server_RequestChainItemUpdate_Implementation(const FConveyorChainItemRPCData& chainData){ }
void UFGConveyorChainRemoteCallObject::Client_RespondChainItemUpdate_Implementation(const FConveyorChainItemRPCData& chainData){ }
void UFGConveyorChainRemoteCallObject::Server_RequestChainSegmentData_Implementation(const FConveyorChainSegmentRPCData& segmentData){ }
void UFGConveyorChainRemoteCallObject::Client_RespondChainSegmentData_Implementation(const FConveyorChainSegmentRPCData& segmentData){ }
AFGConveyorChainSubsystem::AFGConveyorChainSubsystem() : Super() {
	this->mReparamStepsPerSegment = 10;
	this->mServerFactoryTickTime = 0.0;
	this->mClientFactoryTickTime = 0.0;
	this->mClientTickDebt = 0.0;
	this->PrimaryActorTick.TickGroup = ETickingGroup::TG_PrePhysics;
	this->PrimaryActorTick.EndTickGroup = ETickingGroup::TG_PrePhysics;
	this->PrimaryActorTick.bTickEvenWhenPaused = false;
	this->PrimaryActorTick.bCanEverTick = true;
	this->PrimaryActorTick.bStartWithTickEnabled = true;
	this->PrimaryActorTick.bAllowTickOnDedicatedServer = true;
	this->PrimaryActorTick.TickInterval = 0.1;
	this->NetUpdateFrequency = 1000.0;
}
AFGConveyorChainSubsystem* AFGConveyorChainSubsystem::Get(UWorld* world){ return nullptr; }
AFGConveyorChainSubsystem* AFGConveyorChainSubsystem::Get(UObject* worldContext){ return nullptr; }
void AFGConveyorChainSubsystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFGConveyorChainSubsystem, mServerFactoryTickTime);
	DOREPLIFETIME(AFGConveyorChainSubsystem, mAllItemDescriptors);
}
void AFGConveyorChainSubsystem::BeginPlay(){ }
void AFGConveyorChainSubsystem::Tick(float DeltaSeconds){ }
void AFGConveyorChainSubsystem::OnConveyorItemMovementQualityUpdated(FString cvar){ }
void AFGConveyorChainSubsystem::AddConveyorChain(AFGConveyorChainActor* chainActor){ }
void AFGConveyorChainSubsystem::RemoveConveyorChain(AFGConveyorChainActor* chainActor){ }
void AFGConveyorChainSubsystem::BuildItemDescriptorRepArray(){ }
void AFGConveyorChainSubsystem::OnRep_ConveyorItemDescAndID(){ }
void AFGConveyorChainSubsystem::NotifyChainReceivedItemUpdate(AFGConveyorChainActor* chainActor){ }
void AFGConveyorChainSubsystem::NotifyChainNeedsSegmentUpdate(AFGConveyorChainActor* ChainActor){ }
void AFGConveyorChainSubsystem::NotifyChainReceiveSegmentUpdate(AFGConveyorChainActor* chainActor){ }
void AFGConveyorChainSubsystem::NotifyChainNeedsItemUpdate(AFGConveyorChainActor* ChainActor){ }
void AFGConveyorChainSubsystem::OnRep_ServerFactoryTickTime(float oldValue){ }
float AFGConveyorChainSubsystem::GetAndConsumeClientTimeDebt(float updateDelta){ return float(); }
float AFGConveyorChainSubsystem::GetAverageServerTickDeltaTime(){ return float(); }
bool AFGConveyorChainSubsystem::IsUnusuallyLargeTickDelta(){ return bool(); }