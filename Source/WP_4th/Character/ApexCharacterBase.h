#pragma once

#include "CoreMinimal.h"
#include "WP_4thCharacter.h"
#include "ApexCharacterBase.generated.h"

class UHealthComponent;
class UInputAction;

UCLASS(abstract)
class WP_4TH_API AApexCharacterBase : public AWP_4thCharacter
{
	GENERATED_BODY()

public:
	AApexCharacterBase();

	// ─── Components ───────────────────────────────────────────────
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UHealthComponent* HealthComponent;

	// ─── Movement State ───────────────────────────────────────────
	UPROPERTY(ReplicatedUsing = OnRep_IsSprinting, BlueprintReadOnly, Category = "Movement")
	bool bIsSprinting;

	UPROPERTY(ReplicatedUsing = OnRep_IsSliding, BlueprintReadOnly, Category = "Movement")
	bool bIsSliding;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float SprintSpeed;

	UPROPERTY(EditDefaultsOnly, Category = "Movement")
	float WalkSpeed;

	// ─── Input Actions ────────────────────────────────────────────
	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* SprintAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* CrouchAction;

	UPROPERTY(EditAnywhere, Category = "Input")
	UInputAction* SlideAction;

	// ─── Damage ───────────────────────────────────────────────────
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
		AController* EventInstigator, AActor* DamageCauser) override;

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ─── Sprint ───────────────────────────────────────────────────
	void StartSprint();
	void StopSprint();

	UFUNCTION(Server, Reliable)
	void Server_StartSprint();

	UFUNCTION(Server, Reliable)
	void Server_StopSprint();

	UFUNCTION()
	void OnRep_IsSprinting();

	// ─── Slide ────────────────────────────────────────────────────
	void StartSlide();
	void StopSlide();

	UFUNCTION(Server, Reliable)
	void Server_StartSlide();

	UFUNCTION(Server, Reliable)
	void Server_StopSlide();

	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlaySlideAnim();

	UFUNCTION()
	void OnRep_IsSliding();

	// ─── Death ────────────────────────────────────────────────────
	UFUNCTION()
	void HandleDeath();

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnDeath();
};
