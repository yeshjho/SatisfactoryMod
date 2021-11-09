// This file has been automatically generated by the Unreal Header Implementation tool

#include "Buildables/FGBuildablePipelineAttachment.h"
#include "Hologram/FGPipelineAttachmentHologram.h"

AFGBuildablePipelineAttachment::AFGBuildablePipelineAttachment() : Super() {
	this->mRadius = 75.0;
	this->mFluidBoxVolume = 5.0;
	this->mCachedFluidDescriptor = nullptr;
	this->mAddToSignificanceManager = false;
	this->mHologramClass = AFGPipelineAttachmentHologram::StaticClass();
	this->mFactoryTickFunction.TickGroup = ETickingGroup::TG_PrePhysics;
	this->mFactoryTickFunction.EndTickGroup = ETickingGroup::TG_PrePhysics;
	this->mFactoryTickFunction.bTickEvenWhenPaused = false;
	this->mFactoryTickFunction.bCanEverTick = false;
	this->mFactoryTickFunction.bStartWithTickEnabled = true;
	this->mFactoryTickFunction.bAllowTickOnDedicatedServer = true;
	this->mFactoryTickFunction.TickInterval = 0.0;
}
void AFGBuildablePipelineAttachment::BeginPlay(){ }
void AFGBuildablePipelineAttachment::EndPlay(const EEndPlayReason::Type endPlayReason){ }
void AFGBuildablePipelineAttachment::Dismantle_Implementation(){ }
FFluidBox* AFGBuildablePipelineAttachment::GetFluidBox(){ return nullptr; }
TArray< class UFGPipeConnectionComponent* > AFGBuildablePipelineAttachment::GetPipeConnections(){ return TArray<class UFGPipeConnectionComponent*>(); }
