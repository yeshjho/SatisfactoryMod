// This file has been automatically generated by the Unreal Header Implementation tool

#include "FGCustomizationRecipe.h"

#if WITH_EDITOR
EDataValidationResult UFGCustomizationRecipe::IsDataValid(TArray< FText >& ValidationErrors){ return EDataValidationResult::Valid; }
#endif 
UFGCustomizationRecipe::UFGCustomizationRecipe() : Super() {
	this->mCustomizationDesc = nullptr;
}
TSubclassOf< class UFGFactoryCustomizationDescriptor > UFGCustomizationRecipe::GetCustomizationDescriptor(TSubclassOf< UFGCustomizationRecipe > inClass){ return TSubclassOf<class UFGFactoryCustomizationDescriptor>(); }
float UFGCustomizationRecipe::GetCustomizationDescMenuPriority(TSubclassOf< UFGCustomizationRecipe > inClass){ return float(); }
