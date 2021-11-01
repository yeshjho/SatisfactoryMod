// This file has been automatically generated by the Unreal Header Implementation tool

#include "Equipment/FGHoverPack.h"
#include "FGPowerInfoComponent.h"
#include "Components/SceneComponent.h"
#include "FGPowerConnectionComponent.h"
#include "Equipment/FGEquipment.h"

AFGHoverPack::AFGHoverPack() : Super() {
	this->mHoverSpeed = 800.0;
	this->mHoverAccelerationSpeed = 2000.0;
	this->mHoverSprintMultiplier = 2.0;
	this->mRailRoadSurfSpeed = 2500.0;
	this->mRailroadSurfSensitivity = 0.1;
	this->mHoverFriction = 0.99;
	this->mJumpKeyHoldActivationTime = 0.3;
	this->mFallSpeedLimitWhenPowered = 300.0;
	this->mPowerConnectionSearchRadius = 3000.0;
	this->mPowerConnectionSearchTickRate = 0.1;
	this->mPowerConnectionDisconnectionTime = 0.5;
	this->mPowerCapacity = 3.0;
	this->mPowerDrainRate = 0.5;
	this->mPowerConsumption = 0.0;
	this->mCurrentPowerLevel = 0.0;
	this->mRangeWarningNormalizedDistanceThreshold = 0.75;
	this->mCurrentHoverMode = EHoverPackMode::HPM_Inactive;
	this->mCurrentPowerConnection = nullptr;
	this->mHasConnection = false;
	this->mShouldAutomaticallyHoverWhenConnected = false;
	this->mCrouchHoverCancelTime = 0.3;
	this->mCharacterUseDistanceWhenActive = 0.0;
	this->mCurrentConnectionLocation.X = 0.0;
	this->mCurrentConnectionLocation.Y = 0.0;
	this->mCurrentConnectionLocation.Z = 0.0;
	this->mCurrentRailroadTrack = nullptr;
	this->mPowerInfo = CreateDefaultSubobject<UFGPowerInfoComponent>(TEXT("Power Info"));
	this->mPowerConnection = CreateDefaultSubobject<UFGPowerConnectionComponent>(TEXT("Power Connection"));
	this->mEquipmentSlot = EEquipmentSlot::ES_BACK;
	this->mBackAnimation = EBackEquipment::BE_Jetpack;
	this->RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	this->mPowerConnection->SetupAttachment(RootComponent);
}
void AFGHoverPack::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const{ }
void AFGHoverPack::BeginPlay(){ }
void AFGHoverPack::Tick(float deltaTime){ }
void AFGHoverPack::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker){ }
void AFGHoverPack::Equip( AFGCharacterPlayer* character){ }
void AFGHoverPack::UnEquip(){ }
void AFGHoverPack::OnCharacterMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode, EMovementMode NewMovementMode, uint8 NewCustomMode){ }
void AFGHoverPack::AddEquipmentActionBindings(){ }
float AFGHoverPack::GetCharacterUseDistanceOverride() const{ return float(); }
float AFGHoverPack::GetMaxSpeed(bool IsSprinting) const{ return float(); }
float AFGHoverPack::GetHoverSpeed(bool IsSprinting) const{ return float(); }
float AFGHoverPack::GetHoverAccelerationSpeed(bool IsSprinting) const{ return float(); }
float AFGHoverPack::GetNormalizedDistanceFromConnection() const{ return float(); }
float AFGHoverPack::GetDistanceFromCurrentConnection() const{ return float(); }
float AFGHoverPack::GetMaxDistanceFromCurrentConnection() const{ return float(); }
float AFGHoverPack::GetHeightAboveCurrentConnection() const{ return float(); }
void AFGHoverPack::SetHoverMode(EHoverPackMode HoverMode, bool UpdateMovementMode){ }
void AFGHoverPack::ConnectToNearestPowerConnection(){ }
EHoverConnectionStatus AFGHoverPack::GetPowerConnectionStatus( UFGPowerConnectionComponent* Connection) const{ return EHoverConnectionStatus(); }
bool AFGHoverPack::IsPowerConnectionValid( UFGPowerConnectionComponent* Connection, bool CheckDistance) const{ return bool(); }
bool AFGHoverPack::IsRailroadTrackValid( AFGBuildableRailroadTrack* RailroadTrack, bool CheckDistance) const{ return bool(); }
void AFGHoverPack::OnHoverModeChanged_Implementation(EHoverPackMode NewMode){ }
void AFGHoverPack::OnPowerConnectionLocationUpdated_Implementation(const FVector& NewLocation){ }
void AFGHoverPack::OnConnectionStatusUpdated_Implementation(const bool HasConnection){ }
void AFGHoverPack::OnRangeWarningToggle_Implementation(const bool ShouldDisplayWarning){ }
void AFGHoverPack::HandlePowerConnection(const float DeltaTime){ }
void AFGHoverPack::ConnectToPowerConnection( UFGPowerConnectionComponent* Connection){ }
void AFGHoverPack::DisconnectFromCurrentPowerConnection(){ }
void AFGHoverPack::ConnectToRailroadTrack( AFGBuildableRailroadTrack* RailroadTrack){ }
void AFGHoverPack::SetCharacterHoverMovementMode() const{ }
void AFGHoverPack::OnCrouchPressed(){ }
void AFGHoverPack::PlayerStopHover_Server_Implementation(){ }
bool AFGHoverPack::PlayerStopHover_Server_Validate(){ return bool(); }
void AFGHoverPack::PlayerStopHover(){ }
bool AFGHoverPack::PlayerIsInHoverMovementMode() const{ return bool(); }
void AFGHoverPack::OnRep_HasConnection(){ }
void AFGHoverPack::OnRep_CurrentHoverMode(){ }
void AFGHoverPack::OnRep_CurrentConnectionLocation(){ }
AFGHoverPackAttachment::AFGHoverPackAttachment() : Super() {
	this->mCurrentHoverMode = EHoverPackMode::HPM_Inactive;
	this->mHasConnection = false;
	this->mCurrentConnectionLocation.X = 0.0;
	this->mCurrentConnectionLocation.Y = 0.0;
	this->mCurrentConnectionLocation.Z = 0.0;
	this->mCurrentRailroadTrack = nullptr;
}
void AFGHoverPackAttachment::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const{ }
void AFGHoverPackAttachment::SetCurrentHoverMode(EHoverPackMode NewMode){ }
void AFGHoverPackAttachment::SetConnectionStatus(bool HasConnection){ }
void AFGHoverPackAttachment::SetCurrentRailroadTrack( AFGBuildableRailroadTrack* RailroadTrack){ }
void AFGHoverPackAttachment::SetConnectionLocation(const FVector& NewLocation){ }
void AFGHoverPackAttachment::OnRep_CurrentHoverMode(){ }
void AFGHoverPackAttachment::OnRep_HasConnection(){ }
void AFGHoverPackAttachment::OnRep_CurrentConnectionLocation(){ }
