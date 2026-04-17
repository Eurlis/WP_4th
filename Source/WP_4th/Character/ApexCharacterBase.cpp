#include "ApexCharacterBase.h"
#include "Components/HealthComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "WP_4th.h"
#include "Engine/DamageEvents.h"

AApexCharacterBase::AApexCharacterBase()
{
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

	SprintSpeed = 700.f;
	WalkSpeed = 400.f;
}

void AApexCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	if (HasAuthority() && IsValid(HealthComponent))
	{
		HealthComponent->OnDeath.AddDynamic(this, &AApexCharacterBase::HandleDeath);
	}
}

void AApexCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (SprintAction)
		{
			EIC->BindAction(SprintAction, ETriggerEvent::Started,   this, &AApexCharacterBase::StartSprint);
			EIC->BindAction(SprintAction, ETriggerEvent::Completed, this, &AApexCharacterBase::StopSprint);
		}

		if (CrouchAction)
		{
			//EIC->BindAction(CrouchAction, ETriggerEvent::Started,   this, &AApexCharacterBase::Crouch);
			//EIC->BindAction(CrouchAction, ETriggerEvent::Completed, this, &AApexCharacterBase::UnCrouch);
		}

		if (SlideAction)
		{
			EIC->BindAction(SlideAction, ETriggerEvent::Started,   this, &AApexCharacterBase::StartSlide);
			EIC->BindAction(SlideAction, ETriggerEvent::Completed, this, &AApexCharacterBase::StopSlide);
		}
	}
}

void AApexCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AApexCharacterBase, bIsSprinting);
	DOREPLIFETIME(AApexCharacterBase, bIsSliding);
}

float AApexCharacterBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (!HasAuthority()) return 0.f;
	if (!IsValid(HealthComponent)) return 0.f;

	bool bIsHeadshot = false;
	if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
	{
		const FPointDamageEvent* PointDamage = static_cast<const FPointDamageEvent*>(&DamageEvent);
		bIsHeadshot = PointDamage->HitInfo.BoneName == FName("head");
	}

	HealthComponent->ApplyDamage(DamageAmount, bIsHeadshot);
	return DamageAmount;
}

// ─── Sprint ───────────────────────────────────────────────────────────────────

void AApexCharacterBase::StartSprint()
{
	if (!bIsSliding)
	{
		Server_StartSprint();
	}
}

void AApexCharacterBase::StopSprint()
{
	Server_StopSprint();
}

void AApexCharacterBase::Server_StartSprint_Implementation()
{
	bIsSprinting = true;
	GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
}

void AApexCharacterBase::Server_StopSprint_Implementation()
{
	bIsSprinting = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AApexCharacterBase::OnRep_IsSprinting()
{
	// 애니메이션 블루프린트 연동 시 여기서 처리
}

// ─── Slide ────────────────────────────────────────────────────────────────────

void AApexCharacterBase::StartSlide()
{
	if (bIsSprinting)
	{
		Server_StartSlide();
	}
}

void AApexCharacterBase::StopSlide()
{
	Server_StopSlide();
}

void AApexCharacterBase::Server_StartSlide_Implementation()
{
	bIsSliding = true;
	Multicast_PlaySlideAnim();
}

void AApexCharacterBase::Server_StopSlide_Implementation()
{
	bIsSliding = false;
}

void AApexCharacterBase::Multicast_PlaySlideAnim_Implementation()
{
	// 추후 AnimInstance 연동
}

void AApexCharacterBase::OnRep_IsSliding()
{
	// 클라이언트 연출 처리 (카메라 FOV 등)
}

// ─── Death ────────────────────────────────────────────────────────────────────

void AApexCharacterBase::HandleDeath()
{
	if (HasAuthority())
	{
		Multicast_OnDeath();
	}
}

void AApexCharacterBase::Multicast_OnDeath_Implementation()
{
	GetMesh()->SetSimulatePhysics(true);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (AController* PC = GetController())
	{
		PC->UnPossess();
	}
}
