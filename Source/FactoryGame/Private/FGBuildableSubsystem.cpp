// This file has been automatically generated by the Unreal Header Implementation tool

#include "FGBuildableSubsystem.h"

#if STATS
#endif 
AFGBuildableSubsystem::AFGBuildableSubsystem() : Super() {
	this->mDistanceConsideredClose = 30000.0;
	this->mNumFactoriesNeededForCloseCheck = 5;
	this->mBuildableInstancesActor = nullptr;
	this->mFactoryLegInstancesActor = nullptr;
	this->mProductionIndicatorInstanceManager = nullptr;
	this->mColorSlotsPrimary[0].B = 39;;
	this->mColorSlotsPrimary[0].G = 112;;
	this->mColorSlotsPrimary[0].R = 255;;
	this->mColorSlotsPrimary[0].A = 255;;
	this->mColorSlotsPrimary[1].B = 167;;
	this->mColorSlotsPrimary[1].G = 100;;
	this->mColorSlotsPrimary[1].R = 38;;
	this->mColorSlotsPrimary[1].A = 255;;
	this->mColorSlotsPrimary[2].B = 19;;
	this->mColorSlotsPrimary[2].G = 52;;
	this->mColorSlotsPrimary[2].R = 204;;
	this->mColorSlotsPrimary[2].A = 255;;
	this->mColorSlotsPrimary[3].B = 47;;
	this->mColorSlotsPrimary[3].G = 33;;
	this->mColorSlotsPrimary[3].R = 32;;
	this->mColorSlotsPrimary[3].A = 255;;
	this->mColorSlotsPrimary[4].B = 206;;
	this->mColorSlotsPrimary[4].G = 195;;
	this->mColorSlotsPrimary[4].R = 190;;
	this->mColorSlotsPrimary[4].A = 255;;
	this->mColorSlotsPrimary[5].B = 73;;
	this->mColorSlotsPrimary[5].G = 186;;
	this->mColorSlotsPrimary[5].R = 127;;
	this->mColorSlotsPrimary[5].A = 255;;
	this->mColorSlotsPrimary[6].B = 202;;
	this->mColorSlotsPrimary[6].G = 89;;
	this->mColorSlotsPrimary[6].R = 255;;
	this->mColorSlotsPrimary[6].A = 255;;
	this->mColorSlotsPrimary[7].B = 215;;
	this->mColorSlotsPrimary[7].G = 223;;
	this->mColorSlotsPrimary[7].R = 115;;
	this->mColorSlotsPrimary[7].A = 255;;
	this->mColorSlotsPrimary[8].B = 26;;
	this->mColorSlotsPrimary[8].G = 84;;
	this->mColorSlotsPrimary[8].R = 125;;
	this->mColorSlotsPrimary[8].A = 255;;
	this->mColorSlotsPrimary[9].B = 0;;
	this->mColorSlotsPrimary[9].G = 0;;
	this->mColorSlotsPrimary[9].R = 0;;
	this->mColorSlotsPrimary[9].A = 0;;
	this->mColorSlotsPrimary[10].B = 0;;
	this->mColorSlotsPrimary[10].G = 0;;
	this->mColorSlotsPrimary[10].R = 0;;
	this->mColorSlotsPrimary[10].A = 0;;
	this->mColorSlotsPrimary[11].B = 0;;
	this->mColorSlotsPrimary[11].G = 0;;
	this->mColorSlotsPrimary[11].R = 0;;
	this->mColorSlotsPrimary[11].A = 0;;
	this->mColorSlotsPrimary[12].B = 0;;
	this->mColorSlotsPrimary[12].G = 0;;
	this->mColorSlotsPrimary[12].R = 0;;
	this->mColorSlotsPrimary[12].A = 0;;
	this->mColorSlotsPrimary[13].B = 0;;
	this->mColorSlotsPrimary[13].G = 0;;
	this->mColorSlotsPrimary[13].R = 0;;
	this->mColorSlotsPrimary[13].A = 0;;
	this->mColorSlotsPrimary[14].B = 0;;
	this->mColorSlotsPrimary[14].G = 0;;
	this->mColorSlotsPrimary[14].R = 0;;
	this->mColorSlotsPrimary[14].A = 0;;
	this->mColorSlotsPrimary[15].B = 0;;
	this->mColorSlotsPrimary[15].G = 0;;
	this->mColorSlotsPrimary[15].R = 0;;
	this->mColorSlotsPrimary[15].A = 0;;
	this->mColorSlotsPrimary[16].B = 0;;
	this->mColorSlotsPrimary[16].G = 0;;
	this->mColorSlotsPrimary[16].R = 0;;
	this->mColorSlotsPrimary[16].A = 0;;
	this->mColorSlotsPrimary[17].B = 0;;
	this->mColorSlotsPrimary[17].G = 0;;
	this->mColorSlotsPrimary[17].R = 0;;
	this->mColorSlotsPrimary[17].A = 0;;
	this->mColorSlotsSecondary[0].B = 67;;
	this->mColorSlotsSecondary[0].G = 34;;
	this->mColorSlotsSecondary[0].R = 29;;
	this->mColorSlotsSecondary[0].A = 255;;
	this->mColorSlotsSecondary[1].B = 31;;
	this->mColorSlotsSecondary[1].G = 64;;
	this->mColorSlotsSecondary[1].R = 86;;
	this->mColorSlotsSecondary[1].A = 255;;
	this->mColorSlotsSecondary[2].B = 97;;
	this->mColorSlotsSecondary[2].G = 80;;
	this->mColorSlotsSecondary[2].R = 78;;
	this->mColorSlotsSecondary[2].A = 255;;
	this->mColorSlotsSecondary[3].B = 75;;
	this->mColorSlotsSecondary[3].G = 92;;
	this->mColorSlotsSecondary[3].R = 61;;
	this->mColorSlotsSecondary[3].A = 255;;
	this->mColorSlotsSecondary[4].B = 67;;
	this->mColorSlotsSecondary[4].G = 34;;
	this->mColorSlotsSecondary[4].R = 29;;
	this->mColorSlotsSecondary[4].A = 255;;
	this->mColorSlotsSecondary[5].B = 67;;
	this->mColorSlotsSecondary[5].G = 34;;
	this->mColorSlotsSecondary[5].R = 29;;
	this->mColorSlotsSecondary[5].A = 255;;
	this->mColorSlotsSecondary[6].B = 67;;
	this->mColorSlotsSecondary[6].G = 34;;
	this->mColorSlotsSecondary[6].R = 29;;
	this->mColorSlotsSecondary[6].A = 255;;
	this->mColorSlotsSecondary[7].B = 67;;
	this->mColorSlotsSecondary[7].G = 34;;
	this->mColorSlotsSecondary[7].R = 29;;
	this->mColorSlotsSecondary[7].A = 255;;
	this->mColorSlotsSecondary[8].B = 88;;
	this->mColorSlotsSecondary[8].G = 88;;
	this->mColorSlotsSecondary[8].R = 83;;
	this->mColorSlotsSecondary[8].A = 255;;
	this->mColorSlotsSecondary[9].B = 0;;
	this->mColorSlotsSecondary[9].G = 0;;
	this->mColorSlotsSecondary[9].R = 0;;
	this->mColorSlotsSecondary[9].A = 0;;
	this->mColorSlotsSecondary[10].B = 0;;
	this->mColorSlotsSecondary[10].G = 0;;
	this->mColorSlotsSecondary[10].R = 0;;
	this->mColorSlotsSecondary[10].A = 0;;
	this->mColorSlotsSecondary[11].B = 0;;
	this->mColorSlotsSecondary[11].G = 0;;
	this->mColorSlotsSecondary[11].R = 0;;
	this->mColorSlotsSecondary[11].A = 0;;
	this->mColorSlotsSecondary[12].B = 0;;
	this->mColorSlotsSecondary[12].G = 0;;
	this->mColorSlotsSecondary[12].R = 0;;
	this->mColorSlotsSecondary[12].A = 0;;
	this->mColorSlotsSecondary[13].B = 0;;
	this->mColorSlotsSecondary[13].G = 0;;
	this->mColorSlotsSecondary[13].R = 0;;
	this->mColorSlotsSecondary[13].A = 0;;
	this->mColorSlotsSecondary[14].B = 0;;
	this->mColorSlotsSecondary[14].G = 0;;
	this->mColorSlotsSecondary[14].R = 0;;
	this->mColorSlotsSecondary[14].A = 0;;
	this->mColorSlotsSecondary[15].B = 0;;
	this->mColorSlotsSecondary[15].G = 0;;
	this->mColorSlotsSecondary[15].R = 0;;
	this->mColorSlotsSecondary[15].A = 0;;
	this->mColorSlotsSecondary[16].B = 0;;
	this->mColorSlotsSecondary[16].G = 0;;
	this->mColorSlotsSecondary[16].R = 0;;
	this->mColorSlotsSecondary[16].A = 0;;
	this->mColorSlotsSecondary[17].B = 0;;
	this->mColorSlotsSecondary[17].G = 0;;
	this->mColorSlotsSecondary[17].R = 0;;
	this->mColorSlotsSecondary[17].A = 0;;
	this->mFactoryOptimizationEnabled = true;
	this->mReplayEffecTimeDilation = 0.1;
	this->mReplayEffectTimerDefault = 0.5;
	this->mDefaultFactoryMaterial = nullptr;
	this->mFactoryTickFunction.TickGroup = ETickingGroup::TG_PrePhysics;
	this->mFactoryTickFunction.EndTickGroup = ETickingGroup::TG_PrePhysics;
	this->mFactoryTickFunction.bTickEvenWhenPaused = false;
	this->mFactoryTickFunction.bCanEverTick = false;
	this->mFactoryTickFunction.bStartWithTickEnabled = false;
	this->mFactoryTickFunction.bAllowTickOnDedicatedServer = true;
	this->mFactoryTickFunction.TickInterval = 0.0;
	this->mTimeUntilRepDetailCheck = 1.0;
	this->mSqDistanceForDetailCleanup = 10000000.0;
	this->PrimaryActorTick.TickGroup = ETickingGroup::TG_DuringPhysics;
	this->PrimaryActorTick.EndTickGroup = ETickingGroup::TG_PrePhysics;
	this->PrimaryActorTick.bTickEvenWhenPaused = false;
	this->PrimaryActorTick.bCanEverTick = true;
	this->PrimaryActorTick.bStartWithTickEnabled = true;
	this->PrimaryActorTick.bAllowTickOnDedicatedServer = true;
	this->PrimaryActorTick.TickInterval = 0.0;
}
void AFGBuildableSubsystem::PreSaveGame_Implementation(int32 saveVersion, int32 gameVersion){ }
void AFGBuildableSubsystem::PostSaveGame_Implementation(int32 saveVersion, int32 gameVersion){ }
void AFGBuildableSubsystem::PreLoadGame_Implementation(int32 saveVersion, int32 gameVersion){ }
void AFGBuildableSubsystem::PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion){ }
void AFGBuildableSubsystem::GatherDependencies_Implementation(TArray< UObject* >& out_dependentObjects){ }
bool AFGBuildableSubsystem::NeedTransform_Implementation(){ return bool(); }
bool AFGBuildableSubsystem::ShouldSave_Implementation() const{ return bool(); }
void AFGBuildableSubsystem::BeginPlay(){ }
void AFGBuildableSubsystem::EndPlay(const EEndPlayReason::Type EndPlayReason){ }
void AFGBuildableSubsystem::Tick(float dt){ }
void AFGBuildableSubsystem::TickFactory(float dt, ELevelTick TickType){ }
AFGBuildableSubsystem* AFGBuildableSubsystem::Get(UWorld* world){ return nullptr; }
AFGBuildableSubsystem* AFGBuildableSubsystem::Get(UObject* worldContext){ return nullptr; }
AFGBuildable* AFGBuildableSubsystem::BeginSpawnBuildable(TSubclassOf<  AFGBuildable > inClass, const FTransform& inTransform){ return nullptr; }
void AFGBuildableSubsystem::AddBuildable( AFGBuildable* buildable){ }
void AFGBuildableSubsystem::AddConveyor(AFGBuildableConveyorBase* conveyor){ }
void AFGBuildableSubsystem::RemoveFogPlanes( AFGBuildable* buildable){ }
AFGBuildableConveyorBase* AFGBuildableSubsystem::GetConnectedConveyorBelt( UFGFactoryConnectionComponent* connection){ return nullptr; }
void AFGBuildableSubsystem::RemoveBuildable( AFGBuildable* buildable){ }
void AFGBuildableSubsystem::RemoveConveyor(AFGBuildableConveyorBase* conveyor){ }
void AFGBuildableSubsystem::RemoveConveyorFromBucket(AFGBuildableConveyorBase* conveyorToRemove){ }
void AFGBuildableSubsystem::RearrangeConveyorBuckets(int32 emptiedBucketID){ }
void AFGBuildableSubsystem::RemoveAndSplitConveyorBucket(AFGBuildableConveyorBase* conveyorToRemove){ }
bool AFGBuildableSubsystem::IsServerSubSystem() const{ return bool(); }
void AFGBuildableSubsystem::GetTypedBuildable(TSubclassOf<  AFGBuildable > inClass, TArray<  AFGBuildable* >& out_buildables) const{ }
void AFGBuildableSubsystem::ReplayBuildingEffects(){ }
const FFactoryCustomizationColorSlot& AFGBuildableSubsystem::GetColorSlot_Data(uint8 index) const{ return *(new FFactoryCustomizationColorSlot); }
void AFGBuildableSubsystem::SetColorSlot_Data(uint8 index, FFactoryCustomizationColorSlot color){ }
TSubclassOf< class UFGFactoryCustomizationCollection > AFGBuildableSubsystem::GetCollectionForCustomizationClass(TSubclassOf<  UFGFactoryCustomizationDescriptor > collectionClassType) const{ return TSubclassOf<class UFGFactoryCustomizationCollection>(); }
FLinearColor AFGBuildableSubsystem::GetBuildableLightColorSlot(int32 index) const{ return FLinearColor(); }
TArray<FLinearColor> AFGBuildableSubsystem::GetBuildableLightColorSlots() const{ return TArray<FLinearColor>(); }
void AFGBuildableSubsystem::BuildableLightColorSlotsUpdated(const TArray< FLinearColor >& colors){ }
void AFGBuildableSubsystem::UpdateBuildableCullDistances(float newFactor){ }
UMaterialInstanceDynamic* AFGBuildableSubsystem::GetConveyorBelTrackMaterialFromSpeed(float speed, UMaterialInterface* currentMaterial){ return nullptr; }
void AFGBuildableSubsystem::DisplayDebug( UCanvas* canvas, const  FDebugDisplayInfo& debugDisplay, float& YL, float& YPos){ }
void AFGBuildableSubsystem::DebugEnableInstancing(bool enabled){ }
void AFGBuildableSubsystem::DebugGetFactoryActors(TArray< AActor* >& out_actors){ }
FName AFGBuildableSubsystem::GetMeshMapName(UStaticMesh* mesh, UMeshComponent* sourceComponent){ return FName(); }
FNetConstructionID AFGBuildableSubsystem::GetNewNetConstructionID(){ return FNetConstructionID(); }
void AFGBuildableSubsystem::GetNewNetConstructionID(FNetConstructionID& clientConstructionID){ }
void AFGBuildableSubsystem::SpawnPendingConstructionHologram(FNetConstructionID netConstructionID,  AFGHologram* templateHologram,  AFGBuildGun* instigatingBuildGun){ }
void AFGBuildableSubsystem::AddPendingConstructionHologram(FNetConstructionID netConstructionID,  AFGHologram* hologram){ }
void AFGBuildableSubsystem::RemovePendingConstructionHologram(FNetConstructionID netConstructionID){ }
void AFGBuildableSubsystem::ApplyCustomizationPreview( IFGColorInterface* colorInterface, const FFactoryCustomizationData& previewData){ }
void AFGBuildableSubsystem::ClearCustomizationPreview( IFGColorInterface* colorInterface){ }
void AFGBuildableSubsystem::ClearAllCustomizationPreviews(){ }
void AFGBuildableSubsystem::TryRemovePendingDetailActor( AFGReplicationDetailActor* detailActor){ }
void AFGBuildableSubsystem::AddPendingRemovalDetailActor( AFGReplicationDetailActor* detailActor){ }
AFGPlayerController* AFGBuildableSubsystem::GetLocalPlayerController() const{ return nullptr; }
float AFGBuildableSubsystem::GetDistanceSqToBoundingBox(const FVector& point,  AFGBuildable* buildable) const{ return float(); }
void AFGBuildableSubsystem::RegisterFactoryTickFunction(bool shouldRegister){ }
void AFGBuildableSubsystem::CreateFactoryStatID() const{ }
void AFGBuildableSubsystem::UpdateReplayEffects(float dt){ }
void AFGBuildableSubsystem::AddBuildableMeshInstances( AFGBuildable* buildable){ }
void AFGBuildableSubsystem::AddToTickGroup(AFGBuildable* buildable){ }
void AFGBuildableSubsystem::RemoveFromTickGroup(AFGBuildable* buildable){ }
void AFGBuildableSubsystem::SetupColoredMeshInstances(AFGBuildable* buildable){ }
void AFGBuildableSubsystem::SetupProductionIndicatorInstancing( AFGBuildable* buildable){ }
void AFGBuildableSubsystem::TickFactoryActors(float dt){ }
void AFGBuildableSubsystem::UpdateConveyorRenderType(FString cvar){ }
UFGColoredInstanceManager* AFGBuildableSubsystem::GetColoredInstanceManager( UFGColoredInstanceMeshProxy* proxy){ return nullptr; }
UFGFactoryLegInstanceManager* AFGBuildableSubsystem::GetFactoryLegInstanceManager( UFGFactoryLegInstanceMeshProxy* proxy){ return nullptr; }
TSubclassOf< class UFGFactoryCustomizationDescriptor_Swatch > AFGBuildableSubsystem::GetMigrationSwatchForSlot(int32 slotID){ return TSubclassOf<class UFGFactoryCustomizationDescriptor_Swatch>(); }
bool AFGBuildableSubsystem::IsBasedOn(const UMaterialInterface* instance, const UMaterial* base){ return bool(); }
uint8 AFGBuildableSubsystem::mCurrentSubStep = uint8();
uint8 AFGBuildableSubsystem::mCurrentSubStepMax = uint8();
