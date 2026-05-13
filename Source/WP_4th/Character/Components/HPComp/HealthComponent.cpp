#include "HealthComponent.h"
#include "Net/UnrealNetwork.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	MaxHealth = 100.f;
	MaxShield = 100.f;
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth;
	Shield = MaxShield;
}

void UHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UHealthComponent, Health);
	DOREPLIFETIME(UHealthComponent, Shield);
}

EHitSoundType UHealthComponent::ApplyDamage(float RawDamage, bool bIsHeadshot)
{
	if (!GetOwner()->HasAuthority()) return EHitSoundType::FleshHit;
	if (IsDead()) return EHitSoundType::FleshHit;

	EHitSoundType HitType = EHitSoundType::FleshHit;
	float FinalDamage = RawDamage;

	if (Shield > 0.f)
	{
		float ShieldDamage = FMath::Min(Shield, FinalDamage);
		Shield -= ShieldDamage;
		FinalDamage -= ShieldDamage;
		OnShieldChanged.Broadcast(Shield, MaxShield);

		HitType = (Shield <= 0.f) ? EHitSoundType::ShieldBroken : EHitSoundType::ShieldHit;
	}

	Health = FMath::Max(0.f, Health - FinalDamage);
	OnHealthChanged.Broadcast(Health, MaxHealth);

	if (IsDead())
	{
		HitType = EHitSoundType::Downed;
		OnDeath.Broadcast();
	}

	return HitType;
}

void UHealthComponent::ApplyHealthDamage(float RawDamage)
{
	if (!GetOwner()->HasAuthority()) return;
	if (IsDead()) return;

	Health = FMath::Max(0.f, Health - RawDamage);
	OnHealthChanged.Broadcast(Health, MaxHealth);

	if (IsDead())
	{
		OnDeath.Broadcast();
	}
}

bool UHealthComponent::IsDead() const
{
	return Health <= 0.f;
}

void UHealthComponent::OnRep_Health()
{
	OnHealthChanged.Broadcast(Health, MaxHealth);
}

void UHealthComponent::OnRep_Shield()
{
	OnShieldChanged.Broadcast(Shield, MaxShield);
}
