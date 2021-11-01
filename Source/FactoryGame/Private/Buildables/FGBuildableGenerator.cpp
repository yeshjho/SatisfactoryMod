// This file has been automatically generated by the Unreal Header Implementation tool

#include "Buildables/FGBuildableGenerator.h"
#include "Hologram/FGFactoryHologram.h"
#include "FGPowerInfoComponent.h"
#include "Components/SceneComponent.h"

void AFGBuildableGenerator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const{ }
void AFGBuildableGenerator::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker){ }
AFGBuildableGenerator::AFGBuildableGenerator() : Super() {
	this->mPowerProduction = 0.0;
	this->mPowerProductionExponent = 1.3;
	this->mLoadPercentage = 0.0;
}
void AFGBuildableGenerator::BeginPlay(){ }
bool AFGBuildableGenerator::CanProduce_Implementation() const{ return bool(); }
bool AFGBuildableGenerator::Factory_HasPower() const{ return bool(); }
EProductionStatus AFGBuildableGenerator::GetProductionIndicatorStatus() const{ return EProductionStatus(); }
float AFGBuildableGenerator::GetPowerProductionCapacity() const{ return float(); }
float AFGBuildableGenerator::GetDefaultPowerProductionCapacity() const{ return float(); }
float AFGBuildableGenerator::CalcPowerProductionCapacityForPotential(float potential) const{ return float(); }
void AFGBuildableGenerator::SetActorHiddenInGame(bool bNewHidden){ }
bool AFGBuildableGenerator::CanStartPowerProduction_Implementation() const{ return bool(); }
void AFGBuildableGenerator::Factory_TickProducing(float dt){ }
void AFGBuildableGenerator::Factory_StartProducing(){ }
void AFGBuildableGenerator::Factory_StopProducing(){ }
void AFGBuildableGenerator::Factory_StartPowerProduction_Implementation(){ }
void AFGBuildableGenerator::Factory_StopPowerProduction_Implementation(){ }
void AFGBuildableGenerator::Factory_TickPowerProduction_Implementation(float dt){ }
