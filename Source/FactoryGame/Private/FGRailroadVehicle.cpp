// This file has been automatically generated by the Unreal Header Implementation tool

#include "FGRailroadVehicle.h"
#include "Hologram/FGRailroadVehicleHologram.h"

AFGRailroadVehicle::AFGRailroadVehicle() : Super() {
	this->mTrain = nullptr;
	this->mLength = 500.0;
	this->mCoupledVehicleFront = nullptr;
	this->mCoupledVehicleBack = nullptr;
	this->mIsOrientationReversed = false;
	this->mTrackPosition.Track = nullptr;
	this->mTrackPosition.Offset = 0.0;
	this->mTrackPosition.Forward = 0.0;
	this->mIsDerailed = false;
	this->mLastServerTime = 0.0;
	this->mServerTrack = nullptr;
	this->mServerOffset = 0.0;
	this->mServerForward = 0.0;
	this->mServerSpeed = 0.0;
	this->mHologramClass = AFGRailroadVehicleHologram::StaticClass();
	this->SetReplicatingMovement(false);
}
void AFGRailroadVehicle::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const{ }
void AFGRailroadVehicle::BeginPlay(){ }
void AFGRailroadVehicle::Destroyed(){ }
void AFGRailroadVehicle::Serialize(FArchive& ar){ Super::Serialize(ar); }
void AFGRailroadVehicle::Tick(float dt){ }
bool AFGRailroadVehicle::CanDismantle_Implementation() const{ return bool(); }
void AFGRailroadVehicle::GainedSignificance_Implementation(){ }
void AFGRailroadVehicle::LostSignificance_Implementation(){ }
void AFGRailroadVehicle::UpdateUseState_Implementation( AFGCharacterPlayer* byCharacter, const FVector& atLocation,  UPrimitiveComponent* componentHit, FUseState& out_useState) const{ }
void AFGRailroadVehicle::OnUse_Implementation( AFGCharacterPlayer* byCharacter, const FUseState& state){ }
void AFGRailroadVehicle::OnUseStop_Implementation( AFGCharacterPlayer* byCharacter, const FUseState& state){ }
bool AFGRailroadVehicle::IsUseable_Implementation() const{ return bool(); }
void AFGRailroadVehicle::StartIsLookedAt_Implementation( AFGCharacterPlayer* byCharacter, const FUseState& state){ }
void AFGRailroadVehicle::StopIsLookedAt_Implementation( AFGCharacterPlayer* byCharacter, const FUseState& state){ }
FText AFGRailroadVehicle::GetLookAtDecription_Implementation( AFGCharacterPlayer* byCharacter, const FUseState& state) const{ return FText(); }
void AFGRailroadVehicle::RegisterInteractingPlayer_Implementation( AFGCharacterPlayer* player){ }
void AFGRailroadVehicle::UnregisterInteractingPlayer_Implementation( AFGCharacterPlayer* player){ }
bool AFGRailroadVehicle::UpdateRepresentation(){ return bool(); }
bool AFGRailroadVehicle::GetActorShouldShowInCompass(){ return bool(); }
bool AFGRailroadVehicle::GetActorShouldShowOnMap(){ return bool(); }
void AFGRailroadVehicle::UpdatePower(){ }
UFGRailroadVehicleMovementComponent* AFGRailroadVehicle::GetRailroadVehicleMovementComponent() const{ return nullptr; }
bool AFGRailroadVehicle::IsDocked() const{ return bool(); }
void AFGRailroadVehicle::SetTrackPosition(const FRailroadTrackPosition& position){ }
void AFGRailroadVehicle::OnDerail(const FVector& velocity){ }
void AFGRailroadVehicle::OnRerail(){ }
void AFGRailroadVehicle::DisplayDebug( UCanvas* canvas, const  FDebugDisplayInfo& debugDisplay, float& YL, float& YPos){ }
void AFGRailroadVehicle::OnCollided_Implementation(AFGRailroadVehicle* withVehicle, float impactVelocity, bool isPrimaryEvent, bool isDerailed){ }
void AFGRailroadVehicle::TickClientSimulation(float dt){ }
void AFGRailroadVehicle::OnIsSimulatedChanged(){ }
void AFGRailroadVehicle::CoupleVehicleAt(AFGRailroadVehicle* vehicle, ERailroadVehicleCoupler coupler){ }
void AFGRailroadVehicle::DecoupleVehicleAt(ERailroadVehicleCoupler coupler){ }
void AFGRailroadVehicle::OnRep_IsOrientationReversed(){ }
void AFGRailroadVehicle::OnRep_Train(){ }
void AFGRailroadVehicle::OnRep_IsDerailed(){ }
