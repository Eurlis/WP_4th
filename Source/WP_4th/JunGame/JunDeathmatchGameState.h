#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "JunGame/JunRingComponent.h"
#include "JunDeathmatchGameState.generated.h"

class AJunDeathmatchPlayerState;
class APlayerState;

UENUM(BlueprintType)
enum class EJunMatchPhase : uint8
{
	Warmup,
	InProgress,
	Completed
};

UCLASS()
class WP_4TH_API AJunDeathmatchGameState : public AGameState
{
	GENERATED_BODY()

public:
	AJunDeathmatchGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void SetMatchPhase(EJunMatchPhase NewPhase);
	void SetTargetKillCount(int32 NewTargetKillCount);
	void SetCurrentLeader(AJunDeathmatchPlayerState* NewLeader);
	void SetWinningPlayerState(APlayerState* NewWinner);
	void SetRingState(int32 NewRingPhase, float NewRingRadius, bool bNewRingIsShrinking, EJunRingPhaseState NewRingPhaseState, float NewRingPhaseTimeRemaining, const FVector& NewRingCenter);

	UFUNCTION(BlueprintPure, Category = "Deathmatch")
	EJunMatchPhase GetMatchPhase() const { return MatchPhase; }

protected:
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Deathmatch")
	EJunMatchPhase MatchPhase;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Deathmatch")
	int32 TargetKillCount;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Deathmatch")
	TObjectPtr<AJunDeathmatchPlayerState> CurrentLeader;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Deathmatch")
	TObjectPtr<APlayerState> WinningPlayerState;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ring")
	int32 CurrentRingPhase;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ring")
	float CurrentRingRadius;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ring")
	bool bRingIsShrinking;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ring")
	EJunRingPhaseState RingPhaseState;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ring")
	float RingPhaseTimeRemaining;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Ring")
	FVector RingCenter;
};
