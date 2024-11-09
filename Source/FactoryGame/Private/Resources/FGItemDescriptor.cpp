// This file has been automatically generated by the Unreal Header Implementation tool

#include "Resources/FGItemDescriptor.h"
#include "FGCategory.h"
#include "FGItemCategory.h"
#include "FGResourceSettings.h"

EResourceForm UFGItemDescriptor::GetForm(TSubclassOf<UFGItemDescriptor> inClass) {
	if (inClass)
		return inClass.GetDefaultObject()->mForm;
	else
		return EResourceForm();
}
float UFGItemDescriptor::GetEnergyValue(TSubclassOf<UFGItemDescriptor> inClass) {
	if (inClass)
		return inClass.GetDefaultObject()->mEnergyValue;
	else
		return float();
}
float UFGItemDescriptor::GetRadioactiveDecay(TSubclassOf<UFGItemDescriptor> inClass) {
	if (inClass)
		return inClass.GetDefaultObject()->mRadioactiveDecay;
	else
		return float();
}
FText UFGItemDescriptor::GetItemName(TSubclassOf<UFGItemDescriptor> inClass) {
	if (!inClass)
		return FText();
	if (inClass.GetDefaultObject()->mUseDisplayNameAndDescription)
		return inClass.GetDefaultObject()->mDisplayName;
	else
		return FText::FromString(inClass->GetName());
}
FText UFGItemDescriptor::GetItemDescription(TSubclassOf<UFGItemDescriptor> inClass) {
	if (inClass)
		return inClass.GetDefaultObject()->mDescription;
	else
		return FText();
}
UTexture2D* UFGItemDescriptor::GetSmallIcon(TSubclassOf<UFGItemDescriptor> inClass) {
	if (inClass)
		return inClass.GetDefaultObject()->mSmallIcon;
	else
		return nullptr;
}
UTexture2D* UFGItemDescriptor::GetBigIcon(TSubclassOf<UFGItemDescriptor> inClass) {
	if (inClass)
		return inClass.GetDefaultObject()->mPersistentBigIcon;
	else
		return nullptr;
}
UStaticMesh* UFGItemDescriptor::GetItemMesh(TSubclassOf<UFGItemDescriptor> inClass) {
	if (inClass)
		return inClass.GetDefaultObject()->mConveyorMesh;
	else
		return nullptr;
}
int32 UFGItemDescriptor::GetStackSize( TSubclassOf< UFGItemDescriptor > inClass )
{
	QUICK_SCOPE_CYCLE_COUNTER( STAT_UFGItemDescriptor_GetStackSize )

	if( inClass )
	{
		const int32 stackSize = inClass->GetDefaultObject< UFGItemDescriptor >()->mCachedStackSize;
        
		if( stackSize != INDEX_NONE )
		{
			return stackSize;
		}

		// get the data if not set.
		if( const int32* stackSizePtr = UFGResourceSettings::Get()->mStackSizes.FindKey( inClass->GetDefaultObject< UFGItemDescriptor >()->mStackSize ) )
		{
			GetMutableDefault<UFGItemDescriptor>( inClass )->mCachedStackSize = *stackSizePtr;
			return *stackSizePtr;
		}
	}
    
	UE_LOG( LogGame, VeryVerbose, TEXT( "FGItemDescriptor::GetStackSize: class was nullpeter." ) );
	return -1;
}
bool UFGItemDescriptor::CanBeDiscarded(TSubclassOf<UFGItemDescriptor> inClass) {
	if (inClass)
		return inClass.GetDefaultObject()->mCanBeDiscarded;
	else
		return bool();
}
bool UFGItemDescriptor::RememberPickUp(TSubclassOf<UFGItemDescriptor> inClass) {
	if (inClass)
		return inClass.GetDefaultObject()->mRememberPickUp;
	else
		return bool();
}
TSubclassOf<class UFGCategory> UFGItemDescriptor::GetCategory(TSubclassOf<UFGItemDescriptor> inClass) {
	if (inClass)
		return inClass.GetDefaultObject()->mCategory;
	else
		return TSubclassOf<UFGCategory>();
}
FColor UFGItemDescriptor::GetFluidColor(TSubclassOf<UFGItemDescriptor> inClass) {
	if (inClass)
		return inClass.GetDefaultObject()->mFluidColor;
	else
		return FColor();
}
FLinearColor UFGItemDescriptor::GetFluidColorLinear(TSubclassOf<UFGItemDescriptor> inClass) {
	if (inClass)
		return inClass.GetDefaultObject()->mFluidColor.ReinterpretAsLinear();
	else
		return FLinearColor();
}
#if WITH_EDITOR
void UFGItemDescriptor::PostEditChangeProperty( struct FPropertyChangedEvent& propertyChangedEvent )
{
	Super::PostEditChangeProperty( propertyChangedEvent );
	const FName propertyName = propertyChangedEvent.Property ? propertyChangedEvent.Property->GetFName() : NAME_None;
	if( propertyName == GET_MEMBER_NAME_CHECKED( UFGItemDescriptor, mAbbreviatedDisplayName ) )
	{
		mAbbreviatedDisplayName = FText::ChangeKey( FTextKey( "AbbreviatedDisplayName" ), FTextKey( mDisplayName.ToString() ), mAbbreviatedDisplayName );
	}

	if( propertyName == GET_MEMBER_NAME_CHECKED( UFGItemDescriptor, mStackSize ) )
	{
		if( const int32* stackSizePtr = UFGResourceSettings::Get()->mStackSizes.FindKey( mStackSize ) )
		{
			mCachedStackSize = *stackSizePtr;
		}
		else
		{
			mCachedStackSize = INDEX_NONE;
		}
	}
}
#endif
void UFGItemDescriptor::PostLoad() {
	Super::PostLoad();
	if( const int32* stackSizePtr = UFGResourceSettings::Get()->mStackSizes.FindKey( mStackSize ) )
	{
		mCachedStackSize = *stackSizePtr;
	}
	else
	{
		mCachedStackSize = INDEX_NONE;
	}
}

TAutoConsoleVariable<int32> CVarStressTestRadioActivity(TEXT("CVarStressTestRadioActivity"), 0, TEXT(""));
#if !UE_BUILD_SHIPPING
#endif 
UFGItemDescriptor::UFGItemDescriptor() : Super() {
	this->mUseDisplayNameAndDescription = true;
	this->mDisplayName = INVTEXT("");
	this->mDescription = INVTEXT("");
	this->mAbbreviatedDisplayName = INVTEXT("");
	this->mStackSize = EStackSize::SS_MEDIUM;
	this->mCanBeDiscarded = true;
	this->mRememberPickUp = false;
	this->mEnergyValue = 0.0;
	this->mRadioactiveDecay = 0.0;
	this->mForm = EResourceForm::RF_SOLID;
	this->mGasType = EGasType::GT_NORMAL;
	this->mSmallIcon = nullptr;
	this->mPersistentBigIcon = nullptr;
	this->mCrosshairMaterial = nullptr;
	this->mIsAlienItem = false;
	this->mInventorySettingsWidget = nullptr;
	this->mConveyorMesh = nullptr;
	this->mCategory = nullptr;
	this->mQuickSwitchGroup = nullptr;
	this->mMenuPriority = 0.0;
	this->mFluidColor = FColor(255, 255, 255, 0);
	this->mGasColor = FColor(255, 255, 255, 0);
	this->mClassToScanFor = nullptr;
	this->mCustomScannableDetails = nullptr;
	this->mScannableType = EScannableActorType::RTWOT_Default;
	this->mRequiredSchematicToScan = nullptr;
	this->mShouldOverrideScannerDisplayText = false;
	this->mScannerDisplayText = INVTEXT("");
	this->mScannerLightColor = FColor(0, 0, 0, 0);
	this->mNeedsPickUpMarker = false;
	this->mItemIndex = -1;
}
void UFGItemDescriptor::Serialize(FArchive& ar){ Super::Serialize(ar); }
void UFGItemDescriptor::BeginDestroy(){ Super::BeginDestroy(); }
EGasType UFGItemDescriptor::GetGasType(TSubclassOf< UFGItemDescriptor > inClass){ return EGasType(); }
FText UFGItemDescriptor::GetAbbreviatedDisplayName(TSubclassOf< UFGItemDescriptor > inClass){ return FText(); }
UMaterialInterface* UFGItemDescriptor::GetCrosshairMaterial(TSubclassOf< UFGItemDescriptor > inClass){ return nullptr; }
void UFGItemDescriptor::GetDescriptorStatBars(TSubclassOf< UFGItemDescriptor > inClass, TArray<FDescriptorStatBar>& out_DescriptorStatBars){ }
bool UFGItemDescriptor::IsAlienItem(TSubclassOf< UFGItemDescriptor > inClass){ return bool(); }
void UFGItemDescriptor::GetInventorySettingsWidget(TSubclassOf< UFGItemDescriptor > inClass, TSubclassOf<class UFGInventorySettingsWidget>& out_InventorySettingsWidget){ }
float UFGItemDescriptor::GetStackSizeConverted(TSubclassOf< UFGItemDescriptor > inClass){ return float(); }
void UFGItemDescriptor::GetSubCategories(TSubclassOf< UFGItemDescriptor > inClass, TArray< TSubclassOf<  UFGCategory > >& out_subCategories){ }
TArray< TSubclassOf< class UFGCategory > > UFGItemDescriptor::GetSubCategoriesOfClass(TSubclassOf< UFGItemDescriptor > inClass, TSubclassOf<  UFGCategory > outputCategoryClass){ return TArray<TSubclassOf<class UFGCategory> >(); }
TSubclassOf< class UFGQuickSwitchGroup > UFGItemDescriptor::GetQuickSwitchGroup(TSubclassOf< UFGItemDescriptor > inClass){ return TSubclassOf<class UFGQuickSwitchGroup>(); }
float UFGItemDescriptor::GetMenuPriority(TSubclassOf< UFGItemDescriptor > inClass){ return float(); }
FColor UFGItemDescriptor::GetGasColor(TSubclassOf< UFGItemDescriptor > inClass){ return FColor(); }
FLinearColor UFGItemDescriptor::GetGasColorLinear(TSubclassOf< UFGItemDescriptor > inClass){ return FLinearColor(); }
TArray< FCompatibleItemDescriptors > UFGItemDescriptor::GetCompatibleItemDescriptors(TSubclassOf< UFGItemDescriptor > inClass){ return TArray<FCompatibleItemDescriptors>(); }
TSubclassOf< AActor > UFGItemDescriptor::GetClassToScanFor(TSubclassOf< UFGItemDescriptor > inClass){ return TSubclassOf<AActor>(); }
TSubclassOf< class UFGScannableDetails > UFGItemDescriptor::GetCustomScannableDetails(TSubclassOf< UFGItemDescriptor > inClass){ return TSubclassOf<class UFGScannableDetails>(); }
EScannableActorType UFGItemDescriptor::GetScannableActorType(TSubclassOf< UFGItemDescriptor > inClass){ return EScannableActorType(); }
TSubclassOf<UFGSchematic> UFGItemDescriptor::GetRequiredSchematicToScan(TSubclassOf< UFGItemDescriptor > inClass){ return TSubclassOf<UFGSchematic>(); }
FText UFGItemDescriptor::GetScannerDisplayText(TSubclassOf< UFGItemDescriptor > inClass){ return FText(); }
FColor UFGItemDescriptor::GetScannerLightColor(TSubclassOf< UFGItemDescriptor > inClass){ return FColor(); }
bool UFGItemDescriptor::NeedsPickupMapMarker(TSubclassOf<UFGItemDescriptor> inClass){ return bool(); }
void UFGItemDescriptor::SetItemEncountered(TSubclassOf<UFGItemDescriptor> Class, int32 Index){ }
int32 UFGItemDescriptor::IsItemEncountered(TSubclassOf<UFGItemDescriptor> Class){ return int32(); }
bool UFGItemDescriptor::CanItemBePickedup(TSubclassOf< UFGItemDescriptor > inClass){ return bool(); }
bool UFGItemDescriptor::CanItemBePickedup(UFGItemDescriptor* inClass){ return bool(); }
FText UFGItemDescriptor::GetItemNameInternal() const{ return FText(); }
FString UFGItemDescriptor::GetItemNameInternalAsString() const{ return FString(); }
FText UFGItemDescriptor::GetItemDescriptionInternal() const{ return FText(); }
UTexture2D* UFGItemDescriptor::Internal_GetSmallIcon() const{ return nullptr; }
UTexture2D* UFGItemDescriptor::Internal_GetBigIcon() const{ return nullptr; }
