// This file has been automatically generated by the Unreal Header Implementation tool

#include "Equipment/FGEquipmentDecoration.h"

AFGEquipmentDecoration::AFGEquipmentDecoration() : Super() {
	this->mPlaceDistanceMax = 1000.0;
	this->mArmAnimation = EArmEquipment::AE_Generic1Hand;
	this->mDefaultEquipmentActions = 1;
}
void AFGEquipmentDecoration::Tick(float DeltaSeconds){ Super::Tick(DeltaSeconds); }
void AFGEquipmentDecoration::OnPrimaryFirePressed(){ }
void AFGEquipmentDecoration::Server_PrimaryFire_Implementation(){ }
bool AFGEquipmentDecoration::Server_PrimaryFire_Validate(){ return bool(); }
void AFGEquipmentDecoration::HandleDefaultEquipmentActionEvent(EDefaultEquipmentAction action, EDefaultEquipmentActionEvent actionEvent){ }
