// This file has been automatically generated by the Unreal Header Implementation tool

#include "FGSignTypes.h"

FSignStringElement FSignStringElement::InvalidStringElement = FSignStringElement();
void FSignStringData::ParseStringElements(){ }
void FSignStringData::ParseOutStringElements(){ }
int32 FSignStringData::AddElement(FString elementStrData){ return int32(); }
bool FSignStringData::RemoveElement(int32 elementID){ return bool(); }
int32 FSignStringData::GetNewUniqueID(){ return int32(); }
FSignStringElement& FSignStringData::GetStringElementRefFromID(int32 id){ return *(new FSignStringElement); }
bool FSignStringData::GetStringElementCopyFromID(int32 id, FSignStringElement& out_stringElement) const{ return bool(); }
FSignStringData FSignStringData::Empty = FSignStringData();
UFGSignTypeDescriptor::UFGSignTypeDescriptor() : Super() {
	this->mSignCanvasDimensions.X = 0.0;
	this->mSignCanvasDimensions.Y = 0.0;
	this->mDefaultForegroundColor = FLinearColor(0.0, 0.0, 0.0, 0.0);
	this->mDefaultBackgroundColor = FLinearColor(0.0, 0.0, 0.0, 0.0);
	this->mDefaultAuxiliaryColor = FLinearColor(0.0, 0.0, 0.0, 0.0);
}
