#include "JunGame/JunRingActor.h"

AJunRingActor::AJunRingActor()
{
	bReplicates = true;
	SetReplicateMovement(false);

	RingComponent = CreateDefaultSubobject<UJunRingComponent>(TEXT("RingComponent"));
	if (RingComponent)
	{
		RingComponent->OnRingDamageApplied.AddLambda([this](APawn* DamagedPawn)
		{
			OnRingDamageApplied.Broadcast(DamagedPawn);
		});
	}
}

void AJunRingActor::StartRing()
{
	if (RingComponent)
	{
		RingComponent->StartRing();
	}
}

float AJunRingActor::GetCurrentRadius() const
{
	return RingComponent ? RingComponent->GetCurrentRadius() : 0.f;
}

int32 AJunRingActor::GetCurrentPhaseIndex() const
{
	return RingComponent ? RingComponent->GetCurrentPhaseIndex() : INDEX_NONE;
}

bool AJunRingActor::HasRingStarted() const
{
	return RingComponent ? RingComponent->HasRingStarted() : false;
}

bool AJunRingActor::IsRingShrinking() const
{
	return RingComponent ? RingComponent->IsRingShrinking() : false;
}

FVector AJunRingActor::GetRingCenter() const
{
	return RingComponent ? RingComponent->GetRingCenter() : FVector::ZeroVector;
}

EJunRingPhaseState AJunRingActor::GetRingPhaseState() const
{
	return RingComponent ? RingComponent->GetRingPhaseState() : EJunRingPhaseState::Inactive;
}

float AJunRingActor::GetPhaseTimeRemaining() const
{
	return RingComponent ? RingComponent->GetPhaseTimeRemaining() : 0.f;
}
