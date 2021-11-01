// This file has been automatically generated by the Unreal Header Implementation tool

#include "Buildables/FGBuildableResourceExtractorBase.h"
#include "FGConstructDisqualifier.h"
#include "FGPowerInfoComponent.h"
#include "Components/SceneComponent.h"
#include "Hologram/FGResourceExtractorHologram.h"

void AFGBuildableResourceExtractorBase::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const{ }
void AFGBuildableResourceExtractorBase::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker){ }
AFGBuildableResourceExtractorBase::AFGBuildableResourceExtractorBase() : Super() {
	this->mRestrictToNodeType = nullptr;
	this->mOnlyAllowCertainResources = false;
	this->mMustPlaceOnResourceDisqualifier = UFGCDNeedsResourceNode::StaticClass();
	this->mExtractorTypeName = TEXT("None");
	this->mExtractResourceNode = nullptr;
	this->mExtractableResource = nullptr;
	this->mCanChangePotential = true;
	this->mHologramClass = AFGResourceExtractorHologram::StaticClass();
}
void AFGBuildableResourceExtractorBase::BeginPlay(){ }
void AFGBuildableResourceExtractorBase::Destroyed(){ }
bool AFGBuildableResourceExtractorBase::DisconnectExtractableResource(){ return bool(); }
void AFGBuildableResourceExtractorBase::PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion){ }
void AFGBuildableResourceExtractorBase::SetExtractableResource(TScriptInterface< IFGExtractableResourceInterface > extractableInterface){ }
void AFGBuildableResourceExtractorBase::SetResourceNode( AFGResourceNode* resourceNode){ }
UParticleSystem* AFGBuildableResourceExtractorBase::GetMiningParticle(){ return nullptr; }
bool AFGBuildableResourceExtractorBase::CanOccupyResource(const TScriptInterface<  IFGExtractableResourceInterface >& resource) const{ return bool(); }
bool AFGBuildableResourceExtractorBase::IsAllowedOnResource(const TScriptInterface<  IFGExtractableResourceInterface >& resource) const{ return bool(); }
void AFGBuildableResourceExtractorBase::OnExtractableResourceSet(){ }
