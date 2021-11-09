// This file has been automatically generated by the Unreal Header Implementation tool

#include "FGCharacterAnimInstance.h"

UFGCharacterAnimInstance::UFGCharacterAnimInstance() : Super() {
	this->mDirection = 0.0;
	this->mAccelerationVectorLength = 0.0;
	this->mVelocity.X = 0.0;
	this->mVelocity.Y = 0.0;
	this->mVelocity.Z = 0.0;
	this->mVelocityLocalNormalized.X = 0.0;
	this->mVelocityLocalNormalized.Y = 0.0;
	this->mVelocityLocalNormalized.Z = 0.0;
	this->mAccelerationLocalNormalized.X = 0.0;
	this->mAccelerationLocalNormalized.Y = 0.0;
	this->mAccelerationLocalNormalized.Z = 0.0;
	this->mCachedCharacter = nullptr;
	this->mCachedMovementMode = EMovementMode::MOVE_None;
	this->mCachedDefaultWalkMode = EMovementMode::MOVE_None;
	this->mSpeed = 0.0;
	this->mSpeedLastFrame = 0.0;
	this->mSpeedZ = 0.0;
	this->mWalkRotation = 0.0;
	this->mIsStandingStill = false;
	this->mIsAccelerating = false;
	this->mYawDelta = 0.0;
	this->mAimYaw = 0.0;
	this->mAimYawInterpSpeed = 5.0;
	this->mRootRotation.Pitch = 0.0;
	this->mRootRotation.Yaw = 0.0;
	this->mRootRotation.Roll = 0.0;
	this->mTurnInPlaceLeft = false;
	this->mTurnInPlaceRight = false;
	this->mTurnInPlaceComplete = false;
	this->mTurnLeftCurve = nullptr;
	this->mTurnRightCurve = nullptr;
	this->mActorRotationForwardVector.X = 0.0;
	this->mActorRotationForwardVector.Y = 0.0;
	this->mActorRotationForwardVector.Z = 0.0;
	this->mActorRotationForwardVectorReference.X = 0.0;
	this->mActorRotationForwardVectorReference.Y = 0.0;
	this->mActorRotationForwardVectorReference.Z = 0.0;
	this->mAimYawReductionStartTime = 0.0;
	this->mAimYawReductionCurrentTime = 0.0;
	this->mActorRotationLastTick.Pitch = 0.0;
	this->mActorRotationLastTick.Yaw = 0.0;
	this->mActorRotationLastTick.Roll = 0.0;
	this->mYawRotationStrength = 7.0;
	this->mYawRotationInterpSpeed = 5.0;
	this->mRootRotationInterpSpeed = 5.0;
	this->mAllowedToTurn = false;
	this->mTurnInPlaceDefaultTime = 0.8;
	this->mUseTurnInPlace = false;
	this->mCanUpdateActorRotationReference = false;
	this->mYawAimMaxValue = 45.0;
	this->mYawAimMinValue = 45.0;
	this->mAimPitch = 0.0;
	this->mAimPitchInterpSpeed = 5.0;
	this->mGetDeltaPitchRotation = false;
	this->mUsePreLand = false;
	this->mPreLand = false;
	this->mPreLandVelocityMultiplier = 5.0;
	this->mPreLandCollisionChannels.Add(ECollisionChannel::ECC_WorldStatic);
}
void UFGCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds){ }
void UFGCharacterAnimInstance::OnPointDamageTaken_Implementation(FVector shootDirection){ }
void UFGCharacterAnimInstance::OnAnyDamageTaken_Implementation(){ }
void UFGCharacterAnimInstance::OnRadialDamageTaken_Implementation(){ }
FRotator UFGCharacterAnimInstance::GetDesiredWalkRotation(){ return FRotator(); }
FRotator UFGCharacterAnimInstance::GetDesiredRunLeanRotation(){ return FRotator(); }
void UFGCharacterAnimInstance::TurnInPlaceEvent(float dt){ }
FVector UFGCharacterAnimInstance::GetCharacterVelocity()const{ return FVector(); }
