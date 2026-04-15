# Apex Legends 1v1 프로토타입 — 기능 명세서

> **목표**: Listen Server 기반 1v1 멀티플레이어 슈터 프로토타입  
> **엔진**: Unreal Engine 5 (C++)  
> **구조**: 서버 1대 기준, Listen Server 방식

---

## 네트워크 구조 개요

```
[Listen Server (Host)] ←→ [Client A]
                       ←→ [Client B (추후 확장)]
```

| 구분 | 역할 | 키워드 |
|------|------|--------|
| 서버 (Authority) | 데미지·승패·히트판정 등 실제 계산 | `HasAuthority()` |
| 클라이언트 | 입력 전달, 이펙트·HUD 표시 | `IsLocallyControlled()` |
| RPC 통신 | Server RPC / Multicast / RepNotify | `UFUNCTION(Server, Reliable)` |

---

## Phase 1 — 필수 구현 기능 (MVP)

### 캐릭터 이동 시스템

| 기능 | 구현 방식 | 네트워크 처리 |
|------|-----------|--------------|
| 기본 이동 (WASD) | CharacterMovement | 내장 자동 처리 |
| 점프 | CharacterMovement | 내장 자동 처리 |
| 뛰기 (Sprint) | MaxWalkSpeed 변경 | Server RPC |
| 앉기 (Crouch) | Crouch() 함수 | CharacterMovement 내장 |
| 슬라이딩 | 커스텀 Movement Mode | Server RPC + RepNotify |
| 슬라이딩 점프 | 슬라이딩 중 점프 입력 처리 | 위와 동일 |
| 낙하 데미지 | 낙하 속도 → 데미지 계산 | 서버 전용 |
| Mantle (선반 잡기) | LineTrace + 위치 보정 | Server RPC |

```cpp
// 슬라이딩 상태 복제 예시
UPROPERTY(ReplicatedUsing = OnRep_IsSliding)
bool bIsSliding;

UFUNCTION(Server, Reliable)
void Server_StartSlide();

UFUNCTION(NetMulticast, Unreliable)
void Multicast_PlaySlideAnim();
```

---

### 전투 시스템

| 기능 | 처리 위치 | 구현 방식 |
|------|-----------|-----------|
| 체력 (Health) | 서버 계산 | `Replicated` 변수 |
| 실드 (Shield) | 서버 계산 | `Replicated` 변수 |
| 데미지 적용 | **서버 전용** | `TakeDamage()` 오버라이드 |
| 사망 처리 | 서버 판정 | Multicast → 래그돌 |
| 부활 / 다운 | (Phase 2 이후) | - |

```cpp
// HealthComponent.h
UPROPERTY(ReplicatedUsing = OnRep_Health)
float Health;

UPROPERTY(ReplicatedUsing = OnRep_Shield)
float Shield;

UFUNCTION()
void OnRep_Health();  // HUD 업데이트 트리거
```

---

### 무기 시스템

| 기능 | 처리 위치 | 우선순위 |
|------|-----------|---------|
| 발사 (히트스캔) | 서버 LineTrace | ★★★ 필수 |
| 재장전 | 서버 탄약 관리 | ★★★ 필수 |
| 탄약 수량 | 서버 관리 → 클라 표시 | ★★★ 필수 |
| 무기 줍기 | Overlap → Server RPC | ★★★ 필수 |
| 무기 교체 (1·2슬롯) | 인벤토리 컴포넌트 | ★★★ 필수 |
| ADS (줌) | 클라이언트 로컬 | ★★☆ |
| 반동 | 클라이언트 카메라 | ★★☆ |
| 돌격소총 | 자동발사, 히트스캔 | ★★★ 1순위 |
| 권총 | 단발, 높은 정확도 | ★★☆ Phase 2 |
| 산탄총 | 다중 LineTrace | ★★☆ Phase 2 |

```cpp
// 발사 흐름 (네트워크) — 조작감 최적화 버전
//
// [권장 방식] 로컬 FX 즉시 재생 + 서버 데미지 판정 분리
//
// 1. 클라이언트 (로컬): 사운드·이펙트 즉시 재생  ← 딜레이 없이 빠릿한 조작감
// 2. 클라이언트 → 서버: Server_Fire() 호출 (히트 검증 요청)
// 3. 서버: LineTrace → TakeDamage() 실행
// 4. 서버 → 다른 클라이언트들만: Multicast_FireEffect() ← 내 FX는 이미 재생됨
//
// ❌ 잘못된 방식: Server_Fire() → Multicast로 내 FX까지 처리
//    → 클릭 후 FX 재생까지 RTT(왕복 지연) 발생 → 답답한 조작감

UFUNCTION(Server, Reliable)
void Server_Fire(FVector Start, FVector Direction);

// bSkipOwner = true 로 호출하면 자신에게는 중복 재생 방지
UFUNCTION(NetMulticast, Unreliable)
void Multicast_FireEffect(FVector HitLocation);
```

---

### HUD (클라이언트 전용)

| 요소 | 구현 방식 | 비고 |
|------|-----------|------|
| 크로스헤어 | UMG Widget | 항상 표시 |
| HP 바 | UMG + OnRep_Health 바인딩 | |
| 실드 바 | UMG + OnRep_Shield 바인딩 | |
| 현재 탄약 / 최대 탄약 | UMG + RepNotify | |
| 킬 피드 | (Phase 2) | |
| 미니맵 | (Phase 3) | |

---

### 게임 모드 (GameMode)

| 기능 | 처리 위치 | 설명 |
|------|-----------|------|
| 게임 시작 | 서버 | 양쪽 플레이어 스폰 |
| 스폰 위치 결정 | 서버 | PlayerStart 배열 관리 |
| 승패 판정 | **서버 전용** | 상대 사망 시 승리 |
| 결과 화면 | 서버 → Client RPC | 각 클라에 결과 전달 |
| 재시작 | 서버 | (Phase 2) |

```cpp
// ── 1v1 승패 처리 ─────────────────────────────────────────────
void AApexGameMode::OnPlayerDied(AController* DeadPlayer)
{
    if (!HasAuthority()) return;

    // 1v1 전용: 상대방 = 승자
    AController* Winner = GetOtherPlayer(DeadPlayer);
    // Client RPC로 각 플레이어에게 결과 화면 전달
}

// ── 다인전 확장 시 교체 ────────────────────────────────────────
// GetOtherPlayer()는 1v1 전용. 3명 이상부터 아래 방식으로 교체
void AApexGameMode::OnPlayerDied_FFA(AController* DeadPlayer)
{
    if (!HasAuthority()) return;

    // 1. 킬 기록: EventInstigator(공격자)의 PlayerState에 킬 카운트 추가
    //    → TakeDamage 파라미터의 EventInstigator를 여기까지 넘겨받아야 함
    if (AController* Killer = DeadPlayer->GetInstigatorController())
    {
        if (AApexPlayerState* PS = Killer->GetPlayerState<AApexPlayerState>())
            PS->KillCount++;
    }

    // 2. 생존자 배열에서 제거 후 최후 1인 체크
    ActivePlayers.Remove(DeadPlayer);
    if (ActivePlayers.Num() == 1)
    {
        AController* Winner = ActivePlayers[0];
        // 최후 1인 → 게임 종료
    }
}

// ── 스폰 위치: 생존자와 가장 먼 지점 선택 ────────────────────
// 2주차부터 추가 권장 (3명 이상이면 필수)
AActor* AApexGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
    TArray<AActor*> Starts;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), Starts);

    AActor* BestStart = nullptr;
    float MaxDist = 0.f;

    for (AActor* Start : Starts)
    {
        float MinDistToPlayer = MAX_FLT;
        // 살아있는 다른 플레이어와의 최소 거리 계산
        for (AController* PC : ActivePlayers)
        {
            if (PC == Player || !PC->GetPawn()) continue;
            float D = FVector::Dist(Start->GetActorLocation(),
                                    PC->GetPawn()->GetActorLocation());
            MinDistToPlayer = FMath::Min(MinDistToPlayer, D);
        }
        if (MinDistToPlayer > MaxDist)
        {
            MaxDist = MinDistToPlayer;
            BestStart = Start;
        }
    }
    return BestStart ? BestStart : Super::ChoosePlayerStart_Implementation(Player);
}
```

> **⚠️ 다인전 확장 전 반드시 수정해야 할 3가지**
>
> | 항목 | 현재 (1v1) | 다인전 필요 사항 |
> |------|-----------|----------------|
> | 승패 판정 | `GetOtherPlayer()` — 1v1 전용 | `ActivePlayers` 배열로 생존자 관리 |
> | 킬 기록 | 공격자 추적 없음 | `TakeDamage`의 `EventInstigator` → `AApexPlayerState.KillCount++` |
> | 스폰 위치 | 배열 순서대로 배치 | `ChoosePlayerStart()` 오버라이드 → 생존자와 가장 먼 지점 선택 |

---

## Phase 2 — 추가 기능

- 권총 · 산탄총 추가
- 무기 교체 UI 개선
- 다운 / 부활 시스템 (DBNO)
- Mantle (선반 잡기) 완성
- 맵 점프대 (Launch Pad)
- **`AApexPlayerState` 도입** — 킬·데스·스코어 저장 (다인전 전 필수)
- **`ChoosePlayerStart()` 오버라이드** — 생존자와 먼 위치 스폰
- 1v1v1v1 (4인 확장) 기반 구조

```cpp
// ApexPlayerState.h — 다인전 확장 전 반드시 준비
UPROPERTY(Replicated)
int32 KillCount;

UPROPERTY(Replicated)
int32 DeathCount;

// ── TakeDamage에서 EventInstigator(공격자) 추적하는 흐름 ──────
// AApexCharacter::TakeDamage() 내부에서:
//   AController* Killer = DamageEvent.GetInstigator();  // 공격자 컨트롤러
//   if (AApexPlayerState* PS = Killer->GetPlayerState<AApexPlayerState>())
//       PS->KillCount++;  // 킬 카운트 증가 (서버 전용)
//
// → 이 연결이 없으면 3명 이상일 때 누가 죽였는지 알 수 없음
```

---

## Phase 3 — 미래 기능

- 파쿠르 (벽 달리기 등)
- 로프 상호작용 (Zipline)
- 킬피드 · 미니맵
- 로비 · 매칭 시스템

---

## 팀 역할 분배 (2인 협업 기준)

> Git 머지 충돌을 최소화하려면 **같은 파일을 동시에 건드리지 않도록** 도메인(기능 단위)으로 나눠야 합니다.  
> UI/백엔드 분리보다 **독립적으로 테스트 가능한 기능 묶음** 단위 분할을 권장합니다.

### 🙋 학생 A — 캐릭터 동작 & 애니메이션 담당

**학습 포인트**: `CharacterMovement` 내장 동기화를 이해하고, 커스텀 상태(슬라이딩 등)를 `RepNotify`로 맞추는 방법

| 담당 파일 | 내용 |
|-----------|------|
| `ApexCharacter.h/.cpp` | 입력 바인딩, 카메라, 상태 관리 |
| `ApexCharacterMovement.h/.cpp` | 커스텀 이동 모드 |
| 애니메이션 블루프린트 | 상태별 애니메이션 전환 |

**담당 기능**
- 기본 이동 / 마우스 회전 / 점프
- Sprint / Crouch 상태 동기화
- 슬라이딩 & 슬라이딩 점프 (`Server RPC + RepNotify`)
- 낙하 데미지 판정 트리거
- 클라이언트 연출 (슬라이딩 시 카메라 FOV 변경 등)

> **A의 핵심 과제**: 슬라이딩 상태를 `Server RPC → RepNotify`로 연결해서  
> 서버와 모든 클라이언트 화면에서 자연스럽게 애니메이션이 재생되게 만들기

---

### 🙋 학생 B — 전투 시스템 & 게임 규칙 담당

**학습 포인트**: `Authority` 기반 데미지 판정, `RPC` 통신 흐름, `RepNotify`를 이용한 HUD 업데이트

| 담당 파일 | 내용 |
|-----------|------|
| `WeaponBase.h/.cpp` | 발사, 히트스캔 |
| `HealthComponent.h/.cpp` | HP/실드 복제 |
| `ApexGameMode.h/.cpp` | 승패 판정 |
| HUD UMG | HP바, 탄약 표시 |

**담당 기능**
- 무기 액터 스폰 및 장착
- `LineTrace` 히트스캔 발사 (서버 검증)
- `HealthComponent` 데미지 연산 + `RepNotify` 동기화
- 플레이어 사망 처리 및 `GameMode` 승패 판정
- HUD (HP바·탄약·크로스헤어)

> **B의 핵심 과제**: 클릭 → 탄약 감소 → 데미지 → 사망 → 결과 화면까지  
> 서버 기준으로 버그 없이 한 사이클이 돌아가게 만들기

---

## 파일 구조 (권장)

```
Source/
├── Character/
│   ├── ApexCharacter.h / .cpp          ← 플레이어 캐릭터
│   ├── ApexCharacterMovement.h / .cpp  ← 커스텀 이동
│   └── Components/
│       └── HealthComponent.h / .cpp    ← HP/실드 컴포넌트
├── Weapon/
│   ├── WeaponBase.h / .cpp             ← 무기 베이스 클래스
│   ├── AssaultRifle.h / .cpp           ← 돌격소총
│   └── WeaponInterface.h               ← 무기 인터페이스
├── GameMode/
│   ├── ApexGameMode.h / .cpp           ← 승패 판정
│   ├── ApexGameState.h / .cpp          ← 게임 상태 복제
│   └── ApexPlayerState.h / .cpp        ← 킬·데스·스코어 (Phase 2~)
└── UI/
    └── ApexHUD.h / .cpp                ← HUD 컨트롤러
```

---

## 개발 체크리스트

### 1주차
- [ ] 언리얼 프로젝트 생성 및 Git 설정
- [ ] Listen Server 모드 설정
- [ ] `AApexCharacter` 기본 클래스 작성
- [ ] 이동 / 점프 네트워크 테스트
- [ ] 테스트 레벨 제작

### 2주차
- [ ] Sprint / Crouch 구현
- [ ] 슬라이딩 상태머신
- [ ] 슬라이딩 점프 연계
- [ ] `HealthComponent` 작성 (Replicated)
- [ ] OnRep_Health 바인딩

### 3주차
- [ ] `WeaponBase` 클래스 작성
- [ ] 돌격소총 발사 (Server RPC)
- [ ] LineTrace 히트판정 (서버)
- [ ] Multicast 이펙트 동기화
- [ ] `TakeDamage` → 사망 → GameMode 알림

### 4주차
- [ ] 탄약 시스템 & 재장전
- [ ] UMG HUD 완성 (HP/실드/탄약)
- [ ] GameMode 승패판정 + 결과 화면
- [ ] 낙하 데미지
- [ ] 네트워크 2클라이언트 통합 테스트

---

## 언리얼 핵심 코드 패턴

```cpp
// 변수 복제 (서버 → 클라 자동 동기화)
UPROPERTY(Replicated)
float SomeValue;

UPROPERTY(ReplicatedUsing = OnRep_SomeValue)
float SomeValue;

// 서버에서만 실행 보장
if (HasAuthority()) { /* 서버 전용 로직 */ }

// 로컬 클라이언트에서만 실행
if (IsLocallyControlled()) { /* 입력·카메라 등 */ }

// GetLifetimeReplicatedProps 반드시 구현
void AApexCharacter::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AApexCharacter, Health);
    DOREPLIFETIME(AApexCharacter, bIsSliding);
}
```

---

## 초보자 핵심 팁

### 1. 더미 모델(T포즈)로 먼저 테스트하세요

1~3주차까지는 **애니메이션 없이** 아래 도구만으로 네트워크 로직을 검증하세요.

```
✅ 사용할 것
  - PrintString()          — 서버/클라 구분해서 상태 출력
  - DrawDebugLine()        — LineTrace 궤적 시각화 (빨간 선)
  - T포즈 기본 메시        — 이동·충돌만 확인

❌ 아직 붙이지 말 것
  - 총기 애니메이션
  - 발사 이펙트 (파티클)
  - 사운드
```

> 시스템이 완벽히 작동한 뒤 이펙트·애니메이션을 붙여야,  
> 버그가 네트워크 문제인지 비주얼 문제인지 바로 구분됩니다.

### 2. 발사 로직 — 로컬 FX 즉시 재생 패턴

```
❌ 잘못된 방식 (답답한 조작감)
  클릭 → Server_Fire() → Multicast_FireEffect() → 내 화면에 사운드 재생
  ← RTT(왕복 딜레이) 때문에 클릭 후 피드백이 늦음

✅ 올바른 방식 (빠릿한 조작감)
  클릭 → 내 화면 사운드·이펙트 즉시 재생 (로컬)
       → Server_Fire() (데미지 판정만 서버에 요청)
       → Multicast_FireEffect() (다른 플레이어들에게 내 총격 연출 전송)
```

내 이펙트는 로컬에서 즉시 처리하고, `Multicast`는 **다른 플레이어에게 내 총격을 보여줄 때만** 사용하세요.

---

*마지막 업데이트: 피드백 반영 (다인전 확장 경고, 팀 분배, 초보자 팁)*
