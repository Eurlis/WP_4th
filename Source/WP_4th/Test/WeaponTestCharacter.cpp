// Fill out your copyright notice in the Description page of Project Settings.

#include "WeaponTestCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/DataTable.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "WeaponBase.h"
#include "WeaponData.h"
#include "ThrowableBase.h"
#include "TrajectoryHelper.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractableInterface.h"
#include "Net/UnrealNetwork.h"
#include "Pickup/PickupBase.h"

AWeaponTestCharacter::AWeaponTestCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

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

	// === WP4-37: 수류탄 궤적 프리뷰 ===
	TrajectorySpline = CreateDefaultSubobject<USplineComponent>(TEXT("TrajectorySpline"));
	TrajectorySpline->SetupAttachment(GetCapsuleComponent());
	TrajectorySpline->SetMobility(EComponentMobility::Movable);
	TrajectorySpline->SetUsingAbsoluteLocation(true);
	TrajectorySpline->SetUsingAbsoluteRotation(true);
	TrajectorySpline->SetUsingAbsoluteScale(true);
	TrajectorySpline->ClearSplinePoints(false);

	// === WP4-38: 착탄 마커 ===
	TargetMarkerDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("TargetMarkerDecal"));
	TargetMarkerDecal->SetupAttachment(GetCapsuleComponent());
	TargetMarkerDecal->SetVisibility(false);
	TargetMarkerDecal->DecalSize = FVector(50.f, 50.f, 50.f);

	CurrentWeapon = nullptr;

	InteractionComp = CreateDefaultSubobject<UInteractionComponent>(TEXT("InteractionComp"));

	bReplicates = true;
	SetReplicateMovement(true);
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

	// 슬롯 초기화 (서버 권한)
	if (HasAuthority())
	{
		if (WeaponSlots.Num() == 0)
		{
			WeaponSlots.SetNum(4);
			for (int32 i = 0; i < 4; ++i) { WeaponSlots[i] = NAME_None; }
		}
		if (!ARWeaponID.IsNone())      { WeaponSlots[(int32)EWeaponSlotType::Main1]     = ARWeaponID; }
		if (!PistolWeaponID.IsNone())  { WeaponSlots[(int32)EWeaponSlotType::Pistol]    = PistolWeaponID; }
		if (!GrenadeWeaponID.IsNone()) { WeaponSlots[(int32)EWeaponSlotType::Throwable] = GrenadeWeaponID; }
		ActiveSlotIndex = (int32)EWeaponSlotType::Main1;

		// 종류별 수류탄 인벤토리 시드 (GrenadeWeaponID/GrenadeCount는 시드 전용)
		if (!GrenadeWeaponID.IsNone() && GrenadeCount > 0)
		{
			FGrenadeStockEntry Seed;
			Seed.GrenadeID = GrenadeWeaponID;
			Seed.Count = FMath::Clamp(GrenadeCount, 0, MaxGrenadeCount);
			GrenadeStock.Add(Seed);

			ActiveGrenadeID = GrenadeWeaponID;
		}
	}

	// 기본 무기 AR로 시작
	SwitchWeaponByID(ARWeaponID);

	// 카메라 기본 FOV 적용
	if (FirstPersonCamera)
	{
		FirstPersonCamera->SetFieldOfView(DefaultFOV);
	}

	// 이동 속도 원본 저장
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		SavedDefaultWalkSpeed = MoveComp->MaxWalkSpeed;
	}

	// 마커 머터리얼 적용 (BP에서 TargetMarkerMaterial 세팅 시)
	if (TargetMarkerDecal && TargetMarkerMaterial)
	{
		TargetMarkerDecal->SetDecalMaterial(TargetMarkerMaterial);
	}
}

void AWeaponTestCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!FirstPersonCamera) return;

	const bool bAiming = CurrentWeapon && CurrentWeapon->bIsAiming && CurrentSlot == EEquippedSlot::Weapon;
	const float Multiplier = bAiming ? CurrentWeapon->GetADSFOVMultiplier() : 1.0f;
	const float TargetFOV = DefaultFOV * Multiplier;

	const float CurrentFOV = FirstPersonCamera->FieldOfView;
	const float NewFOV = FMath::FInterpTo(CurrentFOV, TargetFOV, DeltaTime, ADSInterpSpeed);
	FirstPersonCamera->SetFieldOfView(NewFOV);

	if (bIsAimingThrowable && IsCurrentWeaponThrowable())
	{
		UpdateThrowableAimPreview();
	}
}

void AWeaponTestCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UE_LOG(LogTemp, Warning, TEXT("[Init] SetupPlayerInputComponent called"));

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInput)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Init] EnhancedInputComponent CAST FAILED"));
		return;
	}

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
		EnhancedInput->BindAction(ReloadAction, ETriggerEvent::Started, this, &AWeaponTestCharacter::Reload);
	}
	if (SwitchARAction)
	{
		EnhancedInput->BindAction(SwitchARAction, ETriggerEvent::Started, this, &AWeaponTestCharacter::SwitchToAR);
	}
	if (SwitchPistolAction)
	{
		EnhancedInput->BindAction(SwitchPistolAction, ETriggerEvent::Started, this, &AWeaponTestCharacter::SwitchToPistol);
	}
	if (SwitchShotgunAction)
	{
		EnhancedInput->BindAction(SwitchShotgunAction, ETriggerEvent::Started, this, &AWeaponTestCharacter::SwitchToShotgun);
	}
	if (SwitchGrenadeAction)
	{
		EnhancedInput->BindAction(SwitchGrenadeAction, ETriggerEvent::Started, this, &AWeaponTestCharacter::SwitchToGrenade);
	}
	if (TurnAction)
	{
		EnhancedInput->BindAction(TurnAction, ETriggerEvent::Triggered, this, &AWeaponTestCharacter::Turn);
	}
	if (LookUpAction)
	{
		EnhancedInput->BindAction(LookUpAction, ETriggerEvent::Triggered, this, &AWeaponTestCharacter::LookUp);
	}
	if (AimAction)
	{
		EnhancedInput->BindAction(AimAction, ETriggerEvent::Started, this, &AWeaponTestCharacter::OnAimStarted);
		EnhancedInput->BindAction(AimAction, ETriggerEvent::Completed, this, &AWeaponTestCharacter::OnAimStopped);
		EnhancedInput->BindAction(AimAction, ETriggerEvent::Canceled, this, &AWeaponTestCharacter::OnAimStopped);
	}
	if (InteractAction)
	{
		EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &AWeaponTestCharacter::OnInteractInput);
		UE_LOG(LogTemp, Warning, TEXT("[Init] InteractAction BOUND successfully"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Init] InteractAction is NULL - check BP assignment"));
	}
	if (DropAction)
	{
		EnhancedInput->BindAction(DropAction, ETriggerEvent::Started, this, &AWeaponTestCharacter::OnDropPressed);
	}
}

void AWeaponTestCharacter::OnInteractInput(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Verbose, TEXT("[Interact] OnInteractInput called! HasAuthority: %d"), HasAuthority());

	if (!InteractionComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Interact] InteractionComp NULL"));
		return;
	}

	if (!InteractionComp->CurrentInteractable)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[Interact] CurrentInteractable NULL"));
		return;
	}

	UE_LOG(LogTemp, Verbose, TEXT("[Interact] Target: %s"),
		*InteractionComp->CurrentInteractable->GetName());

	AActor* Target = InteractionComp->CurrentInteractable;
	IInteractableInterface* Iface = Cast<IInteractableInterface>(Target);
	if (!Iface)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Interact] Cast<IInteractableInterface> FAILED on %s"), *Target->GetName());
		return;
	}
	if (!Iface->CanInteract(this))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Interact] CanInteract returned FALSE on %s"), *Target->GetName());
		return;
	}

	if (HasAuthority())
	{
		UE_LOG(LogTemp, Verbose, TEXT("[Interact] Calling OnInteract directly (server)"));
		Iface->OnInteract(this);
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("[Interact] Calling ServerInteract RPC (client)"));
		ServerInteract(Target);
	}
}

void AWeaponTestCharacter::ServerInteract_Implementation(AActor* TargetInteractable)
{
	if (!TargetInteractable)
	{
		return;
	}

	// 보안: 클라이언트가 보낸 타깃 액터의 거리 재검증 (조작 방지)
	const float MaxAllowedDist = 500.f;
	if (GetDistanceTo(TargetInteractable) > MaxAllowedDist)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Interact] Target too far: %.1f"), GetDistanceTo(TargetInteractable));
		return;
	}

	IInteractableInterface* Iface = Cast<IInteractableInterface>(TargetInteractable);
	if (Iface && Iface->CanInteract(this))
	{
		Iface->OnInteract(this);
	}
}

void AWeaponTestCharacter::OnAimStarted()
{
	if (CurrentSlot != EEquippedSlot::Weapon || !CurrentWeapon) return;

	CurrentWeapon->StartAiming();

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		if (SavedDefaultWalkSpeed <= 0.f)
		{
			SavedDefaultWalkSpeed = MoveComp->MaxWalkSpeed;
		}
		MoveComp->MaxWalkSpeed = SavedDefaultWalkSpeed * ADSWalkSpeedMultiplier;
	}
}

void AWeaponTestCharacter::OnAimStopped()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StopAiming();
	}

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		if (SavedDefaultWalkSpeed > 0.f)
		{
			MoveComp->MaxWalkSpeed = SavedDefaultWalkSpeed;
		}
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
		StartThrowableAim();
	}
	else if (CurrentWeapon)
	{
		CurrentWeapon->StartFire();
	}
}

void AWeaponTestCharacter::StopFire()
{
	if (CurrentSlot == EEquippedSlot::Grenade && bIsAimingThrowable)
	{
		StopThrowableAim();
		ThrowGrenade();

		// ThrowGrenade는 서버에서만 실제 차감을 수행. 클라이언트는 SSOT가 OnRep으로 갱신됨.
		// 여기서는 보유 총량 0이면 무기 모드로 복귀(시각만 즉시 반응).
		if (GetTotalGrenadeCount() <= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Slot] No grenades left, switching back to weapon"));
			SwitchWeaponByID(!LastWeaponID.IsNone() ? LastWeaponID : ARWeaponID);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[Slot] Grenade thrown, remaining total: %d"),
				GetTotalGrenadeCount());
		}
		return;
	}

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

// TODO: 함수명-의미 불일치. 향후 IA 신설 + 함수명 정리 (예: SwitchToSlot1~4)
void AWeaponTestCharacter::SwitchToAR()       { ServerSwitchToSlot(0); }

// TODO: 함수명-의미 불일치. 향후 IA 신설 + 함수명 정리 (예: SwitchToSlot1~4)
void AWeaponTestCharacter::SwitchToPistol()   { ServerSwitchToSlot(1); }

// TODO: 함수명-의미 불일치. 향후 IA 신설 + 함수명 정리 (예: SwitchToSlot1~4)
void AWeaponTestCharacter::SwitchToShotgun()  { ServerSwitchToSlot(2); }

// TODO: 함수명-의미 불일치. 향후 IA 신설 + 함수명 정리 (예: SwitchToSlot1~4)
void AWeaponTestCharacter::SwitchToGrenade()  { ServerSwitchToSlot(3); }

void AWeaponTestCharacter::SwitchWeaponByID(FName WeaponID)
{
	if (WeaponID.IsNone() || !GenericWeaponClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TestChar] SwitchWeaponByID: invalid ID or GenericWeaponClass not set"));
		return;
	}

	// 중복 호출 방어: 이미 해당 무기를 장착 중이면 무시
	// (수류탄 슬롯에서 복귀하는 경우는 숨김 해제 위해 재스폰 필요하므로 예외)
	if (CurrentWeapon && CurrentSlot != EEquippedSlot::Grenade && CurrentWeapon->WeaponID == WeaponID)
	{
		return;
	}

	// 수류탄 모드였으면 무기 모드로 복귀
	if (CurrentSlot == EEquippedSlot::Grenade)
	{
		CurrentSlot = EEquippedSlot::Weapon;
		StopThrowableAim();
	}

	// 기존 무기 제거 (숨김 무기 포함)
	if (CurrentWeapon)
	{
		CurrentWeapon->OnUnequipped();
		CurrentWeapon->Destroy();
		CurrentWeapon = nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	CurrentWeapon = GetWorld()->SpawnActor<AWeaponBase>(GenericWeaponClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (!CurrentWeapon) return;

	// DataTable에서 데이터 로드
	CurrentWeapon->InitFromDataTable(WeaponID);

	CurrentWeapon->AttachToComponent(
		FirstPersonCamera,
		FAttachmentTransformRules::SnapToTargetIncludingScale,
		NAME_None);

	CurrentWeapon->SetActorRelativeLocation(FVector(30.0f, 15.0f, -10.0f));
	CurrentWeapon->SetActorRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));

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

	// 마지막 무기 기억
	LastWeaponID = WeaponID;

	UE_LOG(LogTemp, Warning, TEXT("[TestChar] Weapon switched to: %s"), *WeaponID.ToString());

	// HUD 위젯 등 BP 측 갱신
	if (CurrentWeapon)
	{
		BP_OnWeaponEquipped(CurrentWeapon);
	}
}

void AWeaponTestCharacter::AddGrenade(FName GrenadeID)
{
	// Backwards-compat 진입점: 활성 종류 변신 버그 제거를 위해 helper로 위임
	if (HasAuthority())
	{
		TryAddGrenadeAuth(GrenadeID);
	}
}

// ==================== Grenade Stock Helpers ====================
int32 AWeaponTestCharacter::GetGrenadeCountByID(FName GrenadeID) const
{
	for (const FGrenadeStockEntry& E : GrenadeStock)
	{
		if (E.GrenadeID == GrenadeID) return E.Count;
	}
	return 0;
}

int32 AWeaponTestCharacter::GetTotalGrenadeCount() const
{
	int32 Total = 0;
	for (const FGrenadeStockEntry& E : GrenadeStock) Total += E.Count;
	return Total;
}

bool AWeaponTestCharacter::IsGrenadeStockFull(FName GrenadeID) const
{
	return GetGrenadeCountByID(GrenadeID) >= MaxGrenadeCount;
}

TArray<FName> AWeaponTestCharacter::GetAvailableGrenadeIDs() const
{
	TArray<FName> Out;
	for (const FGrenadeStockEntry& E : GrenadeStock)
	{
		if (E.Count > 0) Out.Add(E.GrenadeID);
	}
	return Out;
}

FName AWeaponTestCharacter::GetNextGrenadeIDInCycle() const
{
	const TArray<FName> Avail = GetAvailableGrenadeIDs();
	if (Avail.Num() == 0) return NAME_None;
	const int32 Idx = Avail.IndexOfByKey(ActiveGrenadeID);
	return Avail[(Idx == INDEX_NONE ? 0 : (Idx + 1) % Avail.Num())];
}

bool AWeaponTestCharacter::AddGrenadeStock(FName GrenadeID, int32 Amount)
{
	if (GrenadeID.IsNone() || Amount <= 0) return false;
	if (IsGrenadeStockFull(GrenadeID))      return false;

	for (FGrenadeStockEntry& E : GrenadeStock)
	{
		if (E.GrenadeID == GrenadeID)
		{
			E.Count = FMath::Min(E.Count + Amount, MaxGrenadeCount);
			return true;
		}
	}
	FGrenadeStockEntry NewEntry;
	NewEntry.GrenadeID = GrenadeID;
	NewEntry.Count = FMath::Min(Amount, MaxGrenadeCount);
	GrenadeStock.Add(NewEntry);
	return true;
}

bool AWeaponTestCharacter::RemoveGrenadeStock(FName GrenadeID, int32 Amount)
{
	if (GrenadeID.IsNone() || Amount <= 0) return false;
	for (int32 i = 0; i < GrenadeStock.Num(); ++i)
	{
		if (GrenadeStock[i].GrenadeID == GrenadeID)
		{
			GrenadeStock[i].Count = FMath::Max(0, GrenadeStock[i].Count - Amount);
			// 0이어도 항목은 보존(다른 종류 cycle 안정성). 빈 항목 정리는 여기 안 함.
			return true;
		}
	}
	return false;
}

bool AWeaponTestCharacter::TryAddGrenadeAuth(FName GrenadeID)
{
	if (!HasAuthority())
	{
		return false;
	}
	if (GrenadeID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Grenade] TryAddGrenadeAuth: invalid ID"));
		return false;
	}
	if (IsGrenadeStockFull(GrenadeID))
	{
		UE_LOG(LogTemp, Log, TEXT("[Grenade] TryAddGrenadeAuth: %s full (%d/%d), refused"),
			*GrenadeID.ToString(), GetGrenadeCountByID(GrenadeID), MaxGrenadeCount);
		return false;
	}

	const bool bAdded = AddGrenadeStock(GrenadeID, 1);
	if (!bAdded) return false;

	// 활성 종류가 비어있을 때만 갱신. 보유 중이면 ActiveGrenadeID 유지.
	if (ActiveGrenadeID.IsNone() || GetGrenadeCountByID(ActiveGrenadeID) == 0)
	{
		ActiveGrenadeID = GrenadeID;
		if (WeaponSlots.IsValidIndex((int32)EWeaponSlotType::Throwable))
		{
			WeaponSlots[(int32)EWeaponSlotType::Throwable] = GrenadeID;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[Grenade] TryAddGrenadeAuth: %s, count = %d/%d (Active: %s)"),
		*GrenadeID.ToString(), GetGrenadeCountByID(GrenadeID), MaxGrenadeCount,
		*ActiveGrenadeID.ToString());
	return true;
}

void AWeaponTestCharacter::SwitchToFirstAvailableSlot()
{
	if (!HasAuthority()) return;
	for (int32 i = 0; i < WeaponSlots.Num(); ++i)
	{
		if (i == (int32)EWeaponSlotType::Throwable) continue;
		if (!WeaponSlots[i].IsNone())
		{
			SwitchToSlot_Internal(i);
			return;
		}
	}
	// 보유 무기 전무: ActiveSlotIndex만 -1로 정리
	ActiveSlotIndex = -1;
}

// ==================== 4슬롯 무기 시스템 ====================

bool AWeaponTestCharacter::IsSlotEmpty(int32 SlotIndex) const
{
	return SlotIndex < 0 || SlotIndex >= WeaponSlots.Num() || WeaponSlots[SlotIndex].IsNone();
}

int32 AWeaponTestCharacter::FindNextAvailableSlot(int32 SkipIndex) const
{
	for (int32 i = 0; i < WeaponSlots.Num(); ++i)
	{
		if (i == SkipIndex) continue;
		if (!WeaponSlots[i].IsNone()) return i;
	}
	return -1;
}

EWeaponSlotType AWeaponTestCharacter::GetSlotForCategory(EWeaponType WeaponCategory) const
{
	switch (WeaponCategory)
	{
		case EWeaponType::Pistol:    return EWeaponSlotType::Pistol;
		case EWeaponType::Throwable: return EWeaponSlotType::Throwable;
		case EWeaponType::Rifle:
		case EWeaponType::Shotgun:
		case EWeaponType::Sniper:
		default:                     return EWeaponSlotType::Main1;
	}
}

void AWeaponTestCharacter::SwitchToSlot_Internal(int32 SlotIndex)
{
	if (SlotIndex == (int32)EWeaponSlotType::Throwable)
	{
		// 보유 0이면 진입 거부
		if (GetTotalGrenadeCount() <= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[Slot] No grenades left!"));
			return;
		}

		// 4번 슬롯 재진입 = 다음 종류로 사이클
		if (CurrentSlot == EEquippedSlot::Grenade && ActiveSlotIndex == SlotIndex)
		{
			const FName Next = GetNextGrenadeIDInCycle();
			if (!Next.IsNone() && Next != ActiveGrenadeID)
			{
				ActiveGrenadeID = Next;
				if (WeaponSlots.IsValidIndex(SlotIndex))
				{
					WeaponSlots[SlotIndex] = Next;
				}
				UE_LOG(LogTemp, Warning, TEXT("[Slot] Cycled active grenade -> %s"),
					*Next.ToString());
			}
			return;
		}

		// ActiveGrenadeID 보정: 비었거나 카운트 0인 종류면 첫 보유 종류로 고정
		if (ActiveGrenadeID.IsNone() || GetGrenadeCountByID(ActiveGrenadeID) == 0)
		{
			const TArray<FName> Avail = GetAvailableGrenadeIDs();
			if (Avail.Num() > 0)
			{
				ActiveGrenadeID = Avail[0];
				if (WeaponSlots.IsValidIndex(SlotIndex))
				{
					WeaponSlots[SlotIndex] = Avail[0];
				}
			}
			else
			{
				return;
			}
		}

		// 현재 무기 숨기기 (Destroy 하지 말고 Hidden 처리)
		if (CurrentWeapon)
		{
			CurrentWeapon->StopFire();
			CurrentWeapon->SetActorHiddenInGame(true);
		}

		ActiveSlotIndex = SlotIndex;
		CurrentSlot = EEquippedSlot::Grenade;
		UE_LOG(LogTemp, Warning, TEXT("[Slot] Switched to Throwable slot (Active: %s, %d)"),
			*ActiveGrenadeID.ToString(), GetGrenadeCountByID(ActiveGrenadeID));
	}
	else
	{
		if (IsSlotEmpty(SlotIndex)) return;
		ActiveSlotIndex = SlotIndex;
		SwitchWeaponByID(WeaponSlots[SlotIndex]);
	}
}

void AWeaponTestCharacter::SpawnPickupFromSlot(int32 SlotIndex)
{
	if (!HasAuthority()) return;
	if (IsSlotEmpty(SlotIndex)) return;
	if (!PickupClass) return;

	const FName WeaponID = WeaponSlots[SlotIndex];
	const FVector Forward = GetActorForwardVector();
	const FVector Location = GetActorLocation() + Forward * 80.0f;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	Params.Owner = this;

	APickupBase* Pickup = GetWorld()->SpawnActor<APickupBase>(
		PickupClass, Location, FRotator::ZeroRotator, Params);

	if (Pickup)
	{
		Pickup->PickupWeaponID = WeaponID;
		Pickup->ForceNetUpdate();
		Pickup->RefreshFromDataTable();
	}
}

void AWeaponTestCharacter::ServerSwitchToSlot_Implementation(int32 SlotIndex)
{
	if (!HasAuthority()) return;
	if (SlotIndex < 0 || SlotIndex >= WeaponSlots.Num()) return;

	// Throwable 슬롯은 WeaponSlots[3] 비어 있어도 보유 카운트로 진입 판단
	if (SlotIndex == (int32)EWeaponSlotType::Throwable)
	{
		if (GetTotalGrenadeCount() <= 0) return;
	}
	else
	{
		if (IsSlotEmpty(SlotIndex)) return;
	}
	SwitchToSlot_Internal(SlotIndex);
}

void AWeaponTestCharacter::ServerAddWeaponToSlot_Implementation(FName WeaponID)
{
	if (!HasAuthority()) return;
	if (WeaponID.IsNone()) return;

	// DataTable에서 카테고리 조회 — GenericWeaponClass CDO의 WeaponDataTable 사용
	// (UpdateThrowableAimPreview의 ThrowableCDO->WeaponDataTable 패턴 답습)
	EWeaponType Category = EWeaponType::Rifle;  // 기본값

	// GenericWeaponClass CDO로 DataTable 접근 시도
	if (GenericWeaponClass)
	{
		AWeaponBase* WeaponCDO = GenericWeaponClass->GetDefaultObject<AWeaponBase>();
		if (WeaponCDO && WeaponCDO->WeaponDataTable)
		{
			const FWeaponData* Data = WeaponCDO->WeaponDataTable->FindRow<FWeaponData>(
				WeaponID, TEXT("ServerAddWeaponToSlot"));
			if (Data)
			{
				Category = Data->Category;
			}
		}
	}
	// GenericThrowableClass CDO로 시도 (Throwable 카테고리인 경우)
	if (Category == EWeaponType::Rifle && GenericThrowableClass)
	{
		AThrowableBase* ThrowableCDO = GenericThrowableClass->GetDefaultObject<AThrowableBase>();
		if (ThrowableCDO && ThrowableCDO->WeaponDataTable)
		{
			const FWeaponData* Data = ThrowableCDO->WeaponDataTable->FindRow<FWeaponData>(
				WeaponID, TEXT("ServerAddWeaponToSlot_Throwable"));
			if (Data)
			{
				Category = Data->Category;
			}
		}
	}

	int32 TargetSlot = -1;

	if (Category == EWeaponType::Pistol)
	{
		TargetSlot = (int32)EWeaponSlotType::Pistol;
	}
	else if (Category == EWeaponType::Throwable)
	{
		// helper에 모든 결정권 위임 (풀 거부, 활성 종류 보존, 슬롯 갱신)
		TryAddGrenadeAuth(WeaponID);
		return;
	}
	else  // Main 카테고리 (Rifle / Shotgun / Sniper)
	{
		if (IsSlotEmpty((int32)EWeaponSlotType::Main1))
			TargetSlot = (int32)EWeaponSlotType::Main1;
		else if (IsSlotEmpty((int32)EWeaponSlotType::Main2))
			TargetSlot = (int32)EWeaponSlotType::Main2;
		else
			TargetSlot = (ActiveSlotIndex >= 0 && ActiveSlotIndex <= 1)
					   ? ActiveSlotIndex
					   : (int32)EWeaponSlotType::Main1;
	}

	if (TargetSlot < 0) return;

	const bool bSlotWasOccupied = !IsSlotEmpty(TargetSlot);
	const bool bIsActiveSlot = (TargetSlot == ActiveSlotIndex);

	// 옵션 b: 활성 슬롯 교체 시 자동 드롭
	if (bSlotWasOccupied)
	{
		SpawnPickupFromSlot(TargetSlot);
	}

	WeaponSlots[TargetSlot] = WeaponID;

	// 자동 전환 조건: 처음 픽업(ActiveSlotIndex == -1) 또는 활성 슬롯 자체를 교체
	if (ActiveSlotIndex < 0 || (bSlotWasOccupied && bIsActiveSlot))
	{
		SwitchToSlot_Internal(TargetSlot);
	}
}

void AWeaponTestCharacter::ServerDropCurrentWeapon_Implementation()
{
	if (!HasAuthority()) return;
	if (ActiveSlotIndex < 0) return;
	if (IsSlotEmpty(ActiveSlotIndex)) return;

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastDropTime < DropCooldown) return;
	LastDropTime = Now;

	SpawnPickupFromSlot(ActiveSlotIndex);

	const int32 DroppedSlot = ActiveSlotIndex;
	WeaponSlots[DroppedSlot] = NAME_None;

	// Throwable 슬롯이었으면 SSOT(GrenadeStock/ActiveGrenadeID)도 함께 비움
	// (드롭 동작 자체는 기존 그대로. Step 12 신규 드롭 로직은 후속 plan에서 처리)
	if (DroppedSlot == (int32)EWeaponSlotType::Throwable)
	{
		GrenadeStock.Empty();
		ActiveGrenadeID = NAME_None;
	}

	const int32 NextSlot = FindNextAvailableSlot(DroppedSlot);
	if (NextSlot >= 0)
	{
		SwitchToSlot_Internal(NextSlot);
	}
	else
	{
		// 빈손
		if (CurrentWeapon)
		{
			CurrentWeapon->Destroy();
			CurrentWeapon = nullptr;
		}
		// Grenade 모드였으면 슬롯 복귀
		if (CurrentSlot == EEquippedSlot::Grenade)
		{
			CurrentSlot = EEquippedSlot::Weapon;
			StopThrowableAim();
		}
		ActiveSlotIndex = -1;
	}
}

void AWeaponTestCharacter::OnDropPressed()
{
	ServerDropCurrentWeapon();
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
	if (ActiveGrenadeID.IsNone() || GetGrenadeCountByID(ActiveGrenadeID) <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Grenade] No grenades left!"));
		return;
	}

	if (!GenericThrowableClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Grenade] GenericThrowableClass not set!"));
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
		GenericThrowableClass, SpawnLocation, SpawnRotation, SpawnParams);

	if (!Grenade)
	{
		return;
	}

	// DataTable에서 수류탄 데이터 로드 — 활성 종류 기준
	Grenade->InitFromDataTable(ActiveGrenadeID);

	FVector ThrowDirection = CameraRotation.Vector();
	ThrowDirection.Z += 0.2f;
	ThrowDirection.Normalize();

	Grenade->ServerThrow(ThrowDirection);

	// 서버 권한에서만 카운트 차감 (서버에서 호출되는 경로 + 클라이언트 임시 차감 회피)
	if (HasAuthority())
	{
		const FName ThrownID = ActiveGrenadeID;
		RemoveGrenadeStock(ThrownID, 1);

		const int32 Remaining = GetGrenadeCountByID(ThrownID);
		UE_LOG(LogTemp, Warning, TEXT("[Grenade] Thrown! %s remaining: %d"),
			*ThrownID.ToString(), Remaining);

		if (Remaining == 0)
		{
			const FName Next = GetNextGrenadeIDInCycle();
			if (!Next.IsNone() && Next != ThrownID)
			{
				ActiveGrenadeID = Next;
				if (WeaponSlots.IsValidIndex((int32)EWeaponSlotType::Throwable))
				{
					WeaponSlots[(int32)EWeaponSlotType::Throwable] = Next;
				}
			}
			else
			{
				// 모두 0
				ActiveGrenadeID = NAME_None;
				if (WeaponSlots.IsValidIndex((int32)EWeaponSlotType::Throwable))
				{
					WeaponSlots[(int32)EWeaponSlotType::Throwable] = NAME_None;
				}
				SwitchToFirstAvailableSlot();
			}
		}
	}
}

// ==================== WP4-37/38: 수류탄 조준 시스템 ====================

bool AWeaponTestCharacter::IsCurrentWeaponThrowable() const
{
	return CurrentSlot == EEquippedSlot::Grenade && GetTotalGrenadeCount() > 0;
}

void AWeaponTestCharacter::StartThrowableAim()
{
	bIsAimingThrowable = true;
}

void AWeaponTestCharacter::StopThrowableAim()
{
	bIsAimingThrowable = false;
	TrajectoryHelper::ClearTrajectory(TrajectorySpline, TrajectoryMeshes);
	if (TargetMarkerDecal)
	{
		TargetMarkerDecal->SetVisibility(false);
	}
}

void AWeaponTestCharacter::UpdateThrowableAimPreview()
{
	if (!GenericThrowableClass || !GetController() || !GetWorld())
	{
		return;
	}

	// GenericThrowableClass CDO 의 DataTable 에서 투척 물리 파라미터 조회
	// (실제 ServerThrow 와 동일한 ThrowForce / ThrowableGravityScale 사용)
	AThrowableBase* ThrowableCDO = GenericThrowableClass->GetDefaultObject<AThrowableBase>();
	if (!ThrowableCDO || !ThrowableCDO->WeaponDataTable)
	{
		return;
	}

	const FName PreviewID = !ActiveGrenadeID.IsNone() ? ActiveGrenadeID : GrenadeWeaponID;
	if (PreviewID.IsNone())
	{
		return;
	}

	FWeaponData* Data = ThrowableCDO->WeaponDataTable->FindRow<FWeaponData>(
		PreviewID, TEXT("ThrowableAimPreview"));
	if (!Data)
	{
		return;
	}

	// ThrowGrenade 와 동일한 SpawnLocation / Direction 계산
	FVector CameraLocation;
	FRotator CameraRotation;
	GetController()->GetPlayerViewPoint(CameraLocation, CameraRotation);

	const FVector StartLocation = CameraLocation + CameraRotation.Vector() * 100.f;

	FVector ThrowDirection = CameraRotation.Vector();
	ThrowDirection.Z += 0.2f;
	ThrowDirection.Normalize();

	const FVector LaunchVelocity = ThrowDirection * Data->ThrowForce;

	FPredictProjectilePathParams PredictParams(
		TrajectoryProjectileRadius,
		StartLocation,
		LaunchVelocity,
		MaxTrajectorySimTime);

	PredictParams.bTraceWithCollision = true;
	PredictParams.bTraceComplex = false;
	PredictParams.ActorsToIgnore.Add(this);
	PredictParams.SimFrequency = 15.f;
	PredictParams.OverrideGravityZ = -980.f * Data->ThrowableGravityScale;
	PredictParams.TraceChannel = ECC_Visibility;

	FPredictProjectilePathResult Result;
	UGameplayStatics::PredictProjectilePath(this, PredictParams, Result);

	CachedTrajectoryResult = Result;

	TrajectoryHelper::UpdateSplineFromPath(
		TrajectorySpline,
		TrajectoryMeshes,
		Result.PathData,
		TrajectorySplineMesh,
		TrajectoryMeshMaterial,
		this);

	if (TargetMarkerDecal)
	{
		const bool bBlockingHit = Result.HitResult.bBlockingHit;
		const FVector LandingPoint = bBlockingHit
			? Result.HitResult.ImpactPoint
			: Result.LastTraceDestination.Location;
		const FVector LandingNormal = bBlockingHit
			? Result.HitResult.ImpactNormal
			: FVector::UpVector;

		TargetMarkerDecal->SetWorldLocationAndRotation(LandingPoint, LandingNormal.Rotation());
		TargetMarkerDecal->SetVisibility(bBlockingHit);
	}
}

// ==================== Replication ====================
void AWeaponTestCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AWeaponTestCharacter, LightAmmo,       COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AWeaponTestCharacter, HeavyAmmo,       COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AWeaponTestCharacter, EnergyAmmo,      COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AWeaponTestCharacter, ShotgunAmmo,     COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AWeaponTestCharacter, WeaponSlots,     COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AWeaponTestCharacter, ActiveSlotIndex, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AWeaponTestCharacter, GrenadeStock,    COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AWeaponTestCharacter, ActiveGrenadeID, COND_OwnerOnly);
}

// ==================== Ammo Pool ====================
int32 AWeaponTestCharacter::GetMaxAmmoForType(EAmmoType Type) const
{
	switch (Type)
	{
	case EAmmoType::Light:   return MaxLightAmmo;
	case EAmmoType::Heavy:   return MaxHeavyAmmo;
	case EAmmoType::Energy:  return MaxEnergyAmmo;
	case EAmmoType::Shotgun: return MaxShotgunAmmo;
	case EAmmoType::Sniper:  return MaxHeavyAmmo;  // Sniper은 Heavy 풀로 통합
	default:                 return 0;
	}
}

int32 AWeaponTestCharacter::GetAmmoForType(EAmmoType Type) const
{
	switch (Type)
	{
	case EAmmoType::Light:   return LightAmmo;
	case EAmmoType::Heavy:   return HeavyAmmo;
	case EAmmoType::Energy:  return EnergyAmmo;
	case EAmmoType::Shotgun: return ShotgunAmmo;
	case EAmmoType::Sniper:  return HeavyAmmo;  // Sniper → Heavy 통합
	default:                 return 0;
	}
}

void AWeaponTestCharacter::SetAmmoForType(EAmmoType Type, int32 NewAmount)
{
	switch (Type)
	{
	case EAmmoType::Light:   LightAmmo   = NewAmount; break;
	case EAmmoType::Heavy:   HeavyAmmo   = NewAmount; break;
	case EAmmoType::Energy:  EnergyAmmo  = NewAmount; break;
	case EAmmoType::Shotgun: ShotgunAmmo = NewAmount; break;
	case EAmmoType::Sniper:  HeavyAmmo   = NewAmount; break;  // Sniper → Heavy 통합
	default: break;
	}
}

int32 AWeaponTestCharacter::GetReserveAmmo(EAmmoType Type) const
{
	return GetAmmoForType(Type);
}

int32 AWeaponTestCharacter::AddAmmo(EAmmoType Type, int32 Count)
{
	if (Count <= 0) return 0;

	if (!HasAuthority())
	{
		// 클라에서 호출 시 RPC만 보내고 0 반환 (실제 추가량은 서버 결과)
		ServerAddAmmo(Type, Count);
		return 0;
	}

	const int32 Current = GetAmmoForType(Type);
	const int32 Max = GetMaxAmmoForType(Type);
	const int32 Added = FMath::Clamp(Max - Current, 0, Count);
	if (Added <= 0) return 0;

	const int32 NewAmount = Current + Added;
	SetAmmoForType(Type, NewAmount);

	UE_LOG(LogTemp, Log, TEXT("[Ammo] AddAmmo %s: +%d (Total: %d / %d)"),
		*UEnum::GetValueAsString(Type), Added, NewAmount, Max);

	OnReserveAmmoChanged.Broadcast(Type, NewAmount);
	return Added;
}

int32 AWeaponTestCharacter::ConsumeReserve(EAmmoType Type, int32 Needed)
{
	if (Needed <= 0) return 0;

	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Ammo] ConsumeReserve called on client - ignored"));
		return 0;
	}

	const int32 Current = GetAmmoForType(Type);
	if (Current <= 0) return 0;

	const int32 Consumed = FMath::Min(Needed, Current);
	const int32 NewAmount = Current - Consumed;
	SetAmmoForType(Type, NewAmount);

	UE_LOG(LogTemp, Log, TEXT("[Ammo] ConsumeReserve %s: -%d (Remaining: %d)"),
		*UEnum::GetValueAsString(Type), Consumed, NewAmount);

	OnReserveAmmoChanged.Broadcast(Type, NewAmount);
	return Consumed;
}

void AWeaponTestCharacter::ServerAddAmmo_Implementation(EAmmoType Type, int32 Count)
{
	AddAmmo(Type, Count);
}
