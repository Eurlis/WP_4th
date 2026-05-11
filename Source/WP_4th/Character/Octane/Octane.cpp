// Fill out your copyright notice in the Description page of Project Settings.


#include "Octane.h"

#include "Character/Components/HPComp/HealthComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"


// Sets default values
AOctane::AOctane()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	ConstructorHelpers::FObjectFinder<USkeletalMesh> OctaneMesh(TEXT("/Game/Models/Octane/Octane/ocatane.ocatane"));
	if (OctaneMesh.Succeeded())
	{
		FirstPersonMesh->SetSkeletalMesh(OctaneMesh.Object);
		GetMesh()->SetSkeletalMesh(OctaneMesh.Object);
	}

	// 2. 메시 Transform (Wraith랑 동일하게 시작)
	GetMesh()->SetRelativeLocationAndRotation(FVector(0, 0, -100.f), FRotator(0, -90.f, 0));

	// 3. ABP 연결
	ConstructorHelpers::FClassFinder<UAnimInstance> ABP_Octane(TEXT("/Game/BluePrints/ABP/ABP_Octane.ABP_Octane"));
	if (ABP_Octane.Succeeded())
		GetMesh()->SetAnimInstanceClass(ABP_Octane.Class);

}

// Called when the game starts or when spawned
void AOctane::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		StartRegen();
	}

}

void AOctane::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AOctane, bStimActive);
}

void AOctane::ActivateTactical()
{
	Server_ActivateStim();
}

void AOctane::ActivateUltimate()
{
	if (bUltimateOnCooldown) return;
	Server_ActivateUltimate();
}

// Called every frame
void AOctane::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void AOctane::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AOctane::OnRep_bStimActive()
{
	UE_LOG(LogTemp, Warning, TEXT("Stim: %s"), bStimActive ? TEXT("ON") : TEXT("OFF"));
}

void AOctane::EnterStim()
{
	bStimActive = true;
	bStimOnCooldown = true;

	SpeedMultiplier = bIsSprinting ? StimSprintMultiplier : StimWalkMultiplier;
	GetCharacterMovement()->MaxWalkSpeed = (bIsSprinting ? SprintSpeed : WalkSpeed) * SpeedMultiplier;

	UE_LOG(LogTemp, Warning, TEXT("[Stim Enter] Speed: %.1f → %.1f (x%.2f) | Sprinting: %s"),
		bIsSprinting ? SprintSpeed : WalkSpeed,
		GetCharacterMovement()->MaxWalkSpeed,
		SpeedMultiplier,
		bIsSprinting ? TEXT("true") : TEXT("false"));

	GetWorldTimerManager().SetTimer(
		StimDurationHandle,
		this,
		&AOctane::ExitStim,
		StimDuration,
		false);

	GetWorldTimerManager().SetTimer(
		StimCooldownHandle,
		FTimerDelegate::CreateLambda([this]() {bStimOnCooldown = false;}),
		StimCooldown,
		false);
}

void AOctane::ExitStim()
{
	float Before = GetCharacterMovement()->MaxWalkSpeed;
	SpeedMultiplier = 1.0f;
	bStimActive = false;
	GetCharacterMovement()->MaxWalkSpeed = bIsSprinting ? SprintSpeed : WalkSpeed;
	SavedDefaultWalkSpeed = GetCharacterMovement()->MaxWalkSpeed;
	UE_LOG(LogTemp, Warning, TEXT("[Stim Exit] Speed: %.1f → %.1f"),
		Before, GetCharacterMovement()->MaxWalkSpeed);

}

void AOctane::StartRegen()
{

	GetWorldTimerManager().SetTimer(
		RegenTickHandle,
		this,
		&AOctane::RegenTick,
		RegenInterval,
		true);
}

void AOctane::StopRegen()
{
	GetWorldTimerManager().ClearTimer(RegenTickHandle);
}

void AOctane::RegenTick()
{
	if (!HealthComponent || HealthComponent->IsDead()) return;
	if (HealthComponent->Health >= HealthComponent->MaxHealth) return;

	HealthComponent->Health = FMath::Min(HealthComponent->Health + RegenAmount, HealthComponent->MaxHealth);

	HealthComponent->OnHealthChanged.Broadcast(HealthComponent->Health, HealthComponent->MaxHealth);
}

float AOctane::TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
	class AController* EventInstigator, AActor* DamageCauser)
{
	float Result = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (HasAuthority() && DamageAmount > 0.f)
	{
		StopRegen();

		GetWorldTimerManager().SetTimer(
			RegenDelayHandle,
			this,
			&AOctane::StartRegen,
			RegenDelay,
			false);
	}
	return Result;
}

void AOctane::Server_ActivateUltimate_Implementation()
{

	if (bUltimateOnCooldown || !LaunchPadClass) return;

	FVector SpawnLoc = GetActorLocation() - FVector(0, 0, 90.f);
	FRotator SpawnRot = FRotator(0, GetActorRotation().Yaw, 0);

	GetWorld()->SpawnActor<AOctaneLaunchPad>(LaunchPadClass, SpawnLoc, SpawnRot);

	bUltimateOnCooldown = true;
	GetWorldTimerManager().SetTimer(
		UltimateCooldownHandle,
		FTimerDelegate::CreateLambda([this]() {bUltimateOnCooldown = false;}),
		UltimateCooldown, false);
}

void AOctane::Server_ActivateStim_Implementation()
{
	if (bStimActive || bStimOnCooldown)
		return;

	if (!HealthComponent || HealthComponent->Health <= 1.f)
		return;

	float HpCost = FMath::Min(StimHPCost, HealthComponent->Health - 1.f);
	HealthComponent->Health -= HpCost;
	HealthComponent->OnHealthChanged.Broadcast(HealthComponent->Health, HealthComponent->MaxHealth);

	EnterStim();
}

