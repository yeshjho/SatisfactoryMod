// This file has been automatically generated by the Unreal Header Implementation tool

#include "WheeledVehicles/FGTargetPoint.h"

#ifdef DEBUG_SELF_DRIVING
void AFGTargetPoint::Tick(float DeltaSeconds){ }
#endif 
AFGTargetPoint::AFGTargetPoint() : Super() {
	this->mNext = nullptr;
	this->mWaitTime = 3.0;
	this->mTargetSpeed = -1;
	this->mDefaultWaitTime = 3.0;
	this->mIsLoopTarget = false;
	this->mIsDockingTarget = false;
	this->mHostStation = nullptr;
	this->PrimaryActorTick.TickGroup = ETickingGroup::TG_PrePhysics;
	this->PrimaryActorTick.EndTickGroup = ETickingGroup::TG_PrePhysics;
	this->PrimaryActorTick.bTickEvenWhenPaused = false;
	this->PrimaryActorTick.bCanEverTick = true;
	this->PrimaryActorTick.bStartWithTickEnabled = true;
	this->PrimaryActorTick.bAllowTickOnDedicatedServer = true;
	this->PrimaryActorTick.TickInterval = 0.0;
	this->bAlwaysRelevant = true;
	this->SetReplicatingMovement(true);
	this->bReplicates = true;
	this->NetDormancy = ENetDormancy::DORM_Initial;
}
void AFGTargetPoint::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const{ }
void AFGTargetPoint::PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion){ }
bool AFGTargetPoint::ShouldSave_Implementation() const{ return bool(); }
void AFGTargetPoint::StartIsLookedAt_Implementation( AFGCharacterPlayer* byCharacter, const FUseState& state){ }
void AFGTargetPoint::StopIsLookedAt_Implementation( AFGCharacterPlayer* byCharacter, const FUseState& state){ }
void AFGTargetPoint::SetVisibility(bool isVisible){ }
float AFGTargetPoint::GetWaitTime(){ return float(); }
void AFGTargetPoint::IncreaseWaitTime(float addedWaitTime){ }
void AFGTargetPoint::SetWaitTime(float newWaitTime){ }
bool AFGTargetPoint::IsTargetSpeedStill() const{ return bool(); }
void AFGTargetPoint::SetTargetSpeed(int32 newSpeed){ }
void AFGTargetPoint::SetIsLoopTarget(bool isLoopTarget){ }
void AFGTargetPoint::SetIsDockingTarget(bool isDockingTarget){ }
void AFGTargetPoint::SetHostStation( AFGBuildableDockingStation* hostStation){ }
bool AFGTargetPoint::IsTemporary() const{ return bool(); }
void AFGTargetPoint::NotifyIsTemporaryChanged(){ }
void AFGTargetPoint::SetNext(AFGTargetPoint* next){ }
FVector AFGTargetPoint::GetDebugPointLocation(float zOffset) const{ return FVector(); }
void AFGTargetPoint::SetOwningList( AFGDrivingTargetList* owningList){ }
bool AFGTargetPoint::TryClaim( AFGWheeledVehicle* vehicle, TSet< TWeakObjectPtr<  AFGWheeledVehicle > >& blockingVehicles, bool essentialsOnly){ return bool(); }
void AFGTargetPoint::ForceClaim( AFGWheeledVehicle* vehicle, bool essentialsOnly){ }
void AFGTargetPoint::Claim( AFGWheeledVehicle* vehicle, bool recursive, bool essentialsOnly){ }
void AFGTargetPoint::Unclaim( AFGWheeledVehicle* vehicle){ }
void AFGTargetPoint::ForceUnclaim(){ }
void AFGTargetPoint::Unclaim( AFGWheeledVehicle* vehicle, bool recursive){ }
bool AFGTargetPoint::IsLocked(const  AFGWheeledVehicle* vehicle, TSet< TWeakObjectPtr<  AFGWheeledVehicle > >& blockingVehicles) const{ return bool(); }
bool AFGTargetPoint::IsLockedByDocking(const  AFGWheeledVehicle* vehicle) const{ return bool(); }
void AFGTargetPoint::DrawTargetDebug( AFGWheeledVehicle* claimant, bool drawSearchPoints, bool drawSmall){ }
int AFGTargetPoint::GetDebugLevel() const{ return int(); }
void AFGTargetPoint::FindBlockingVehicles( AFGWheeledVehicle* blockedVehicle, TSet< const  AFGWheeledVehicle* >& blockingVehicles) const{ }
void AFGTargetPoint::BeginPlay(){ }
void AFGTargetPoint::EndPlay(const EEndPlayReason::Type EndPlayReason){ }
void AFGTargetPoint::OnRep_IsLoopTarget(){ }
void AFGTargetPoint::OnRep_IsDockingTarget(){ }
void AFGTargetPoint::OnRep_TargetSpeed(){ }
void AFGTargetPoint::OnRep_HostStation(){ }
void AFGTargetPoint::UpdateDockingTarget(){ }
