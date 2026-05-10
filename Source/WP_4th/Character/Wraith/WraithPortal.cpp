// Fill out your copyright notice in the Description page of Project Settings.


#include "WraithPortal.h"
#include "NiagaraComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SplineComponent.h"
#include "GameFramework/Character.h"


// Sets default values
AWraithPortal::AWraithPortal()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	TriggerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerSphere"));
	TriggerSphere->SetSphereRadius(100.f);
	TriggerSphere->SetCollisionProfileName(TEXT("Trigger"));
	RootComponent = TriggerSphere;

	PortalVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PortalVFX"));
	PortalVFX->SetupAttachment(RootComponent);
	PortalVFX->SetIsReplicated(true);

	PortalSpline = CreateDefaultSubobject<USplineComponent>(TEXT("PortalSpline"));
	PortalSpline->SetupAttachment(RootComponent);
}

void AWraithPortal::SetPathAndLink(const TArray<FVector>& Path, AWraithPortal* Other)
{
	LinkedPortal = Other;

	PortalSpline->ClearSplinePoints();
	for (int32 i =0; i< Path.Num(); i++)
	{
		PortalSpline->AddSplinePoint(Path[i], ESplineCoordinateSpace::World);
	}
	PortalSpline->UpdateSpline();

	TriggerSphere->OnComponentBeginOverlap.AddDynamic(this, &AWraithPortal::OnOverlapBegin);

	PortalVFX->Activate(true);
}

void AWraithPortal::AddCooldown(AActor* Actor, float Duration)
{
	RecentlyTeleported.Add(Actor);
	FTimerHandle TH;
	FTimerDelegate Delegate = FTimerDelegate::CreateLambda([this, Actor]()
	{
		ClearCooldown(Actor);
	});
	GetWorldTimerManager().SetTimer(TH,Delegate,Duration, false);
}


// Called when the game starts or when spawned
void AWraithPortal::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void AWraithPortal::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority() || !bIsTraveling || !TravelingChar) return;

	float SplineLength = PortalSpline->GetSplineLength();
	TravelDistance += TravelSpeed * DeltaTime;

	if (TravelDistance >= SplineLength)
	{
		FVector EndLocation = PortalSpline->GetLocationAtDistanceAlongSpline(SplineLength, ESplineCoordinateSpace::World);

		TravelingChar->SetActorLocation(EndLocation, false, nullptr, ETeleportType::TeleportPhysics);
		FinishTraversal();
		return;
	}

	FVector NewLocation = PortalSpline->GetLocationAtDistanceAlongSpline(TravelDistance, ESplineCoordinateSpace::World);
	TravelingChar->SetActorLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);
}

void AWraithPortal::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;
	if (!LinkedPortal || !OtherActor) return;
	if (RecentlyTeleported.Contains(OtherActor)) return;
	if (bIsTraveling) return;

	ACharacter* Char = Cast<ACharacter>(OtherActor);
	if (!Char) return;
	RecentlyTeleported.Add(OtherActor);
	StartTraversal(Char);


}

void AWraithPortal::ClearCooldown(AActor* Actor)
{
	RecentlyTeleported.Remove(Actor);
}

void AWraithPortal::StartTraversal(ACharacter* Char)
{
	TravelingChar = Char;
	TravelDistance = 0.f;
	bIsTraveling = true;

	// 이동 시작 즉시 목적지 포탈 차단 (이동 중 트리거 진입 방지)
	if (LinkedPortal)
	{
		LinkedPortal->RecentlyTeleported.Add(Char);
	}

	if (APlayerController* PC = Cast<APlayerController>(Char->GetController()))
	{
		Char->DisableInput(PC);
	}
}

void AWraithPortal::FinishTraversal()
{
	if (TravelingChar)
	{
		if (APlayerController* PC = Cast<APlayerController>(TravelingChar->GetController()))
		{
			TravelingChar->EnableInput(PC);
		}
		AActor* CharActor = TravelingChar;

		// 현재 포탈 쿨다운 2초 후 해제
		FTimerHandle TH1;
		FTimerDelegate D1 = FTimerDelegate::CreateLambda([this, CharActor]()
		{
			ClearCooldown(CharActor);
		});
		GetWorldTimerManager().SetTimer(TH1, D1, 2.f, false);

		// 목적지 포탈 쿨다운 2초 후 해제
		if (LinkedPortal)
		{
			FTimerHandle TH2;
			FTimerDelegate D2 = FTimerDelegate::CreateLambda([this, CharActor]()
			{
				if (LinkedPortal) LinkedPortal->ClearCooldown(CharActor);
			});
			GetWorldTimerManager().SetTimer(TH2, D2, 2.f, false);
		}
	}
	TravelingChar = nullptr;
	bIsTraveling = false;
}

