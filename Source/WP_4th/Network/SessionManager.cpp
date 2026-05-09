#include "SessionManager.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Online/OnlineSessionNames.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const FName SessionKey_HostName(TEXT("HOSTNAME"));
}

USessionManager::USessionManager()
{
}

void USessionManager::Init()
{
	IOnlineSessionPtr Session = GetSessionInterface();
	if (!Session.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SessionManager] Init: SessionInterface not valid"));
		return;
	}
	UE_LOG(LogTemp, Log, TEXT("[SessionManager] Init OK"));
}

IOnlineSessionPtr USessionManager::GetSessionInterface() const
{
	IOnlineSubsystem* OSS = Online::GetSubsystem(GetWorld());
	if (!OSS)
	{
		return nullptr;
	}
	return OSS->GetSessionInterface();
}

void USessionManager::CreateSession(const FString& HostName, int32 MaxPlayers)
{
	IOnlineSessionPtr Session = GetSessionInterface();
	if (!Session.IsValid())
	{
		OnCreateSessionComplete.Broadcast(false);
		return;
	}

	PendingHostName = HostName;
	PendingMaxPlayers = FMath::Max(1, MaxPlayers);

	if (Session->GetNamedSession(NAME_GameSession) != nullptr)
	{
		bRecreateAfterDestroy = true;
		DestroySession();
		return;
	}

	StartCreateSessionInternal();
}

void USessionManager::StartCreateSessionInternal()
{
	IOnlineSessionPtr Session = GetSessionInterface();
	if (!Session.IsValid())
	{
		OnCreateSessionComplete.Broadcast(false);
		return;
	}

	FOnlineSessionSettings Settings;
	Settings.bIsLANMatch = true;
	Settings.bShouldAdvertise = true;
	Settings.bAllowJoinInProgress = true;
	Settings.bUsesPresence = true;
	Settings.bUseLobbiesIfAvailable = true;
	Settings.bAllowJoinViaPresence = true;
	Settings.NumPublicConnections = PendingMaxPlayers;
	Settings.NumPrivateConnections = 0;

	Settings.Set(SessionKey_HostName, PendingHostName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	if (CreateHandle.IsValid())
	{
		Session->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
	}
	CreateHandle = Session->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &USessionManager::HandleCreateSessionComplete));

	const ULocalPlayer* LocalPlayer = GetWorld() ? GetWorld()->GetFirstLocalPlayerFromController() : nullptr;
	const FUniqueNetIdRepl NetId = LocalPlayer ? LocalPlayer->GetPreferredUniqueNetId() : FUniqueNetIdRepl();

	const bool bStarted = NetId.IsValid()
		? Session->CreateSession(*NetId, NAME_GameSession, Settings)
		: Session->CreateSession(0, NAME_GameSession, Settings);

	if (!bStarted)
	{
		Session->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
		CreateHandle.Reset();
		OnCreateSessionComplete.Broadcast(false);
	}
}

void USessionManager::HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSessionPtr Session = GetSessionInterface();
	if (Session.IsValid() && CreateHandle.IsValid())
	{
		Session->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
		CreateHandle.Reset();
	}
	UE_LOG(LogTemp, Log, TEXT("[SessionManager] CreateSession complete: %s"), bWasSuccessful ? TEXT("OK") : TEXT("FAIL"));
	OnCreateSessionComplete.Broadcast(bWasSuccessful);
}

void USessionManager::FindSessions(int32 MaxResults)
{
	IOnlineSessionPtr Session = GetSessionInterface();
	if (!Session.IsValid())
	{
		OnFindSessionsComplete.Broadcast(false, 0);
		return;
	}

	SessionSearch = MakeShared<FOnlineSessionSearch>();
	SessionSearch->MaxSearchResults = FMath::Max(1, MaxResults);
	SessionSearch->bIsLanQuery = true;
	SessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

	if (FindHandle.IsValid())
	{
		Session->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
	}
	FindHandle = Session->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &USessionManager::HandleFindSessionsComplete));

	const ULocalPlayer* LocalPlayer = GetWorld() ? GetWorld()->GetFirstLocalPlayerFromController() : nullptr;
	const FUniqueNetIdRepl NetId = LocalPlayer ? LocalPlayer->GetPreferredUniqueNetId() : FUniqueNetIdRepl();

	const bool bStarted = NetId.IsValid()
		? Session->FindSessions(*NetId, SessionSearch.ToSharedRef())
		: Session->FindSessions(0, SessionSearch.ToSharedRef());

	if (!bStarted)
	{
		Session->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
		FindHandle.Reset();
		OnFindSessionsComplete.Broadcast(false, 0);
	}
}

void USessionManager::HandleFindSessionsComplete(bool bWasSuccessful)
{
	IOnlineSessionPtr Session = GetSessionInterface();
	if (Session.IsValid() && FindHandle.IsValid())
	{
		Session->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
		FindHandle.Reset();
	}

	CachedSearchResults.Reset();

	int32 NumFound = 0;
	if (bWasSuccessful && SessionSearch.IsValid())
	{
		const TArray<FOnlineSessionSearchResult>& Results = SessionSearch->SearchResults;
		NumFound = Results.Num();

		for (int32 i = 0; i < Results.Num(); ++i)
		{
			const FOnlineSessionSearchResult& R = Results[i];
			FSessionSearchResultBP Item;
			Item.SearchIndex = i;
			Item.PingMs = R.PingInMs;
			Item.MaxPlayers = R.Session.SessionSettings.NumPublicConnections;
			Item.CurrentPlayers = R.Session.SessionSettings.NumPublicConnections - R.Session.NumOpenPublicConnections;

			FString Host;
			if (R.Session.SessionSettings.Get(SessionKey_HostName, Host))
			{
				Item.HostName = Host;
			}
			else
			{
				Item.HostName = R.Session.OwningUserName;
			}

			CachedSearchResults.Add(Item);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[SessionManager] FindSessions complete: %s, Found=%d"),
		bWasSuccessful ? TEXT("OK") : TEXT("FAIL"), NumFound);
	OnFindSessionsComplete.Broadcast(bWasSuccessful, NumFound);
}

void USessionManager::JoinSessionByIndex(int32 SessionIndex)
{
	IOnlineSessionPtr Session = GetSessionInterface();
	if (!Session.IsValid() || !SessionSearch.IsValid())
	{
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	if (!SessionSearch->SearchResults.IsValidIndex(SessionIndex))
	{
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	if (JoinHandle.IsValid())
	{
		Session->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
	}
	JoinHandle = Session->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &USessionManager::HandleJoinSessionComplete));

	const ULocalPlayer* LocalPlayer = GetWorld() ? GetWorld()->GetFirstLocalPlayerFromController() : nullptr;
	const FUniqueNetIdRepl NetId = LocalPlayer ? LocalPlayer->GetPreferredUniqueNetId() : FUniqueNetIdRepl();

	const bool bStarted = NetId.IsValid()
		? Session->JoinSession(*NetId, NAME_GameSession, SessionSearch->SearchResults[SessionIndex])
		: Session->JoinSession(0, NAME_GameSession, SessionSearch->SearchResults[SessionIndex]);

	if (!bStarted)
	{
		Session->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
		JoinHandle.Reset();
		OnJoinSessionComplete.Broadcast(false);
	}
}

void USessionManager::HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSessionPtr Session = GetSessionInterface();
	if (Session.IsValid() && JoinHandle.IsValid())
	{
		Session->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
		JoinHandle.Reset();
	}

	const bool bSuccess = (Result == EOnJoinSessionCompleteResult::Success);
	UE_LOG(LogTemp, Log, TEXT("[SessionManager] JoinSession complete: %s (Result=%d)"),
		bSuccess ? TEXT("OK") : TEXT("FAIL"), (int32)Result);

	if (bSuccess && Session.IsValid())
	{
		FString ConnectString;
		if (Session->GetResolvedConnectString(SessionName, ConnectString))
		{
			APlayerController* PC = GetWorld() ? UGameplayStatics::GetPlayerController(GetWorld(), 0) : nullptr;
			if (PC)
			{
				UE_LOG(LogTemp, Log, TEXT("[SessionManager] ClientTravel -> %s"), *ConnectString);
				PC->ClientTravel(ConnectString, TRAVEL_Absolute);
				OnJoinSessionComplete.Broadcast(true);
				return;
			}
		}
		UE_LOG(LogTemp, Warning, TEXT("[SessionManager] JoinSession success but no connect string / PC"));
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	OnJoinSessionComplete.Broadcast(false);
}

void USessionManager::DestroySession()
{
	IOnlineSessionPtr Session = GetSessionInterface();
	if (!Session.IsValid())
	{
		bRecreateAfterDestroy = false;
		OnDestroySessionComplete.Broadcast(false);
		return;
	}

	if (DestroyHandle.IsValid())
	{
		Session->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
	}
	DestroyHandle = Session->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &USessionManager::HandleDestroySessionComplete));

	if (!Session->DestroySession(NAME_GameSession))
	{
		Session->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
		DestroyHandle.Reset();
		bRecreateAfterDestroy = false;
		OnDestroySessionComplete.Broadcast(false);
	}
}

void USessionManager::HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	IOnlineSessionPtr Session = GetSessionInterface();
	if (Session.IsValid() && DestroyHandle.IsValid())
	{
		Session->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
		DestroyHandle.Reset();
	}
	UE_LOG(LogTemp, Log, TEXT("[SessionManager] DestroySession complete: %s"), bWasSuccessful ? TEXT("OK") : TEXT("FAIL"));

	OnDestroySessionComplete.Broadcast(bWasSuccessful);

	if (bRecreateAfterDestroy)
	{
		bRecreateAfterDestroy = false;
		if (bWasSuccessful)
		{
			StartCreateSessionInternal();
		}
		else
		{
			OnCreateSessionComplete.Broadcast(false);
		}
	}
}
