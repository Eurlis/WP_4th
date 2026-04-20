#include "HealthComponent.h"
#include "Net/UnrealNetwork.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	MaxHealth = 100.f;
	MaxShield = 100.f;
	HeadshotMultiplier = 1.5f;
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

void UHealthComponent::ApplyDamage(float RawDamage, bool bIsHeadshot)
{
	if (!GetOwner()->HasAuthority()) return;
	if (IsDead()) return;

	float FinalDamage = bIsHeadshot ? RawDamage * HeadshotMultiplier : RawDamage;

	if (Shield > 0.f)
	{
		float ShieldDamage = FMath::Min(Shield, FinalDamage);
		Shield -= ShieldDamage;
		FinalDamage -= ShieldDamage;
		OnShieldChanged.Broadcast(Shield, MaxShield);
	}

	Health = FMath::Max(0.f, Health - FinalDamage);
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
