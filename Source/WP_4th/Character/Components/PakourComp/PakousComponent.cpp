// Fill out your copyright notice in the Description page of Project Settings.


#include "PakousComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"


// Sets default values for this component's properties
UPakousComponent::UPakousComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UPakousComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());	
	WallTraceParams.AddIgnoredActor(OwnerCharacter);
}


// Called every frame
void UPakousComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                     FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	DetectWall();
	if (bwallFoward)
	{
		ScanWallTop();
		if (bFoundTop)ScanWallEdge();
		ScanLanding();
	}
	// ...
}

bool UPakousComponent::CanWallJump() const
{
	return bwallFoward && OwnerCharacter && !OwnerCharacter->GetCharacterMovement()->IsMovingOnGround();
}

void UPakousComponent::DetectWall()
{
	if (!OwnerCharacter) return;
	
	FVector Start = OwnerCharacter->GetActorLocation();
	FVector End = Start + OwnerCharacter->GetActorForwardVector() * 75.f;
	
	bwallFoward = GetWorld()->LineTraceSingleByChannel(wallHitResult, Start, End, ECC_WorldStatic, WallTraceParams);
	
	DrawDebugLine(GetWorld(), Start, End, bwallFoward ? FColor::Green : FColor::Red, false, -1.f, 0, 2.f);
	// Normal 반전
	if (bwallFoward)
	{
		WallNormal = wallHitResult.Normal;
		//벽이 바라보는 방향
		WallNormalReversed = WallNormal.RotateAngleAxis(180.f, FVector::UpVector);
		//캐릭터 -> 벽 방향
	}
	
}

void UPakousComponent::ScanWallTop()
{
	for (int i = 0; i < 10; i++)
	{
		// 플레이어 위치 + 앞방향 75+ 위로 300~0 순서로 내려오면 스캔
		FVector TraceStart = OwnerCharacter->GetActorLocation()+ WallNormalReversed * 75.f + FVector(0, 0, 300.f - (i * 30.f));
		FVector TraceEnd = TraceStart + FVector(0, 0, -30.f);
		if (GetWorld()->LineTraceSingleByChannel(ScanHitResult, TraceStart, TraceEnd, ECC_WorldStatic, WallTraceParams))
		{
			bFoundTop = true;
			DrawDebugSphere(GetWorld(), ScanHitResult.ImpactPoint,
 8.f, 8, FColor::Yellow, false, -1.f);
			break;
		}
	}
	
}

void UPakousComponent::ScanWallEdge()
{
	if (bFoundTop)
	{
		for (int i = 0; i < 10; i++)
		{
			FVector SphereStart = ScanHitResult.ImpactPoint+ WallNormalReversed * (i * 15.f);
			FVector SphereEnd = SphereStart + FVector(0, 0, -50.f);
			FHitResult SphereHit;
			bool bHit = UKismetSystemLibrary::SphereTraceSingle(this ,SphereStart, SphereEnd, 10.f, UEngineTypes::ConvertToTraceType(ECC_WorldStatic),false, {OwnerCharacter}, EDrawDebugTrace::None, SphereHit, true);
			
			if (bHit)
			{
				LastTopHit = SphereHit;
				DrawDebugSphere(GetWorld(), LastTopHit.ImpactPoint, 8.f,
	  8, FColor::Orange, false, -1.f);
			}
			else break;
		}
	}
}

void UPakousComponent::ScanLanding()
{
	FHitResult BoltLandingHit;
	
	FVector LandingStart = LastTopHit.ImpactPoint + WallNormalReversed * 50.f;
	FVector LandingEnd = LandingStart + FVector(0, 0, 200.f);
	
	bCanVault = UKismetSystemLibrary::SphereTraceSingle(this, LandingStart, LandingEnd, 20.f, UEngineTypes::ConvertToTraceType(ECC_WorldStatic), false, {OwnerCharacter}, EDrawDebugTrace::None, BoltLandingHit, true);
	
	if (bCanVault)
	{
		VaultLandingLocation = BoltLandingHit.ImpactPoint;
		DrawDebugSphere(GetWorld(), VaultLandingLocation, 15.f,
	  8, FColor::Green, false, -1.f);
	}
}

