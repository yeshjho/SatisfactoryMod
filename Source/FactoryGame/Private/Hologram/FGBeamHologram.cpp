// This file has been automatically generated by the Unreal Header Implementation tool

#include "Hologram/FGBeamHologram.h"
#include "Net/UnrealNetwork.h"

AFGBeamHologram::AFGBeamHologram() : Super() {
	this->mBuildModeDiagonal = nullptr;
	this->mBuildModeFreeForm = nullptr;
	this->mCostMultiplier = 1.0;
	this->mBeamMesh = nullptr;
	this->mCurrentLength = 0.0;
	this->mBuildStep = EBeamHologramBuildStep::BHBS_Placement;
	this->mNeedsValidFloor = false;
	this->mAllowEdgePlacementInDesignerEvenOnIntersect = true;
}
void AFGBeamHologram::BeginPlay(){ Super::BeginPlay(); }
void AFGBeamHologram::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFGBeamHologram, mCurrentLength);
	DOREPLIFETIME(AFGBeamHologram, mBuildStep);
}
bool AFGBeamHologram::IsValidHitResult(const FHitResult& hitResult) const{ return bool(); }
bool AFGBeamHologram::TrySnapToActor(const FHitResult& hitResult){ return bool(); }
void AFGBeamHologram::SetHologramLocationAndRotation(const FHitResult& hitResult){ }
bool AFGBeamHologram::DoMultiStepPlacement(bool isInputFromARelease){ return bool(); }
int32 AFGBeamHologram::GetRotationStep() const{ return int32(); }
void AFGBeamHologram::GetSupportedBuildModes_Implementation(TArray< TSubclassOf<UFGBuildGunModeDescriptor> >& out_buildmodes) const{ }
void AFGBeamHologram::ConfigureActor(AFGBuildable* inBuildable) const{ }
int32 AFGBeamHologram::GetBaseCostMultiplier() const{ return int32(); }
bool AFGBeamHologram::CanBeZooped() const{ return bool(); }
bool AFGBeamHologram::CanIntersectWithDesigner(AFGBuildableBlueprintDesigner* designer) const{ return bool(); }
void AFGBeamHologram::OnRep_CurrentLength(){ }
bool AFGBeamHologram::IsHologramIdenticalToActor(AActor* actor, const FVector& hologramLocationOffset) const{ return bool(); }
void AFGBeamHologram::CreateAttachmentPointTransform(FTransform& out_transformResult, const FHitResult& HitResult, AFGBuildable* pBuildable, const FFGAttachmentPoint& BuildablePoint, const FFGAttachmentPoint& LocalPoint){ }
void AFGBeamHologram::CreateVerticalBeam(const FHitResult& hitResult, bool allowDiagonal){ }
void AFGBeamHologram::CreateFreeformBeam(const FHitResult& hitResult){ }
void AFGBeamHologram::SetCurrentLength(float newLength){ }
