// Fill out your copyright notice in the Description page of Project Settings.

#include "FireZone.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "DrawDebugHelpers.h"

AFireZone::AFireZone()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true); // 회전 복제 필요 (투척 방향 화염)

	// BoxComponent: 수평 확장 (X=길이, Y=너비, Z=높이)
	DamageBox = CreateDefaultSubobject<UBoxComponent>(TEXT("DamageBox"));
	RootComponent = DamageBox;
	DamageBox->SetBoxExtent(FVector(200.0f, 600.0f, 100.0f));
	DamageBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DamageBox->SetCollisionResponseToAllChannels(ECR_Overlap);
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
	DamageInstigator = InInstigator;

	// 시전자 사망 후에도 킬 크레딧 유지되도록 컨트롤러 캐시
	if (APawn* InstigatorPawn = Cast<APawn>(InInstigator))
	{
		CachedInstigatorController = InstigatorPawn->GetController();
	}

	// Radius → Box 치수 변환 (투척 방향 수직 수평 확장)
	//   앞뒤(X) = Radius × 0.5  (짧음)
	//   좌우(Y) = Radius × 1.5  (김 ⭐ Apex 화염 패턴)
	//   위아래(Z) = Radius × 0.25
	if (InRadius > 0.0f && DamageBox)
	{
		BoxExtent = FVector(
			InRadius * 0.5f,
			InRadius * 1.5f,
			InRadius * 0.25f
		);
		DamageBox->SetBoxExtent(BoxExtent);
	}

	UE_LOG(LogTemp, Warning,
	       TEXT("[FireZone] Init: Dur=%.1fs Tick=%.2fs Dmg=%.1f BoxExtent=%s Instigator=%s"),
	       Duration, TickInterval, DamagePerTick, *BoxExtent.ToString(),
	       InInstigator ? *InInstigator->GetName() : TEXT("NULL"));

	if (GetWorld())
	{
		DrawDebugBox(
			GetWorld(),
			GetActorLocation(),
			BoxExtent,
			GetActorQuat(),
			FColor::Red,
			false,
			Duration,
			0,
			5.0f
		);
	}

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

void AFireZone::ApplyTickDamage()
{
	if (!HasAuthority()) return;
	if (!DamageBox) return;

	TArray<AActor*> OverlappingActors;
	DamageBox->GetOverlappingActors(OverlappingActors, ACharacter::StaticClass());

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
