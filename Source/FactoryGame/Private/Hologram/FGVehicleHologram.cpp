// This file has been automatically generated by the Unreal Header Implementation tool

#include "Hologram/FGVehicleHologram.h"

AFGVehicleHologram::AFGVehicleHologram() : Super() {
	this->mDefaultSwatch = nullptr;
}
void AFGVehicleHologram::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

}
void AFGVehicleHologram::BeginPlay(){ }
AActor* AFGVehicleHologram::Construct(TArray< AActor* >& out_children, FNetConstructionID netConstructionID){ return nullptr; }
void AFGVehicleHologram::SetHologramLocationAndRotation(const FHitResult& hitResult){ }
void AFGVehicleHologram::CheckValidPlacement(){ }
AFGVehicle* AFGVehicleHologram::ConstructVehicle(FNetConstructionID netConstructionID) const{ return nullptr; }
