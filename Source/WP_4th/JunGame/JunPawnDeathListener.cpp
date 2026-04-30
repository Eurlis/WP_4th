#include "JunGame/JunPawnDeathListener.h"

#include "JunGame/JunDeathmatchGameMode.h"

void UJunPawnDeathListener::Initialize(AJunDeathmatchGameMode* InOwnerGameMode, APawn* InObservedPawn)
{
	OwnerGameMode = InOwnerGameMode;
	ObservedPawn = InObservedPawn;
}

void UJunPawnDeathListener::HandleObservedDeath()
{
	if (OwnerGameMode && ObservedPawn)
	{
		OwnerGameMode->HandleObservedPawnDeath(ObservedPawn);
	}
}
