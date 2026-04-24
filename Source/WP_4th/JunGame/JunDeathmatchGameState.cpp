#include "JunGame/JunDeathmatchGameState.h"

#include "Net/UnrealNetwork.h"

AJunDeathmatchGameState::AJunDeathmatchGameState()
{
	MatchPhase = EJunMatchPhase::Warmup;
	TargetKillCount = 0;
	CurrentLeader = nullptr;
	WinningPlayerState = nullptr;
	CurrentRingPhase = INDEX_NONE;
	CurrentRingRadius = 0.f;
	bRingIsShrinking = false;
	RingPhaseState = EJunRingPhaseState::Inactive;
	RingPhaseTimeRemaining = 0.f;
	RingCenter = FVector::ZeroVector;
}

void AJunDeathmatchGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AJunDeathmatchGameState, MatchPhase);
	DOREPLIFETIME(AJunDeathmatchGameState, TargetKillCount);
	DOREPLIFETIME(AJunDeathmatchGameState, CurrentLeader);
	DOREPLIFETIME(AJunDeathmatchGameState, WinningPlayerState);
	DOREPLIFETIME(AJunDeathmatchGameState, CurrentRingPhase);
	DOREPLIFETIME(AJunDeathmatchGameState, CurrentRingRadius);
	DOREPLIFETIME(AJunDeathmatchGameState, bRingIsShrinking);
	DOREPLIFETIME(AJunDeathmatchGameState, RingPhaseState);
	DOREPLIFETIME(AJunDeathmatchGameState, RingPhaseTimeRemaining);
	DOREPLIFETIME(AJunDeathmatchGameState, RingCenter);
}

void AJunDeathmatchGameState::SetMatchPhase(EJunMatchPhase NewPhase)
{
	MatchPhase = NewPhase;
}

void AJunDeathmatchGameState::SetTargetKillCount(int32 NewTargetKillCount)
{
	TargetKillCount = NewTargetKillCount;
}

void AJunDeathmatchGameState::SetCurrentLeader(AJunDeathmatchPlayerState* NewLeader)
{
	CurrentLeader = NewLeader;
}

void AJunDeathmatchGameState::SetWinningPlayerState(APlayerState* NewWinner)
{
	WinningPlayerState = NewWinner;
}

void AJunDeathmatchGameState::SetRingState(int32 NewRingPhase, float NewRingRadius, bool bNewRingIsShrinking, EJunRingPhaseState NewRingPhaseState, float NewRingPhaseTimeRemaining, const FVector& NewRingCenter)
{
	CurrentRingPhase = NewRingPhase;
	CurrentRingRadius = NewRingRadius;
	bRingIsShrinking = bNewRingIsShrinking;
	RingPhaseState = NewRingPhaseState;
	RingPhaseTimeRemaining = NewRingPhaseTimeRemaining;
	RingCenter = NewRingCenter;
}
