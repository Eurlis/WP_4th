#include "ApexDeathmatchGameState.h"

#include "Net/UnrealNetwork.h"

AApexDeathmatchGameState::AApexDeathmatchGameState()
{
}

void AApexDeathmatchGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AApexDeathmatchGameState, MatchStartServerTime);
	DOREPLIFETIME(AApexDeathmatchGameState, MatchDurationReplicated);
}

float AApexDeathmatchGameState::GetRemainingMatchTime() const
{
	if (MatchDurationReplicated <= 0.0f)
	{
		return MatchDurationReplicated;
	}

	const float ServerTime = GetServerWorldTimeSeconds();
	const float Elapsed = ServerTime - MatchStartServerTime;
	const float Remaining = MatchDurationReplicated - Elapsed;

	return FMath::Max(Remaining, 0.0f);
}

bool AApexDeathmatchGameState::IsMatchActive() const
{
	return MatchDurationReplicated > 0.0f && GetRemainingMatchTime() > 0.0f;
}
