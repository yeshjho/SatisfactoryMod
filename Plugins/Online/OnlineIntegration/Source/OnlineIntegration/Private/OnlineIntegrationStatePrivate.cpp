// This file has been automatically generated by the Unreal Header Implementation tool

#include "OnlineIntegrationStatePrivate.h"

#include "GameplayTagContainerViewModel.h"

void UOnlineIntegrationStatePrivate::RegisterBackend(UOnlineIntegrationBackend* InBackend){ }
void UOnlineIntegrationStatePrivate::RegisterSessionDefinition(USessionDefinition* InSessionDefinition){ }
void UOnlineIntegrationStatePrivate::SetFirstLocalUserInfo(ULocalUserInfo* LocalUserInfo){ }
void UOnlineIntegrationStatePrivate::SetPendingSessionMigrationSequence(USessionMigrationSequence* SessionMigrationSequence){ }
UGameplayTagContainerViewModel& UOnlineIntegrationStatePrivate::GetMutableTagContainer(){ return *NewObject<UGameplayTagContainerViewModel>(); }