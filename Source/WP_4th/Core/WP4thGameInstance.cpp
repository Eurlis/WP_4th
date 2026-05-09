#include "WP4thGameInstance.h"
#include "SessionManager.h"

UWP4thGameInstance::UWP4thGameInstance()
{
}

void UWP4thGameInstance::Init()
{
	Super::Init();

	SessionManager = NewObject<USessionManager>(this, USessionManager::StaticClass(), TEXT("SessionManager"));
	if (SessionManager)
	{
		SessionManager->Init();
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[WP4thGameInstance] Failed to create SessionManager"));
	}
}
