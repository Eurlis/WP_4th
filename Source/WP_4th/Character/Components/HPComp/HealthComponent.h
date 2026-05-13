#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

UENUM(BlueprintType)
enum class EHitSoundType : uint8
{
	FleshHit UMETA(DisplayName="Flesh Hit"),
	ShieldHit UMETA(DisplayName="Shield Hit"),
	ShieldBroken UMETA(DisplayName="Shield Broken"),
	Downed UMETA(DisplayName="Downed"),
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, NewHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnShieldChanged, float, NewShield, float, MaxShield);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class WP_4TH_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHealthComponent();

	UPROPERTY(ReplicatedUsing = OnRep_Health, BlueprintReadOnly, Category = "Health")
	float Health;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
	float MaxHealth;

	UPROPERTY(ReplicatedUsing = OnRep_Shield, BlueprintReadOnly, Category = "Health")
	float Shield;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health")
	float MaxShield;


	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnDeath OnDeath;

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnShieldChanged OnShieldChanged;

	UFUNCTION(BlueprintCallable, Category = "Health")
	EHitSoundType ApplyDamage(float RawDamage, bool bIsHeadshot);

	UFUNCTION(BlueprintCallable, Category = "Health")
	void ApplyHealthDamage(float RawDamage);

	UFUNCTION(BlueprintCallable, Category = "Health")
	bool IsDead() const;

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_Health();

	UFUNCTION()
	void OnRep_Shield();
};
