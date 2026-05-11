// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/ApexCharacterBase.h"
#include "OctaneLaunchPad.h"
#include "Octane.generated.h"

UCLASS()
class WP_4TH_API AOctane : public AApexCharacterBase
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AOctane();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void ActivateTactical() override; // Stim
	virtual void ActivateUltimate() override; // Launch Pad

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
	UPROPERTY(EditDefaultsOnly, Category="Stim")
	float StimHPCost = 20.f;

	UPROPERTY(EditDefaultsOnly, Category="Stim")
	float StimDuration = 6.f;

	UPROPERTY(EditDefaultsOnly, Category="Stim")
	float StimCooldown = 1.f;

	UPROPERTY(EditDefaultsOnly, Category="Stim")
	float StimWalkMultiplier = 1.3f;

	UPROPERTY(EditDefaultsOnly, Category="Stim")
	float StimSprintMultiplier = 1.4f;

	UPROPERTY(ReplicatedUsing=OnRep_bStimActive, BlueprintReadOnly, Category="Stim")
	bool bStimActive = false;

private:
	bool bStimOnCooldown = false;

	FTimerHandle StimDurationHandle;
	FTimerHandle StimCooldownHandle;

	UFUNCTION()
	void OnRep_bStimActive();

	UFUNCTION(Server, Reliable)
	void Server_ActivateStim();

	void EnterStim();
	void ExitStim();

public:
	UPROPERTY(EditDefaultsOnly, Category="Passive")
	float RegenAmount = 1.f;

	UPROPERTY(EditDefaultsOnly, Category="Passive")
	float RegenInterval = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category="Passive")
	float RegenDelay = 5.f;

	FTimerHandle RegenTickHandle;
	FTimerHandle RegenDelayHandle;

	void StartRegen();
	void StopRegen();
	void RegenTick();

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, class AController* EventInstigator, AActor* DamageCauser) override;

public:
	UPROPERTY(EditDefaultsOnly, Category="Ultimate")
	TSubclassOf<AOctaneLaunchPad> LaunchPadClass;

	UPROPERTY(EditDefaultsOnly, Category="Ultimate")
	float UltimateCooldown = 90.f;

private:
	bool bUltimateOnCooldown = false;
	FTimerHandle UltimateCooldownHandle;

	UFUNCTION(Server, Reliable)
	void Server_ActivateUltimate();
};
