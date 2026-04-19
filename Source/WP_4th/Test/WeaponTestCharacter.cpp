// Fill out your copyright notice in the Description page of Project Settings.

#include "WeaponTestCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "WeaponBase.h"
#include "ThrowableBase.h"

AWeaponTestCharacter::AWeaponTestCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, BaseEyeHeight));
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	FirstPersonCamera->bUsePawnControlRotation = true;

	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh1P"));
	Mesh1P->SetupAttachment(FirstPersonCamera);
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;

	CurrentWeapon = nullptr;
}

void AWeaponTestCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}

	if (ARClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		CurrentWeapon = GetWorld()->SpawnActor<AWeaponBase>(ARClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
		if (CurrentWeapon)
		{
			UE_LOG(LogTemp, Warning, TEXT("[TestChar] Weapon spawned: %s"), *CurrentWeapon->GetName());

			CurrentWeapon->AttachToComponent(
				FirstPersonCamera,
				FAttachmentTransformRules::SnapToTargetIncludingScale,
				NAME_None);

			CurrentWeapon->SetActorRelativeLocation(FVector(30.0f, 15.0f, -10.0f));
			CurrentWeapon->SetActorRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));

			UE_LOG(LogTemp, Warning, TEXT("[TestChar] Weapon attached to camera"));

			CurrentWeapon->OwningCharacter = this;
			CurrentWeapon->OnEquipped();

			if (CurrentWeapon->WeaponMesh1P)
			{
				CurrentWeapon->WeaponMesh1P->SetVisibility(true);
				CurrentWeapon->WeaponMesh1P->SetOnlyOwnerSee(false);
			}
			if (CurrentWeapon->WeaponMesh3P)
			{
				CurrentWeapon->WeaponMesh3P->SetVisibility(false);
			}
		}
	}
}

void AWeaponTestCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput) return;

	if (MoveAction)
	{
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AWeaponTestCharacter::Move);
	}
	if (LookAction)
	{
		EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AWeaponTestCharacter::Look);
	}
	if (JumpAction)
	{
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	}
	if (FireAction)
	{
		EnhancedInput->BindAction(FireAction, ETriggerEvent::Started, this, &AWeaponTestCharacter::StartFire);
		EnhancedInput->BindAction(FireAction, ETriggerEvent::Completed, this, &AWeaponTestCharacter::StopFire);
	}
	if (ReloadAction)
	{
		EnhancedInput->BindAction(ReloadAction, ETriggerEvent::Triggered, this, &AWeaponTestCharacter::Reload);
	}
	if (SwitchARAction)
	{
		EnhancedInput->BindAction(SwitchARAction, ETriggerEvent::Triggered, this, &AWeaponTestCharacter::SwitchToAR);
	}
	if (SwitchPistolAction)
	{
		EnhancedInput->BindAction(SwitchPistolAction, ETriggerEvent::Triggered, this, &AWeaponTestCharacter::SwitchToPistol);
	}
	if (SwitchShotgunAction)
	{
		EnhancedInput->BindAction(SwitchShotgunAction, ETriggerEvent::Triggered, this, &AWeaponTestCharacter::SwitchToShotgun);
	}
	if (SwitchGrenadeAction)
	{
		EnhancedInput->BindAction(SwitchGrenadeAction, ETriggerEvent::Triggered, this, &AWeaponTestCharacter::SwitchToGrenade);
	}
	if (TurnAction)
	{
		EnhancedInput->BindAction(TurnAction, ETriggerEvent::Triggered, this, &AWeaponTestCharacter::Turn);
	}
	if (LookUpAction)
	{
		EnhancedInput->BindAction(LookUpAction, ETriggerEvent::Triggered, this, &AWeaponTestCharacter::LookUp);
	}
}

void AWeaponTestCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AWeaponTestCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	AddControllerYawInput(LookAxisVector.X);
	AddControllerPitchInput(LookAxisVector.Y);
}

void AWeaponTestCharacter::StartFire()
{
	if (CurrentSlot == EEquippedSlot::Grenade)
	{
		ThrowGrenade();

		// 던진 후 자동으로 무기 슬롯으로 복귀
		SwitchWeapon(ARClass);
		CurrentSlot = EEquippedSlot::Weapon;
	}
	else if (CurrentWeapon)
	{
		CurrentWeapon->StartFire();
	}
}

void AWeaponTestCharacter::StopFire()
{
	if (CurrentSlot == EEquippedSlot::Weapon && CurrentWeapon)
	{
		CurrentWeapon->StopFire();
	}
}

void AWeaponTestCharacter::Reload()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StartReload();
	}
}

void AWeaponTestCharacter::Turn(const FInputActionValue& Value)
{
	float TurnValue = Value.Get<float>();
	if (Controller != nullptr)
	{
		AddControllerYawInput(TurnValue);
	}
}

void AWeaponTestCharacter::LookUp(const FInputActionValue& Value)
{
	float LookValue = Value.Get<float>();
	if (Controller != nullptr)
	{
		AddControllerPitchInput(LookValue);
	}
}

void AWeaponTestCharacter::SwitchToAR()
{
	SwitchWeapon(ARClass);
}

void AWeaponTestCharacter::SwitchToPistol()
{
	SwitchWeapon(PistolClass);
}

void AWeaponTestCharacter::SwitchToShotgun()
{
	SwitchWeapon(ShotgunClass);
}

void AWeaponTestCharacter::SwitchToGrenade()
{
	if (GrenadeCount <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Slot] No grenades left!"));
		return;
	}

	// 현재 무기 숨기기 (Destroy 하지 말고 Hidden 처리)
	if (CurrentWeapon)
	{
		CurrentWeapon->StopFire();
		CurrentWeapon->SetActorHiddenInGame(true);
	}

	CurrentSlot = EEquippedSlot::Grenade;
	UE_LOG(LogTemp, Warning, TEXT("[Slot] Switched to Grenade (Count: %d)"), GrenadeCount);
}

void AWeaponTestCharacter::SwitchWeapon(TSubclassOf<AWeaponBase> NewWeaponClass)
{
	if (!NewWeaponClass) return;

	// 수류탄 슬롯에서 무기 슬롯으로 복귀: 숨긴 무기 다시 보이게
	if (CurrentSlot == EEquippedSlot::Grenade && CurrentWeapon)
	{
		CurrentWeapon->SetActorHiddenInGame(false);
	}
	CurrentSlot = EEquippedSlot::Weapon;

	if (CurrentWeapon && CurrentWeapon->IsA(NewWeaponClass)) return;

	if (CurrentWeapon)
	{
		CurrentWeapon->OnUnequipped();
		CurrentWeapon->Destroy();
		CurrentWeapon = nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	CurrentWeapon = GetWorld()->SpawnActor<AWeaponBase>(NewWeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (CurrentWeapon)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TestChar] Weapon spawned: %s"), *CurrentWeapon->GetName());

		CurrentWeapon->AttachToComponent(
			FirstPersonCamera,
			FAttachmentTransformRules::SnapToTargetIncludingScale,
			NAME_None);

		CurrentWeapon->SetActorRelativeLocation(FVector(30.0f, 15.0f, -10.0f));
		CurrentWeapon->SetActorRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));

		UE_LOG(LogTemp, Warning, TEXT("[TestChar] Weapon attached to camera"));

		CurrentWeapon->OwningCharacter = this;
		CurrentWeapon->OnEquipped();

		if (CurrentWeapon->WeaponMesh1P)
		{
			CurrentWeapon->WeaponMesh1P->SetVisibility(true);
			CurrentWeapon->WeaponMesh1P->SetOnlyOwnerSee(false);
		}
		if (CurrentWeapon->WeaponMesh3P)
		{
			CurrentWeapon->WeaponMesh3P->SetVisibility(false);
		}
	}
}

void AWeaponTestCharacter::ServerApplyDamage(float Damage, ACharacter* DamageInstigator, FHitResult HitResult)
{
	UE_LOG(LogTemp, Warning, TEXT("[TestChar] Damage: %.1f, Bone: %s"), Damage, *HitResult.BoneName.ToString());
}

void AWeaponTestCharacter::ClientShowHitMarker_Implementation(bool bIsHeadshot)
{
	UE_LOG(LogTemp, Warning, TEXT("[TestChar] HitMarker! Headshot: %d"), bIsHeadshot);
}

FVector AWeaponTestCharacter::GetAimDirection() const
{
	return FirstPersonCamera->GetForwardVector();
}

void AWeaponTestCharacter::ThrowGrenade()
{
	if (GrenadeCount <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Grenade] No grenades left!"));
		return;
	}

	if (!GrenadeClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Grenade] GrenadeClass not set!"));
		return;
	}

	FVector CameraLocation;
	FRotator CameraRotation;
	GetController()->GetPlayerViewPoint(CameraLocation, CameraRotation);

	FVector SpawnLocation = CameraLocation + CameraRotation.Vector() * 100.f;
	FRotator SpawnRotation = CameraRotation;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AThrowableBase* Grenade = GetWorld()->SpawnActor<AThrowableBase>(
		GrenadeClass, SpawnLocation, SpawnRotation, SpawnParams);

	if (Grenade)
	{
		FVector ThrowDirection = CameraRotation.Vector();
		ThrowDirection.Z += 0.2f;
		ThrowDirection.Normalize();

		Grenade->ServerThrow(ThrowDirection);
		GrenadeCount--;

		UE_LOG(LogTemp, Warning, TEXT("[Grenade] Thrown! Remaining: %d"), GrenadeCount);
	}
}
