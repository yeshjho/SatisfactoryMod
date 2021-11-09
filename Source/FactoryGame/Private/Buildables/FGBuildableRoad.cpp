// This file has been automatically generated by the Unreal Header Implementation tool

#include "Buildables/FGBuildableRoad.h"
#include "Hologram/FGRoadHologram.h"
#include "Components/SceneComponent.h"
#include "FGRoadConnectionComponent.h"
#include "FGSplineComponent.h"

void AFGBuildableRoad::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const{ }
AFGBuildableRoad::AFGBuildableRoad() : Super() {
	this->mConnection0 = CreateDefaultSubobject<UFGRoadConnectionComponent>(TEXT("Connection0"));
	this->mConnection1 = CreateDefaultSubobject<UFGRoadConnectionComponent>(TEXT("Connection1"));
	this->mSplineComponent = CreateDefaultSubobject<UFGSplineComponent>(TEXT("SplineComponent"));
	this->mHologramClass = AFGRoadHologram::StaticClass();
	this->mConnection0->SetupAttachment(RootComponent);
	this->mConnection1->SetupAttachment(RootComponent);
	this->mSplineComponent->SetupAttachment(RootComponent);
}
void AFGBuildableRoad::BeginPlay(){ }
