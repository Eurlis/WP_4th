// Fill out your copyright notice in the Description page of Project Settings.

#include "FireZone.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"
#include "UObject/ConstructorHelpers.h"

AFireZone::AFireZone()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false); // 정적 액터 - 위치 복제 불필요

	DamageSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DamageSphere"));
	RootComponent = DamageSphere;
	DamageSphere->SetSphereRadius(400.0f);
	DamageSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DamageSphere->SetCollisionResponseToAllChannels(ECR_Overlap);

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(DamageSphere);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetRelativeScale3D(FVector(4.0f, 4.0f, 0.5f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(
		TEXT("/Engine/BasicShapes/Cylinder")
	);
	if (CylinderMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(CylinderMesh.Object);
	}
}

void AFireZone::BeginPlay()
{
	Super::BeginPlay();
}

void AFireZone::InitializeFireZone(float InDuration, float InTickInterval,
                                   float InDamagePerTick, float InRadius,
                                   AActor* InInstigator)
{
	// 서버에서만 데이터 세팅 + 타이머 돌림 (클라 오용 방지)
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[FireZone] InitializeFireZone called on client, ignoring"));
		return;
	}

	Duration = InDuration;
	TickInterval = InTickInterval;
	DamagePerTick = InDamagePerTick;
	Radius = InRadius;
	DamageInstigator = InInstigator;

	// 시전자 사망 후에도 킬 크레딧 유지되도록 컨트롤러 캐시
	if (APawn* InstigatorPawn = Cast<APawn>(InInstigator))
	{
		CachedInstigatorController = InstigatorPawn->GetController();
	}

	DamageSphere->SetSphereRadius(Radius);

	UE_LOG(LogTemp, Warning,
	       TEXT("[FireZone] Initialized: Dur=%.1fs, Tick=%.2fs, Dmg=%.1f, Radius=%.1f, Instigator=%s"),
	       Duration, TickInterval, DamagePerTick, Radius,
	       InInstigator ? *InInstigator->GetName() : TEXT("NULL"));

	if (GetWorld())
	{
		DrawDebugCircle(
			GetWorld(), GetActorLocation(), Radius, 32,
			FColor::Red, false, Duration, 0, 5.0f,
			FVector(1, 0, 0), FVector(0, 1, 0), false
		);
	}

	if (HasAuthority())
	{
		GetWorld()->GetTimerManager().SetTimer(
			DamageTimerHandle, this,
			&AFireZone::ApplyTickDamage,
			TickInterval, true
		);

		GetWorld()->GetTimerManager().SetTimer(
			LifetimeTimerHandle, this,
			&AFireZone::DestroyFireZone,
			Duration, false
		);
	}
}

void AFireZone::ApplyTickDamage()
{
	if (!HasAuthority()) return;

	TArray<AActor*> OverlappingActors;
	DamageSphere->GetOverlappingActors(OverlappingActors, ACharacter::StaticClass());

	UE_LOG(LogTemp, Log,
	       TEXT("[FireZone] Tick - Overlapping Characters: %d"),
	       OverlappingActors.Num());

	AController* InstigatorController = CachedInstigatorController.Get();

	for (AActor* Actor : OverlappingActors)
	{
		if (!IsValid(Actor)) continue;

		UGameplayStatics::ApplyDamage(
			Actor, DamagePerTick,
			InstigatorController,		// 킬 크레딧
			DamageInstigator,			// DamageCauser
			nullptr
		);

		const bool bIsSelfDamage = (Actor == DamageInstigator);
		UE_LOG(LogTemp, Log,
		       TEXT("[FireZone] %s %s: %.1f damage"),
		       bIsSelfDamage ? TEXT("Self-damage on") : TEXT("Damaged"),
		       *Actor->GetName(),
		       DamagePerTick);
	}
}

void AFireZone::DestroyFireZone()
{
	UE_LOG(LogTemp, Warning, TEXT("[FireZone] Duration ended, destroying"));

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(DamageTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(LifetimeTimerHandle);
	}

	Destroy();
}
