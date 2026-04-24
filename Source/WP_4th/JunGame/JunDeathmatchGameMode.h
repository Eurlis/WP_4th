#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameFramework/GameMode.h"
#include "JunGame/JunBalanceData.h"
#include "JunDeathmatchGameMode.generated.h"

class AJunRingActor;
class AJunDeathmatchGameState;
class AJunDeathmatchPlayerState;
class AController;
class APawn;
class UJunPawnDeathListener;

UCLASS()
class WP_4TH_API AJunDeathmatchGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AJunDeathmatchGameMode();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Deathmatch")
	void RegisterKill(AController* KillerController, AController* VictimController);

	UFUNCTION(BlueprintCallable, Category = "Deathmatch")
	void RequestRespawn(AController* EliminatedController);

	UFUNCTION(BlueprintCallable, Category = "Deathmatch")
	void HandleObservedPawnDeath(APawn* EliminatedPawn);

protected:
	virtual void HandleMatchHasStarted() override;
	virtual void HandleMatchHasEnded() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deathmatch")
	int32 TargetKillCount;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deathmatch")
	float MatchStartDelay;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deathmatch")
	float PostMatchDelay;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deathmatch")
	float RespawnDelay;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deathmatch")
	bool bAllowRespawn;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deathmatch|Balance")
	UDataTable* DeathmatchSettingsDataTable;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deathmatch|Balance")
	FName DeathmatchSettingsRowName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deathmatch")
	TSubclassOf<AJunRingActor> RingActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Deathmatch")
	bool bStartRingOnBeginPlay;

	UPROPERTY()
	AJunRingActor* RingActorInstance;

private:
	void StartDeathmatch();
	void LoadDeathmatchSettings();
	void SpawnRingActor();
	void FinishMatch(AController* WinningController);
	void RespawnController(AController* EliminatedController);
	void HandleSpawnedActor(AActor* SpawnedActor);
	void RegisterObservedPawn(APawn* Pawn);
	void RefreshLeaderState();
	void SyncGameStateFromConfig();
	void SyncRingStateToGameState();
	void HandleRingDamageApplied(APawn* DamagedPawn);

	UFUNCTION()
	void HandlePostMatchCleanup();

	TMap<TObjectPtr<AController>, int32> KillCounts;
	TMap<TObjectPtr<APawn>, TObjectPtr<AController>> PawnControllerCache;
	TMap<TObjectPtr<APawn>, float> LastRingDamageTimestamps;
	TSet<TObjectPtr<APawn>> RegisteredPawns;

	UPROPERTY()
	TArray<TObjectPtr<UJunPawnDeathListener>> DeathListeners;

	FDelegateHandle ActorSpawnedHandle;
	FTimerHandle StartMatchTimerHandle;
	FTimerHandle RingStateSyncTimerHandle;
	FTimerHandle PostMatchCleanupTimerHandle;
};
