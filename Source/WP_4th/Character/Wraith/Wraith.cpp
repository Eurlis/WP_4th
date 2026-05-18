// Fill out your copyright notice in the Description page of Project Settings.


#include "Wraith.h"

#include "Camera/CameraComponent.h"
#include "Algo/Reverse.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"


// Sets default values
AWraith::AWraith()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	// Asset 세팅
	ConstructorHelpers::FObjectFinder<USkeletalMesh> WraithMesh(TEXT("/Game/Models/Wraith/Wraith.Wraith"));
	if (WraithMesh.Succeeded())
	{
		FirstPersonMesh->SetSkeletalMesh(WraithMesh.Object);
		GetMesh()->SetSkeletalMesh(WraithMesh.Object);
	}

	// Transfrom 설정.. 터지지마세요.. FirstPersonMesh
	FirstPersonMesh->SetWorldLocationAndRotation(FVector(0.000000,0.000000,0.000000),FRotator(0.000000,0.000000,0.000000));
	FirstPersonMesh->SetRelativeScale3D(FVector(1.000000,1.000000,1.000000));

	//FirstPersonCameraComponent TransForm
	/*FirstPersonCameraComponent->SetRelativeLocationAndRotation(FVector(12.520892,3.034245,-0.000011), FRotator(-20.000000,50.000000,-110.000000));
	FirstPersonCameraComponent->SetRelativeScale3D(FVector(1.000000,1.000000,1.000000));
	*/

	//Mesh TransForm
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.000000,0.000000,-100.000000), FRotator(0.000000,-90.000000,0.000000));
	GetMesh()->SetRelativeScale3D(FVector(1.000000,1.000000,1.000000));
	GetCapsuleComponent()->SetRelativeScale3D(FVector(1.f));

	ConstructorHelpers::FClassFinder<UAnimInstance> ABP_Wraith(TEXT("/Game/BluePrints/ABP/ABP_Wraith.ABP_Wraith_C"));
	if (ABP_Wraith.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(ABP_Wraith.Class);
	}

	VoidVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("VoidVFX"));
	VoidVFX ->SetupAttachment(RootComponent);
	VoidVFX ->SetAutoActivate(false);
}

// Called when the game starts or when spawned
void AWraith::BeginPlay()
{
	Super::BeginPlay();
}

void AWraith::ActivateUltimate()
{
	if (bUltimateOnCooldown) return;
	if (PortalA && PortalB) return;

	Server_ActivateUltimate();
}

void AWraith::ActivateTactical()
{
	if (bTacticalOnCooldown || IsInVoid) return;
	Server_ActivateTactical();
}

void AWraith::Server_ActivateTactical_Implementation()
{
	if (bTacticalOnCooldown || IsInVoid) return;
	Multcast_SetvoidState(true);
	GetWorldTimerManager().SetTimer(
		 TacticalDurationTimer,
		 FTimerDelegate::CreateLambda([this]()
		 {
			 Multcast_SetvoidState(false);
		 }),
		 TacticalDuration, false);

}

void AWraith::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWraith, IsInVoid);
}

// Called every frame
void AWraith::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (HasAuthority() && bPlacingPortal)
	{
		FVector CurrentLocation = GetActorLocation();
		if (FVector::Dist (CurrentLocation, LastSampledLocation) >= SampleDistance)
		{
			RecordedPath.Add(CurrentLocation);
			LastSampledLocation = CurrentLocation;
		}
	}
}

float AWraith::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator,
	AActor* DamageCauser)
{
	if (HasAuthority() && IsInVoid)
		return 0.f;

	return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

}


// Called to bind functionality to input
void AWraith::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void AWraith::OnRep_IsInVoid()
{
	if (IsInVoid) EnterVoid();
	else ExitVoid();
}

void AWraith::EnterVoid()
{
	UE_LOG(LogTemp, Warning, TEXT("[Wraith] Into the Void 진입!"));
	if (Sound_TacticalActivate)
		UGameplayStatics::SpawnSoundAtLocation(this, Sound_TacticalActivate, GetActorLocation());

	/*VoidMaterials.Empty();
	for (int32 i =0; i< GetMesh()->GetNumMaterials(); i++)
	{
		UMaterialInstanceDynamic* MID = GetMesh()->CreateAndSetMaterialInstanceDynamic(i);
		MID->SetScalarParameterValue(TEXT("Opacity"), 0.3);
		VoidMaterials.Add(MID);
	}*/
	GetMesh()->SetVisibility(false);

	if (VoidVFX) VoidVFX->Activate(true);
	SpeedMultiplier = 1.5f;
	GetCharacterMovement()->MaxWalkSpeed = (bIsSprinting ? SprintSpeed : WalkSpeed) * SpeedMultiplier;

}

void AWraith::ExitVoid()
{
	UE_LOG(LogTemp, Warning, TEXT("[Wraith] Void 종료, 쿨타임 시작"));
	/*for (UMaterialInstanceDynamic* MID : VoidMaterials)
	{
		if (MID) MID->SetScalarParameterValue(TEXT("Opacity"), 1.f);
	}
	VoidMaterials.Empty();*/
	GetMesh()->SetVisibility(true);

	if (VoidVFX) VoidVFX->Deactivate();

	SpeedMultiplier = 1.0f;
	GetCharacterMovement()->MaxWalkSpeed = bIsSprinting ? SprintSpeed : WalkSpeed;


	if (HasAuthority())
	{
		bTacticalOnCooldown = true;
		TacticalCooldownRemaining = TacticalCooldown;
		GetWorldTimerManager().SetTimer(TacticalCooldownTimer,[this]()
		{
			bTacticalOnCooldown = false;
		}, TacticalCooldown, false);
	}

}

void AWraith::DeactivatePortals()
{
	if (PortalA) { PortalA->Destroy(); PortalA = nullptr; }
	if (PortalB) { PortalB->Destroy(); PortalB = nullptr; }

	bUltimateOnCooldown = true;
	UltCooldownRemaining = UltimateCooldown;
	GetWorldTimerManager().SetTimer(UltimateCooldownTimer, [this]()
	{
		bUltimateOnCooldown = false;
	}, UltimateCooldown, false);

	UE_LOG(LogTemp, Log, TEXT("[Wraith Ult] 포탈 Off, 쿨타임.."));
}

void AWraith::Server_ActivateUltimate_Implementation()
{
	if (bUltimateOnCooldown) return;
	if (PortalA && PortalB) return;

	if (!bPlacingPortal)
	{
		PortalALocation = GetActorLocation();
		bPlacingPortal = true;
		Multicast_PlayUltSound();
		UE_LOG(LogTemp, Warning, TEXT("[Wraith Ult] Portal A 저장: %s "), *PortalALocation.ToString());

		RecordedPath.Empty();
		RecordedPath.Add(PortalALocation);
		LastSampledLocation = PortalALocation;

	}
	else
	{
		bPlacingPortal = false;

		RecordedPath.Add(GetActorLocation());

		if (!PortalClass) PortalClass = AWraithPortal::StaticClass();

		FActorSpawnParameters Params;
		Params.Owner = this;


		PortalA = GetWorld()->SpawnActor<AWraithPortal>(PortalClass, PortalALocation, FRotator::ZeroRotator, Params);
		PortalB = GetWorld()->SpawnActor<AWraithPortal>(PortalClass, GetActorLocation(), FRotator::ZeroRotator, Params);


		if (PortalA && PortalB)
		{
			TArray<FVector> ReversedPath = RecordedPath;
			Algo::Reverse(ReversedPath);

			PortalA->SetPathAndLink(RecordedPath, PortalB);
			PortalB->SetPathAndLink(ReversedPath, PortalA);

			// DrawDebugSphere(GetWorld(), PortalALocation, 100.f, 12, FColor::Red, false, 10.f);
			// DrawDebugSphere(GetWorld(), GetActorLocation(), 100.f, 12, FColor::Blue, false, 10.f);
			GetWorldTimerManager().SetTimer(UltimateDurationTimer, this, &AWraith::DeactivatePortals, UltimateDuration, false);

			UE_LOG(LogTemp, Log, TEXT("[Wraith Ult] 포탈 On"));
		}
	}
}

void AWraith::Multcast_SetvoidState_Implementation(bool bInVoid)
{
	IsInVoid = bInVoid;
	if (bInVoid) EnterVoid();
	else ExitVoid();
}

