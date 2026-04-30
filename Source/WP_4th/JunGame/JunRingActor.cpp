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

void AJunRingActor::StopRing()
{
	if (RingComponent)
	{
		RingComponent->StopRing();
	}
}

void AJunRingActor::PauseRing()
{
	if (RingComponent)
	{
		RingComponent->PauseRing();
	}
}

void AJunRingActor::ResumeRing()
{
	if (RingComponent)
	{
		RingComponent->ResumeRing();
	}
}

void AJunRingActor::ResetRing()
{
	if (RingComponent)
	{
		RingComponent->ResetRing();
	}
}

void AJunRingActor::ResetForRound()
{
	if (RingComponent)
	{
		RingComponent->ResetForRound();
	}
}

bool AJunRingActor::AdvanceToPhase(int32 PhaseIndex)
{
	return RingComponent ? RingComponent->AdvanceToPhase(PhaseIndex) : false;
}

bool AJunRingActor::ReloadRingData()
{
	return RingComponent ? RingComponent->ReloadRingData() : false;
}

float AJunRingActor::GetCurrentRadius() const
{
	return RingComponent ? RingComponent->GetCurrentRadius() : 0.f;
}

float AJunRingActor::GetTargetRadius() const
{
	return RingComponent ? RingComponent->GetTargetRadius() : 0.f;
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

bool AJunRingActor::IsRingPaused() const
{
	return RingComponent ? RingComponent->IsRingPaused() : false;
}

bool AJunRingActor::IsRingActive() const
{
	return RingComponent ? RingComponent->IsRingActive() : false;
}

FVector AJunRingActor::GetRingCenter() const
{
	return RingComponent ? RingComponent->GetRingCenter() : FVector::ZeroVector;
}

FVector AJunRingActor::GetTargetRingCenter() const
{
	return RingComponent ? RingComponent->GetTargetRingCenter() : FVector::ZeroVector;
}

EJunRingPhaseState AJunRingActor::GetRingPhaseState() const
{
	return RingComponent ? RingComponent->GetRingPhaseState() : EJunRingPhaseState::Inactive;
}

float AJunRingActor::GetPhaseTimeRemaining() const
{
	return RingComponent ? RingComponent->GetPhaseTimeRemaining() : 0.f;
}
