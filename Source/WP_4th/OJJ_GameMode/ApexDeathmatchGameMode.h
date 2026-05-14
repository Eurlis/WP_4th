#pragma once

#include "CoreMinimal.h"
#include "JunGame/JunDeathmatchGameMode.h"
#include "Engine/EngineTypes.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "ApexDeathmatchGameMode.generated.h"

class ABulletPoolManager;

UCLASS(Blueprintable)
class WP_4TH_API AApexDeathmatchGameMode : public AJunDeathmatchGameMode
{
	GENERATED_BODY()

public:
	AApexDeathmatchGameMode();

	UFUNCTION(BlueprintCallable, Category = "Apex|Deathmatch")
	void HandleApexPawnKilled(AController* Killer, AController* Victim);

	// 부모 AJunDeathmatchGameMode::RequestRespawn 의 UFUNCTION 을 override 합니다.
	// UHT 규칙상 자식에서는 UFUNCTION() 재선언 불가 — Blueprint 카테고리는 부모 정의를 그대로 사용.
	void RequestRespawn(AController* EliminatedController);

protected:
	virtual void BeginPlay() override;
	virtual void HandleMatchHasStarted() override;
	virtual void HandleMatchHasEnded() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Apex|BulletPool")
	TSubclassOf<ABulletPoolManager> BulletPoolManagerClass;

	UPROPERTY()
	TObjectPtr<ABulletPoolManager> SpawnedBulletPool;

	UPROPERTY(EditDefaultsOnly, Category = "Apex|Respawn")
	float ApexRespawnDelay = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Apex|Respawn")
	bool bApexAllowRespawn = true;

	UPROPERTY(EditDefaultsOnly, Category = "Apex|Match")
	float MatchDuration = 300.0f;

	UFUNCTION(BlueprintImplementableEvent, Category = "Apex|Match")
	void BP_OnMatchEnded(AController* WinnerController, int32 WinnerKills);

private:
	void SpawnBulletPool();
	void RespawnController(AController* EliminatedController);
	void ClearAllRespawnTimers();
	void OnMatchTimeUp();

	TMap<TWeakObjectPtr<AController>, FTimerHandle> RespawnTimers;
	FTimerHandle MatchTimerHandle;
	bool bMatchTimeExpired = false;
};
