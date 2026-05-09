#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "SessionManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSM_CreateSessionComplete, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSM_FindSessionsComplete, bool, bSuccess, int32, NumFound);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSM_JoinSessionComplete, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSM_DestroySessionComplete, bool, bSuccess);

USTRUCT(BlueprintType)
struct FSessionSearchResultBP
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Session")
	FString HostName;

	UPROPERTY(BlueprintReadOnly, Category="Session")
	int32 MaxPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category="Session")
	int32 CurrentPlayers = 0;

	UPROPERTY(BlueprintReadOnly, Category="Session")
	int32 PingMs = 0;

	UPROPERTY(BlueprintReadOnly, Category="Session")
	int32 SearchIndex = -1;
};

UCLASS(BlueprintType)
class WP_4TH_API USessionManager : public UObject
{
	GENERATED_BODY()

public:
	USessionManager();

	void Init();

	UFUNCTION(BlueprintCallable, Category="Session")
	void CreateSession(const FString& HostName, int32 MaxPlayers);

	UFUNCTION(BlueprintCallable, Category="Session")
	void FindSessions(int32 MaxResults = 20);

	UFUNCTION(BlueprintCallable, Category="Session")
	void JoinSessionByIndex(int32 SessionIndex);

	UFUNCTION(BlueprintCallable, Category="Session")
	void DestroySession();

	UFUNCTION(BlueprintPure, Category="Session")
	const TArray<FSessionSearchResultBP>& GetSearchResults() const { return CachedSearchResults; }

	UPROPERTY(BlueprintAssignable, Category="Session")
	FOnSM_CreateSessionComplete OnCreateSessionComplete;

	UPROPERTY(BlueprintAssignable, Category="Session")
	FOnSM_FindSessionsComplete OnFindSessionsComplete;

	UPROPERTY(BlueprintAssignable, Category="Session")
	FOnSM_JoinSessionComplete OnJoinSessionComplete;

	UPROPERTY(BlueprintAssignable, Category="Session")
	FOnSM_DestroySessionComplete OnDestroySessionComplete;

private:
	IOnlineSessionPtr GetSessionInterface() const;

	void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleFindSessionsComplete(bool bWasSuccessful);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	void StartCreateSessionInternal();

	FDelegateHandle CreateHandle;
	FDelegateHandle FindHandle;
	FDelegateHandle JoinHandle;
	FDelegateHandle DestroyHandle;

	TSharedPtr<FOnlineSessionSearch> SessionSearch;

	UPROPERTY()
	TArray<FSessionSearchResultBP> CachedSearchResults;

	FString PendingHostName;
	int32 PendingMaxPlayers = 0;
	bool bRecreateAfterDestroy = false;
};
