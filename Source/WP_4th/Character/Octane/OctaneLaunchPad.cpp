// Fill out your copyright notice in the Description page of Project Settings.


#include "OctaneLaunchPad.h"

#include "Character/ApexCharacterBase.h"
#include "Components/BoxComponent.h"
#include "GameFramework/CharacterMovementComponent.h"


// Sets default values
AOctaneLaunchPad::AOctaneLaunchPad()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	PadMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PadMesh"));
	RootComponent = PadMesh;
	PadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox ->SetupAttachment(RootComponent);
	TriggerBox ->SetBoxExtent(FVector(80.f,80.f, 50.f));
	TriggerBox ->SetCollisionProfileName(TEXT("Trigger"));
}

// Called when the game starts or when spawned
void AOctaneLaunchPad::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AOctaneLaunchPad::OnTriggerOverlap);

		GetWorldTimerManager().SetTimer(
	  LifeTimeHandle,
	  FTimerDelegate::CreateLambda([this]() { Destroy(); }),
	  LifeTime, false);


	}
}

// Called every frame
void AOctaneLaunchPad::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AOctaneLaunchPad::OnTriggerOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;

	AApexCharacterBase* Char = Cast<AApexCharacterBase>(OtherActor);
	if (!Char) return;

	// 최근 발사된 캐릭터 중복 방지
	if (LaunchCooldowns.Contains(Char)) return;

	FTimerHandle& Handle = LaunchCooldowns.Add(Char);
	GetWorldTimerManager().SetTimer(Handle,
		FTimerDelegate::CreateLambda([this, Char]() { LaunchCooldowns.Remove(Char); }),
		2.f, false);

	// 수평 속도로 진입 상태 판별 (슬라이딩/앉기/공중 진입 모두 커버)
	FVector Velocity = Char->GetVelocity();
	float HorizSpeed = Velocity.Size2D();
	bool bIsLow = HorizSpeed > 200.f  // 수평으로 빠르게 움직이는 중
			   || Char->bIsSliding
			   || Char->GetCharacterMovement()->IsCrouching()
			   || Char->GetCharacterMovement()->IsFalling();

	FVector LaunchVel;

	if (bIsLow)
	{
		// 낮은 각도 — 이동 방향으로 멀리
		FVector HorizDir = Velocity.GetSafeNormal2D();
		if (HorizDir.IsNearlyZero())
			HorizDir = Char->GetActorForwardVector();

		float Rad = FMath::DegreesToRadians(SlidingAngle);
		LaunchVel = (HorizDir * FMath::Cos(Rad) + FVector::UpVector * FMath::Sin(Rad))
					* LaunchStrength;

		UE_LOG(LogTemp, Warning, TEXT("[LaunchPad] Low angle | HorizSpeed: %.1f | Sliding: %s | Crouching: %s"),
			HorizSpeed,
			Char->bIsSliding ? TEXT("true") : TEXT("false"),
			Char->GetCharacterMovement()->IsCrouching() ? TEXT("true") : TEXT("false"));
	}
	else
	{
		// 높은 각도 — 위로
		float Rad = FMath::DegreesToRadians(StandingAngle);
		FVector HorizDir = Char->GetActorForwardVector();
		LaunchVel = (HorizDir * FMath::Cos(Rad) + FVector::UpVector * FMath::Sin(Rad))
					* LaunchStrength;

		UE_LOG(LogTemp, Warning, TEXT("[LaunchPad] High angle | HorizSpeed: %.1f"), HorizSpeed);
	}

	Char->LaunchCharacter(LaunchVel, true, true);
	Char->Multicast_GrantAirJump();

}

