// This file has been automatically generated by the Unreal Header Implementation tool

#include "Buildables/FGBuildableGeneratorNuclear.h"
#include "Hologram/FGFactoryHologram.h"
#include "Replication/FGReplicationDetailInventoryComponent.h"
#include "FGPowerInfoComponent.h"
#include "Components/SceneComponent.h"

AFGBuildableGeneratorNuclear::AFGBuildableGeneratorNuclear() : Super() {
	this->mOutputInventory = nullptr;
	this->mOutputInventoryHandler = CreateDefaultSubobject<UFGReplicationDetailInventoryComponent>(TEXT("WasteInventoryHandler"));
	this->mWasteLeftFromCurrentFuel = 0;
	this->mCurrentGeneratorNuclearWarning = EGeneratorNuclearWarning::GNW_None;
}
void AFGBuildableGeneratorNuclear::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const{ }
void AFGBuildableGeneratorNuclear::BeginPlay(){ }
void AFGBuildableGeneratorNuclear::Factory_Tick(float dt){ }
void AFGBuildableGeneratorNuclear::LoadFuel(){ }
bool AFGBuildableGeneratorNuclear::Factory_HasPower() const{ return bool(); }
bool AFGBuildableGeneratorNuclear::CanStartPowerProduction_Implementation() const{ return bool(); }
void AFGBuildableGeneratorNuclear::OnReplicationDetailActorRemoved(){ }
bool AFGBuildableGeneratorNuclear::IsWasteFull() const{ return bool(); }
bool AFGBuildableGeneratorNuclear::CanLoadFuel() const{ return bool(); }
void AFGBuildableGeneratorNuclear::TryProduceWaste(){ }
void AFGBuildableGeneratorNuclear::OnRep_ReplicationDetailActor(){ }
AFGReplicationDetailActor_GeneratorNuclear* AFGBuildableGeneratorNuclear::GetCastRepDetailsActor() const{ return nullptr; }
bool AFGBuildableGeneratorNuclear::CanFitWasteOfNextFuelClass() const{ return bool(); }
