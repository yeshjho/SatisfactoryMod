// This file has been automatically generated by the Unreal Header Implementation tool

#include "WheeledVehicles/FGSplinePathMovementComponent.h"

#ifdef DEBUG_SELF_DRIVING
void UFGSplinePathMovementComponent::DrawDebugVehicle(int visualDebugLevel, int textualDebugLevel){ }
#endif 
UFGSplinePathMovementComponent::UFGSplinePathMovementComponent() : Super() {
	this->mTargetList = nullptr;
	this->mEndOfPath = false;
	this->mTransitionTarget = nullptr;
	this->mServerStartTime = 0.0;
	this->mIsDocked = false;
	this->mIsBlocked = false;
}
void UFGSplinePathMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const{ }
void UFGSplinePathMovementComponent::BeginPlay(){ }
void UFGSplinePathMovementComponent::EndPlay(const EEndPlayReason::Type EndPlayReason){ }
void UFGSplinePathMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction){ }
void UFGSplinePathMovementComponent::DestroyPath_Server(){ }
void UFGSplinePathMovementComponent::DrawDebug(int debugLevel) const{ }
void UFGSplinePathMovementComponent::TransitionToSplinePath_Server( AFGTargetPoint* target, const TArray< FVector >* intermediateStops , bool startReversing){ }
void UFGSplinePathMovementComponent::TransitionToSplinePath( AFGTargetPoint* target, const TArray< FVector >* intermediateStops , bool startReversing){ }
void UFGSplinePathMovementComponent::SetPaused(bool isPaused){ }
void UFGSplinePathMovementComponent::SetIsDocked(bool isDocked){ }
void UFGSplinePathMovementComponent::SetIsBlocked( AFGWheeledVehicle* blockingVehicle){ }
void UFGSplinePathMovementComponent::ResetIsBlocked(){ }
void UFGSplinePathMovementComponent::DrawTargetDebug(bool drawSearchPoints) const{ }
void UFGSplinePathMovementComponent::SetIsDeadlocked(bool isDeadlocked, bool notify){ }
void UFGSplinePathMovementComponent::ResetTarget(){ }
bool UFGSplinePathMovementComponent::TryClaim( AFGTargetPoint* target, bool essentialsOnly){ return bool(); }
void UFGSplinePathMovementComponent::GetShortestRotation(FRotator& rotation){ }
bool UFGSplinePathMovementComponent::HasData() const{ return bool(); }
float UFGSplinePathMovementComponent::GetServerTime() const{ return float(); }
void UFGSplinePathMovementComponent::OnRep_TransitionTarget(){ }
void UFGSplinePathMovementComponent::OnRep_ServerStartTime(){ }
void UFGSplinePathMovementComponent::OnRep_IsDocked(){ }
void UFGSplinePathMovementComponent::OnRep_IsBlocked(){ }
bool UFGSplinePathMovementComponent::TickSplinePathMovement(double deltaTime){ return bool(); }
void UFGSplinePathMovementComponent::AdvanceOnSplinePath(){ }
void UFGSplinePathMovementComponent::TryClaimTarget(){ }
void UFGSplinePathMovementComponent::SetTarget( AFGTargetPoint* newTarget){ }
float UFGSplinePathMovementComponent::GetStartTime(float adjustment){ return float(); }
void UFGSplinePathMovementComponent::SetStartTime(float startTime){ }
bool UFGSplinePathMovementComponent::ShouldAdjustClient() const{ return bool(); }
