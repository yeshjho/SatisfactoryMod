// This file has been automatically generated by the Unreal Header Implementation tool

#include "Buildables/FGBuildableManufacturer.h"
#include "FGInventoryComponent.h"
#include "Net/UnrealNetwork.h"

void UFGManufacturerClipboardRCO::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UFGManufacturerClipboardRCO, mForceNetField_UFGManufacturerClipboardRCO);
}
void UFGManufacturerClipboardRCO::Server_PasteSettings_Implementation( AFGBuildableManufacturer* manufacturer, AFGCharacterPlayer* player, TSubclassOf<  UFGRecipe > recipe, float overclock, float productionBoost, TSubclassOf<UFGPowerShardDescriptor> overclockingShard, TSubclassOf<UFGPowerShardDescriptor> productionBoostShard){ }
void AFGBuildableManufacturer::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AFGBuildableManufacturer, mCurrentRecipe);
}
void AFGBuildableManufacturer::GetConditionalReplicatedProps(TArray<FFGCondReplicatedProperty>& outProps) const{ }
AFGBuildableManufacturer::AFGBuildableManufacturer() : Super() {
	this->mManufacturingSpeed = 1.0;
	this->mCurrentManufacturingProgress = 0.0;
	this->mInputInventory = CreateDefaultSubobject<UFGInventoryComponent>(TEXT("InputInventory"));
	this->mOutputInventory = CreateDefaultSubobject<UFGInventoryComponent>(TEXT("OutputInventory"));
	this->mCurrentRecipe = nullptr;
	this->mCachedRecipe = nullptr;
	this->mCanEverMonitorProductivity = true;
	this->mCanChangePotential = true;
	this->mCanChangeProductionBoost = true;
	this->NetDormancy = ENetDormancy::DORM_Initial;
}
void AFGBuildableManufacturer::BeginPlay(){ Super::BeginPlay(); }
void AFGBuildableManufacturer::EndPlay(const EEndPlayReason::Type endPlayReason){ Super::EndPlay(endPlayReason); }
bool AFGBuildableManufacturer::IsConfigured() const{ return bool(); }
float AFGBuildableManufacturer::GetProductionCycleTime() const{ return float(); }
float AFGBuildableManufacturer::GetDefaultProductionCycleTime() const{ return float(); }
float AFGBuildableManufacturer::GetProductionCycleTimeForRecipe(TSubclassOf< UFGRecipe > recipe) const{ return float(); }
float AFGBuildableManufacturer::CalcProductionCycleTimeForPotential(float potential) const{ return float(); }
void AFGBuildableManufacturer::SetCurrentProductionBoost(float newProductionBoost){ }
UFGFactoryClipboardSettings* AFGBuildableManufacturer::CopySettings_Implementation(){ return nullptr; }
bool AFGBuildableManufacturer::PasteSettings_Implementation(UFGFactoryClipboardSettings* settings){ return bool(); }
TSubclassOf<UFGItemDescriptor> AFGBuildableManufacturer::GetRecipeProducerItemDescriptor_Implementation(UObject* WorldContext) const{ return TSubclassOf<UFGItemDescriptor>(); }
bool AFGBuildableManufacturer::MoveOrDropInputInventory(AFGCharacterPlayer* pawn){ return bool(); }
bool AFGBuildableManufacturer::MoveOrDropOutputInventory(AFGCharacterPlayer* pawn){ return bool(); }
float AFGBuildableManufacturer::GetProductionProgress() const{ return float(); }
void AFGBuildableManufacturer::GetAvailableRecipes(TArray< TSubclassOf<  UFGRecipe > >& out_recipes) const{ }
void AFGBuildableManufacturer::SetRecipe(TSubclassOf<  UFGRecipe > recipe){ }
bool AFGBuildableManufacturer::CanProduce_Implementation() const{ return bool(); }
void AFGBuildableManufacturer::Factory_CollectInput_Implementation(){ }
void AFGBuildableManufacturer::Factory_PullPipeInput_Implementation(float dt){ }
void AFGBuildableManufacturer::Factory_PushPipeOutput_Implementation(float dt){ }
void AFGBuildableManufacturer::Factory_TickProducing(float dt){ }
void AFGBuildableManufacturer::PostLoadGame_Implementation(int32 saveVersion, int32 gameVersion){ }
void AFGBuildableManufacturer::InvalidateCacheCanProduce_InputItemAdded(TSubclassOf< UFGItemDescriptor > itemClass, const int32 numAdded, UFGInventoryComponent* sourceInventory){ }
void AFGBuildableManufacturer::InvalidateCacheCanProduce_InputItemRemoved(TSubclassOf< UFGItemDescriptor > itemClass, const int32 numAdded, UFGInventoryComponent* targetInventory){ }
void AFGBuildableManufacturer::InvalidateCacheCanProduce_OutputItemAdded(TSubclassOf< UFGItemDescriptor > itemClass, const int32 numAdded, UFGInventoryComponent* sourceInventory){ }
void AFGBuildableManufacturer::InvalidateCacheCanProduce_OutputItemRemoved(TSubclassOf< UFGItemDescriptor > itemClass, const int32 numAdded, UFGInventoryComponent* targetInventory){ }
void AFGBuildableManufacturer::CreateInventories(){ }
void AFGBuildableManufacturer::OnRep_CurrentRecipe(){ }
void AFGBuildableManufacturer::GetInputInventoryItems(TArray< FInventoryStack >& out_items) const{ }
void AFGBuildableManufacturer::ClearInputInventoryItems(){ }
void AFGBuildableManufacturer::GetOutputInventoryItems(TArray< FInventoryStack >& out_items){ }
void AFGBuildableManufacturer::ClearOutputInventoryItems(){ }
void AFGBuildableManufacturer::SetUpInventoryFilters(){ }
bool AFGBuildableManufacturer::AssignInputAccessIndices(TSubclassOf< UFGRecipe > recipe){ return bool(); }
bool AFGBuildableManufacturer::AssignOutputAccessIndices(TSubclassOf< UFGRecipe > recipe){ return bool(); }
void AFGBuildableManufacturer::Factory_ConsumeIngredients(){ }
bool AFGBuildableManufacturer::HasRequiredIngredients() const{ return bool(); }
