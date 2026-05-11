# Jun Session Log

## 템플릿

아래 블록을 복사해서 최신 기록을 위에 추가한다.

---

### YYYY-MM-DD HH:MM

#### 목표

- 

#### 변경 내용

- 

#### 검증

- 

#### 남은 일

- 

#### 다음 세션 첫 작업

- 

---

## Active Notes

### 2026-05-11 14:31

#### 목표

- `Source/JunDocs` 안의 현재 문서와 CSV를 전부 확인
- 현재 Ring/Deathmatch 구현이 JunDocs 기준과 어긋나는 부분이 있는지 점검
- 필요한 수정이 있으면 코드와 문서에 반영

#### 확인한 내용

- JunDocs 기준상 Ring은 서버 권한이 source of truth이고, GameMode는 match/ring orchestration을 담당한다.
- `BP_JunRingHost + JunRingComponent` 단독 smoke test에서는 `bStartAutomatically=true` 기본값이 유효하다.
- Deathmatch 경로에서는 `AJunDeathmatchGameMode`가 `MatchStartDelay` 이후 `StartDeathmatch`에서 Ring 시작 여부를 제어해야 한다.
- 기존 `AJunDeathmatchGameMode::SpawnRingActor`는 일반 `SpawnActor`를 사용했기 때문에 `AJunRingActor` 내부 `JunRingComponent::BeginPlay`가 먼저 실행되어 `bStartAutomatically=true`로 Ring이 match start 전에 시작될 수 있었다.
- match end 이후 Ring을 명시적으로 멈추는 처리가 부족했다.

#### 변경 내용

- `AJunDeathmatchGameMode::SpawnRingActor`를 deferred spawn 방식으로 변경
- Deathmatch GameMode가 spawn한 Ring actor의 `JunRingComponent->bStartAutomatically`를 `FinishSpawning` 전에 `false`로 설정
- Deathmatch Ring은 `StartDeathmatch`에서 `bStartRingOnBeginPlay`가 true일 때만 시작되도록 정리
- `FinishMatch`와 `HandleMatchHasEnded`에서 `RingActorInstance->StopRing()` 호출 후 GameState Ring 상태를 즉시 동기화
- `JunUnrealSetupChecklist.md`에 Deathmatch Ring 자동 시작 제어, `bStartRingOnBeginPlay=false` 확인, match end 이후 Ring 정지 확인 항목 추가

#### 수정한 파일

- `Source/WP_4th/JunGame/JunDeathmatchGameMode.cpp`
- `Source/JunDocs/Guides/JunUnrealSetupChecklist.md`
- `Source/JunDocs/Logs/JunSessionLog.md`

#### 검증

- `WP_4thEditor Win64 Development` 빌드 실행
- 명령: `C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat WP_4thEditor Win64 Development -Project=C:\Users\user\Documents\GitHub\WP_4th\WP_4th.uproject -WaitMutex`
- 결과: 성공

#### 남은 일

- Unreal Editor에서 `BP_JunRingHost + JunRingComponent` 단독 smoke test 실행
- Unreal Editor에서 Jun Deathmatch GameMode 경로 검증
- `bStartRingOnBeginPlay=false`일 때 Ring이 자동 시작하지 않는지 PIE 확인
- match end 이후 Ring damage가 멈추는지 PIE 확인
- Shield가 남아있는 캐릭터가 Ring 밖에 있을 때 Shield는 유지되고 Health만 감소하는지 PIE 확인

#### 다음 세션 첫 작업

- 위 PIE 체크리스트를 실제 에디터에서 실행하고 결과를 이 로그에 이어서 기록

### 2026-05-11 14:21

#### 목표

- 캐릭터의 실제 체력(`Health`)과 방어막/아머 역할의 `Shield` 데미지 처리 흐름 재확인
- 링 외부 데미지는 Shield 유무와 상관없이 실제 체력에 직접 적용되도록 변경
- Jun 관련 작업 대화와 결과를 다음 새 채팅/새 환경에서도 추적 가능하게 문서 기록

#### 확인한 내용

- `UHealthComponent::ApplyDamage`는 기존에 `Shield`를 먼저 깎고 남은 데미지만 `Health`에 적용한다.
- `UJunRingComponent::ApplyRingDamage`는 링 밖 `APawn`에게 `UGameplayStatics::ApplyDamage(..., RingDamageType)`를 호출한다.
- `AApexCharacterBase::TakeDamage`는 링 데미지도 일반 데미지와 같은 `HealthComponent->ApplyDamage` 경로로 넘기고 있었다.
- 따라서 기존 상태에서는 링 외부 데미지도 Shield가 있으면 Health가 바로 감소하지 않았다.
- JunDocs 운영 규칙상 세션 기록은 이 파일(`Logs/JunSessionLog.md`)에 남긴다.

#### 변경 내용

- `UHealthComponent::ApplyHealthDamage(float RawDamage)` 추가
- `ApplyHealthDamage`는 서버 권한에서만 실행되며 Shield를 건드리지 않고 `Health`만 감소시킨다.
- `AApexCharacterBase::TakeDamage`에서 `DamageEvent.DamageTypeClass`가 `UJunRingDamageType` 계열이면 `ApplyHealthDamage`를 호출하도록 변경
- 일반 데미지는 기존 `ApplyDamage` 경로를 유지해서 Shield 우선 감소 동작을 보존
- `JunEngineeringHarness.md`에 대화 연속성 규칙을 추가해서 새 세션 시작 시 `JunReadme.md`와 최신 `JunSessionLog.md`를 먼저 확인하도록 명시

#### 수정한 파일

- `Source/WP_4th/Character/Components/HPComp/HealthComponent.h`
- `Source/WP_4th/Character/Components/HPComp/HealthComponent.cpp`
- `Source/WP_4th/Character/ApexCharacterBase.cpp`
- `Source/JunDocs/Overview/JunEngineeringHarness.md`
- `Source/JunDocs/Logs/JunSessionLog.md`

#### 검증

- `WP_4thEditor Win64 Development` 빌드 실행
- 명령: `C:\Program Files\Epic Games\UE_5.7\Engine\Build\BatchFiles\Build.bat WP_4thEditor Win64 Development -Project=C:\Users\user\Documents\GitHub\WP_4th\WP_4th.uproject -WaitMutex`
- 결과: 성공

#### 남은 일

- PIE에서 Shield가 남아있는 캐릭터를 링 밖에 둔 뒤 Health만 감소하고 Shield가 유지되는지 수동 확인
- 일반 무기/투척물 데미지는 여전히 Shield를 먼저 깎는지 수동 확인

#### 다음 세션 첫 작업

- `JunSessionLog.md` 최신 기록을 먼저 읽고, 링 데미지 PIE 검증 결과를 이어서 기록

### 2026-04-28

#### 목표

- 새 Ring 작업 지침을 기존 Jun 링 제작 흐름에 통합
- 기존 구현과 비교해 채택할 설계 기준 정리

#### 변경 내용

- `AGENTS.md` 추가
- `plans/zone_component_plan.md` 추가
- `docs/reports/zone_component_research.md` 추가
- `docs/reports/zone_component_api.md` 추가
- `JunReadme.md`와 `JunEngineeringHarness.md`에 새 Ring 계획 문서 링크 반영
- 확장된 round-based team elimination Ring 계획 반영
- 공식 Unreal 문서 기반 citation을 research report에 반영
- API 문서에 CSV schema, replication snapshot, Blueprint hooks, Mermaid diagrams, test priorities 추가

#### 검증

- 기존 `JunRingComponent`, `JunDeathmatchGameState`, `JunBalanceData`를 확인하고 새 지침과의 차이를 문서화
- 코드 추가 구현은 계획 승인 전 보류
- `plans/zone_component_plan.md`, `docs/reports/zone_component_research.md`, `docs/reports/zone_component_api.md`를 UTF-8로 읽어 한글 출력 확인

#### 남은 일

- `plans/zone_component_plan.md` 승인 여부 확인
- 승인 후 `JunRing*` 추가 또는 기존 `JunRing*` 리팩터링 방향 결정
- Option B 기준 `AJunRingStateActor + UJunRingComponent` 구현 여부 결정

#### 다음 세션 첫 작업

- 승인된 방향에 따라 Ring runtime snapshot과 DataTable validation부터 구현

### 2026-04-24

#### 목표

- Jun 전용 문서 하네스 시작
- 현재 프로젝트 목표와 진행 상태 확인
- Jun 담당 범위를 자기장 시스템과 데스매치 게임모드로 확정

#### 변경 내용

- `JunReadme.md` 추가
- `Source/JunDocs/Overview/JunProjectCharter.md` 추가
- `Source/JunDocs/Overview/JunEngineeringHarness.md` 추가
- `Source/JunDocs/Overview/JunBacklog.md` 추가
- `Source/JunDocs/Logs/JunSessionLog.md` 추가
- `CLAUDE.md` 기준으로 현재 프로젝트 목표와 진행 상태 재확인
- Jun 담당을 자기장(링) 시스템과 데스매치 게임모드로 문서 반영
- 팀 작업 경계를 문서에 반영
- `Source/JunDocs/Design/JunRingDeathmatchDesign.md` 추가
- `AJunDeathmatchGameMode`, `AJunRingActor` 초안 코드 추가
- `AJunDeathmatchPlayerState`, `AJunDeathmatchGameState`, `UJunPawnDeathListener`, `UJunRingDamageType` 추가
- DataTable 기반 밸런스 구조 확장
- Unreal Editor 세팅 체크리스트와 밸런스 튜닝 가이드 추가
- 하네스 기반 검증 문서 추가

#### 검증

- 프로젝트 루트 구조와 `WP_4th.uproject`, `Source/WP_4th/WP_4th.Build.cs` 기준으로 문서 연결 확인
- `CLAUDE.md`의 목표, 진행상황, 서버 권한 규칙과 Jun 문서 방향 일치 여부 확인

#### 남은 일

- 자기장 시스템 상세 요구사항 정리
- 데스매치 게임모드 상세 요구사항 정리
- 두 시스템 설계 문서 추가
- 킬 등록과 리스폰을 캐릭터 사망 흐름에 연결
- 블루프린트 또는 맵에서 새 GameMode 적용
- 실제 컴파일 및 PIE 검증
- 무기 담당자와 킬 크레딧 연동

#### 다음 세션 첫 작업

- 자기장 규칙과 데스매치 규칙을 문서로 먼저 확정
### 2026-04-24 API Hygiene Follow-up

#### Goal

- Prevent reintroduction of legacy Unreal iterator patterns in Jun-owned code
- Update JunDocs so UE 5.7 API checks are explicit during implementation and validation

#### Changes

- Added a `UE 5.7 API Hygiene` section to `Source/JunDocs/JunReadme.md`
- Added explicit modern-API rules to `Source/JunDocs/Overview/JunEngineeringHarness.md`
- Added re-check items to `Source/JunDocs/Guides/JunUnrealSetupChecklist.md`
- Added validation rules to `Source/JunDocs/Guides/JunValidationHarness.md`

#### Validation

- Re-checked JunDocs files that govern implementation, setup, and validation flow
- Confirmed the docs now explicitly reject `FConstPawnIterator` and `GetPawnIterator()` for UE 5.7 gameplay code

#### Follow-up

- Apply the same API hygiene standard whenever external snippets or old Unreal tutorials are referenced

### 2026-04-28 Ring Component Drop-in Pass

#### Goal

- Standardize current Jun documentation terminology on `Ring`
- Make `UJunRingComponent` useful as a drop-in component on a simple host actor in any test mode
- Keep the implementation scoped to Jun-owned Ring files

#### Changes

- Expanded `FJunRingPhaseRow` with optional CSV/DataTable fields for radius, delay, shrink duration, DPS, center mode, warning lead time, and notes
- Kept backward compatibility with existing `WaitTime`, `ShrinkTime`, `DamageInterval`, and `DamagePerTick`
- Added `StopRing`, `PauseRing`, `ResumeRing`, `ResetRing`, `ResetForRound`, `AdvanceToPhase`, and `ReloadRingData`
- Added `InitFromDataTable` and `InitFromPhaseRows` to `UJunRingComponent`
- Added matching control/read wrapper APIs to `AJunRingActor`
- Enabled default auto-start and debug draw for fast PIE verification
- Added readable Ring phase validation logs
- Updated setup and validation checklists for the `BP_JunRingHost + JunRingComponent` smoke test path

#### Validation

- Built `WP_4thEditor Win64 Development` with UE 5.7 `Build.bat`
- Result: succeeded

#### Follow-up

- In Unreal Editor, create `BP_JunRingHost`, add `JunRingComponent`, place it in a test map, and run the checklist in `Source/JunDocs/Guides/JunUnrealSetupChecklist.md`
- Next coding pass should focus on GameState-facing minimal Ring snapshot if UI or multiplayer HUD binding needs it

### 2026-04-28 Ring Shrink Smoke Test Fix

#### Goal

- Fix the case where debug draw appears but the Ring does not shrink during the standalone smoke test

#### Cause

- The checklist told the user to lower `InitialRadius` to `3000`, but built-in default phase targets were authored for `InitialRadius=10000` (`8000`, `4000`, `1500`)
- This could make validation reject the phase data because target radius was larger than the current radius
- The previous built-in first wait time was also 30 seconds, which was too slow for a smoke test

#### Changes

- Built-in Ring phases now scale down automatically when no DataTable is assigned and `InitialRadius` is lower than the authored default
- Built-in no-DataTable phase timings are now smoke-test friendly: first shrink starts after 3 seconds
- Existing Blueprint instances with older built-in no-DataTable phase timings are clamped to smoke-test-friendly values at runtime
- Added `StartRing`, `BeginPhase`, `StartShrink`, and `CompletePhase` logs
- Updated `JunUnrealSetupChecklist.md` to mention the 3-second wait and Output Log checks

#### Validation

- Built `WP_4thEditor Win64 Development` with UE 5.7 `Build.bat`
- Result: succeeded

#### Follow-up

- Re-run the `BP_JunRingHost + JunRingComponent` standalone test
- If the sphere still does not shrink, check Output Log for `JunRingComponent: StartRing`, `BeginPhase`, and `StartShrink`

### 2026-04-28 AGENTS Merge Into JunDocs

#### Goal

- Remove the separate root-level agent guideline document from the Jun workflow
- Keep the same engineering rules inside Jun-owned documentation

#### Changes

- Merged the `AGENTS.md` working mode, Ring architecture rules, data rules, deliverables, milestone reporting, and validation policy into `Source/JunDocs/Overview/JunEngineeringHarness.md`
- Added a short pointer in `Source/JunDocs/JunReadme.md` that Codex/AI rules are managed through JunDocs
- Removed the standalone root `AGENTS.md`

#### Validation

- Confirmed the JunDocs entry point and engineering harness now contain the relevant AGENTS rules

### 2026-04-30 Ring Client Sync Fix

#### Goal

- Fix the issue where the Ring shrinks on the server but the client debug Ring does not shrink or sync to the server radius

#### Cause

- A generic `BP_JunRingHost` actor may not have replication enabled, which prevents `UJunRingComponent` replicated fields from reaching clients
- Clients were drawing the last replicated `CurrentRadius` only and were not locally interpolating from replicated shrink timing

#### Changes

- `UJunRingComponent` enables replication on its owner at runtime when running with authority
- Replicated `PhaseStartRadius`, `PhaseTargetRadius`, `ShrinkStartTime`, `ShrinkEndTime`, and `PhaseStateEndTime`
- Clients now run the same visual radius interpolation path while damage remains server-only
- Phase transitions call `ForceNetUpdate()` so clients receive transition snapshots promptly
- Ring time now uses `GameState->GetServerWorldTimeSeconds()` when available
- Updated the checklist to require `BP_JunRingHost` Class Defaults `Replicates` for multiplayer checks

#### Validation

- Built `WP_4thEditor Win64 Development` with UE 5.7 `Build.bat`
- Initial build exposed a UE 5.7 deprecation warning for direct `NetUpdateFrequency` access
- Replaced it with `SetNetUpdateFrequency/GetNetUpdateFrequency`
- Final build result: succeeded
