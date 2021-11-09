// This file has been automatically generated by the Unreal Header Implementation tool

#include "Hologram/FGPipelineSupportHologram.h"
#include "Components/SceneComponent.h"

AFGPipelineSupportHologram::AFGPipelineSupportHologram() : Super() {
	this->mSupportMesh.Mesh = nullptr;
	this->mSupportMesh.Height = 0.0;
	this->mCanAdjustVerticalAngle = true;
	this->mSnapConnection = nullptr;
	this->mVerticalAngle = 0.0;
	this->mSupportLength = 0.0;
	this->mSupportMeshComponent = nullptr;
	this->mSupportTopPartMeshComponent = nullptr;
	this->mSupportLengthComponent = nullptr;
	this->mInstancedMeshComponent = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Instanced Mesh Component"));
	this->mMaxZoopAmount = 9;
	this->mBuildModeZoop = nullptr;
	this->mClearanceExtent.X = 0.0;
	this->mClearanceExtent.Y = 0.0;
	this->mClearanceExtent.Z = 0.0;
	this->mUseGradualFoundationRotations = true;
	this->mInstancedMeshComponent->SetupAttachment(RootComponent);
}
void AFGPipelineSupportHologram::BeginPlay(){ }
bool AFGPipelineSupportHologram::DoMultiStepPlacement(bool isInputFromARelease){ return bool(); }
bool AFGPipelineSupportHologram::IsValidHitResult(const FHitResult& hitResult) const{ return bool(); }
bool AFGPipelineSupportHologram::TrySnapToActor(const FHitResult& hitResult){ return bool(); }
void AFGPipelineSupportHologram::SetHologramLocationAndRotation(const FHitResult& hitResult){ }
AActor* AFGPipelineSupportHologram::Construct(TArray<AActor*>& out_children, FNetConstructionID constructionID){ return nullptr; }
void AFGPipelineSupportHologram::GetSupportedBuildModes_Implementation(TArray< TSubclassOf< UFGHologramBuildModeDescriptor > >& out_buildmodes) const{ }
void AFGPipelineSupportHologram::OnBuildModeChanged(){ }
void AFGPipelineSupportHologram::SerializeConstructMessage(FArchive& ar, FNetConstructionID id){ }
void AFGPipelineSupportHologram::SetSupportLength(float height){ }
void AFGPipelineSupportHologram::SnapToConnection(UFGPipeConnectionComponentBase* connection,  AFGPipelineHologram* parentPipeline){ }
void AFGPipelineSupportHologram::ResetBuildSteps(){ }
void AFGPipelineSupportHologram::ResetVerticalRotation(){ }
void AFGPipelineSupportHologram::UpdateSupportLengthRelativeLoc(){ }
void AFGPipelineSupportHologram::Scroll(int32 delta){ }
void AFGPipelineSupportHologram::ConfigureActor( AFGBuildable* inBuildable) const{ }
void AFGPipelineSupportHologram::CheckValidPlacement(){ }
void AFGPipelineSupportHologram::OnRep_SupportMesh(){ }
void AFGPipelineSupportHologram::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> & OutLifetimeProps) const { Super::GetLifetimeReplicatedProps(OutLifetimeProps); }
