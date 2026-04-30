#include "JunGame/JunDeathmatchPlayerState.h"

#include "Net/UnrealNetwork.h"

AJunDeathmatchPlayerState::AJunDeathmatchPlayerState()
{
	Eliminations = 0;
	Deaths = 0;
	RespawnCount = 0;
}

void AJunDeathmatchPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AJunDeathmatchPlayerState, Eliminations);
	DOREPLIFETIME(AJunDeathmatchPlayerState, Deaths);
	DOREPLIFETIME(AJunDeathmatchPlayerState, RespawnCount);
}

void AJunDeathmatchPlayerState::RegisterElimination()
{
	++Eliminations;
	SetScore(static_cast<float>(Eliminations));
}

void AJunDeathmatchPlayerState::RegisterDeath()
{
	++Deaths;
}

void AJunDeathmatchPlayerState::RegisterRespawn()
{
	++RespawnCount;
}
