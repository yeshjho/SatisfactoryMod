// This file has been automatically generated by the Unreal Header Implementation tool

#include "Online/FGSessionSettings.h"

void UFGSessionSettingsModel::SetSessionDefinition(USessionDefinition* SessionDefinition){ }
void UFGSessionSettingsModel::SetSessionProfile(const FSessionProfilePath& ProfilePath){ }
void UFGSessionSettingsModel::SetSessionName(FString SessionName){ }
bool UFGSessionSettingsModel::HasPendingSessionDefinition() const{ return bool(); }
bool UFGSessionSettingsModel::HasPendingSessionProfile() const{ return bool(); }
bool UFGSessionSettingsModel::HasPendingSessionName() const{ return bool(); }
bool UFGSessionSettingsModel::RequiresSessionRestart() const{ return bool(); }
void UFGSessionSettingsModel::Reset(){ }
const FString& UFGSessionSettingsModel::GetSessionName() const{ return *(new FString); }
USessionDefinition* UFGSessionSettingsModel::GetSessionDefinition() const{ return nullptr; }
FName UFGSessionSettingsModel::GetSessionProfile() const{ return FName(); }
void UFGSessionSettingsModel::SetActiveSessionDefinition(USessionDefinition* activeSessionDefinition){ }
void UFGSessionSettingsModel::SetCurrentSessionDefinition(USessionDefinition* activeSessionDefinition){ }
void UFGSessionSettingsModel::SetCurrentSessionName(const FString& currentSessionName){ }
void UFGSessionSettingsModel::InitializeModel(UFGSessionSettings* sessionSettings, AFGGameMode* gameMode){ }
FCommonSessionCreationSettings UFGSessionSettings::MakeSessionCreationSettings(APlayerController* Host, const FString& SessionName, const FSoftObjectPath& MapAssetName, const FCreateNewGameParameters& CreateNewGameParameters){ return FCommonSessionCreationSettings(); }
void UFGSessionSettings::Initialize(FSubsystemCollectionBase& Collection){ }
void UFGSessionSettings::ApplySettingsModel(UFGSessionSettingsModel* Model){ }
void UFGSessionSettings::SetSessionSettingsProfile(FName ProfileName){ }
UFGSessionSettingsModel* UFGSessionSettings::MakeSessionSettingsModel(AFGGameMode* gameMode){ return nullptr; }
FName UFGSessionSettings::GetCurrentProfileForSessionDefinition(USessionDefinition* SessionDefinition){ return FName(); }
FSessionProfilePath UFGSessionSettings::MakeSessionProfilePathFromDefinitionAndProfile(USessionDefinition* SessionDefinition, const FSessionSettingsProfile &Profile) const{ return FSessionProfilePath(); }
FSessionProfilePath UFGSessionSettings::MakeSessionProfilePathFromDefinitionAndProfileName(USessionDefinition* SessionDefinition, FName ProfileName) const{ return FSessionProfilePath(); }
USessionDefinition* UFGSessionSettings::SessionDefinitionFromProfilePath(const FSessionProfilePath& SessionProfile) const{ return nullptr; }
FName UFGSessionSettings::SessionProfileNameFromProfilePath(const FSessionProfilePath& SessionProfile) const{ return FName(); }
FSessionSettingsProfile UFGSessionSettings::SessionProfileFromProfilePath(const FSessionProfilePath& SessionProfile) const{ return FSessionSettingsProfile(); }
FDelegateHandle UFGSessionSettings::AddOnActiveSessionDefinitionChangedDelegate(FOnSessionDefinitionChanged::FDelegate&& Delegate){ return FDelegateHandle(); }
FDelegateHandle UFGSessionSettings::AddOnCurrentSessionDefinitionChangedDelegate(FOnSessionDefinitionChanged::FDelegate&& Delegate){ return FDelegateHandle(); }
void UFGSessionSettings::PostLoadMap(UWorld* world){ }
void UFGSessionSettings::SetActiveSessionDefinition(USessionDefinition* SessionDefinition){ }