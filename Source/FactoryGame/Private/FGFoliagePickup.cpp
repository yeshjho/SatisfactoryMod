// This file has been automatically generated by the Unreal Header Implementation tool

#include "FGFoliagePickup.h"

AFGFoliagePickup::AFGFoliagePickup() : Super() {
	this->mPickupRepeatInterval = 0.5;
	this->mAutoPickUpToggleDelay = 0.5;
	this->mPickupMesh = nullptr;
	this->PrimaryActorTick.TickGroup = ETickingGroup::TG_PrePhysics;
	this->PrimaryActorTick.EndTickGroup = ETickingGroup::TG_PrePhysics;
	this->PrimaryActorTick.bTickEvenWhenPaused = false;
	this->PrimaryActorTick.bCanEverTick = true;
	this->PrimaryActorTick.bStartWithTickEnabled = true;
	this->PrimaryActorTick.bAllowTickOnDedicatedServer = true;
	this->PrimaryActorTick.TickInterval = 0.0;
	this->bOnlyRelevantToOwner = true;
	this->SetHidden(true);
	this->bNetUseOwnerRelevancy = true;
	this->bReplicates = true;
}
void AFGFoliagePickup::BeginPlay(){ Super::BeginPlay(); }
void AFGFoliagePickup::Tick(float DeltaSeconds){ Super::Tick(DeltaSeconds); }
void AFGFoliagePickup::UpdateUseState_Implementation( AFGCharacterPlayer* byCharacter, const FVector& atLocation,  UPrimitiveComponent* componentHit, FUseState& out_useState){ }
bool AFGFoliagePickup::IsUseable_Implementation() const{ return bool(); }
void AFGFoliagePickup::StartIsLookedAt_Implementation( AFGCharacterPlayer* byCharacter, const FUseState& state){ }
FText AFGFoliagePickup::GetLookAtDecription_Implementation( AFGCharacterPlayer* byCharacter, const FUseState& state) const{ return FText(); }
void AFGFoliagePickup::StopIsLookedAt_Implementation( AFGCharacterPlayer* byCharacter, const FUseState& state){ }
void AFGFoliagePickup::BroadcastPickup_Implementation( UStaticMesh* fromStaticMesh, FVector atLocation){ }
void AFGFoliagePickup::SetPickupData( UHierarchicalInstancedStaticMeshComponent* component, int32 instanceId, AFGCharacterPlayer* byCharacter){ }
AFGCharacterPlayer* AFGFoliagePickup::GetOwnerCharacter() const{ return nullptr; }
bool AFGFoliagePickup::CanPickUpFoliageCurrently() const{ return bool(); }
void AFGFoliagePickup::DoPickup(){ }
void AFGFoliagePickup::Input_Use(const  FInputActionValue& actionValue){ }
void AFGFoliagePickup::Input_ToggleAutoPickup_Started(){ }
void AFGFoliagePickup::Input_ToggleAutoPickup_Triggered(){ }
void AFGFoliagePickup::Input_ToggleAutoPickup_Canceled(){ }
void AFGFoliagePickup::ToggleAutoPickUp(){ }
void AFGFoliagePickup::OnAutoPickUpToggleDelayCompleted(){ }
void AFGFoliagePickup::Server_PickUpFoliage_Implementation( AFGCharacterPlayer* byCharacter, FFoliageInstanceStableId StableId, const FVector& instanceLocation){ }
bool AFGFoliagePickup::AddToPlayerInventory( AFGCharacterPlayer* character,  UHierarchicalInstancedStaticMeshComponent* meshComponent, uint32 seed){ return bool(); }
bool AFGFoliagePickup::HasPlayerSpaceFor( AFGCharacterPlayer* character,  UHierarchicalInstancedStaticMeshComponent* meshComponent, uint32 seed){ return bool(); }
