# WP_4th Jun Workspace Notes

이 문서는 `WP_4th` 프로젝트에서 Jun 작업 범위를 구분하기 위한 개인 진입 문서다.

## 목적

- 3인 병렬 작업 환경에서 Jun 담당 문서를 명확히 분리
- AI가 문서 소유자를 혼동하지 않도록 접두사 규칙 고정
- 코드 작업 전에 Jun 기준의 작업 하네스 확보

## 문서 규칙

- Jun이 작성하는 마크다운 문서는 모두 `Jun` 접두사를 사용한다.
- 예시: `JunReadme.md`, `JunBacklog.md`, `JunSessionLog.md`
- 공용 설정 파일이 아닌 이상, 소유자 표시 없는 신규 `md`는 만들지 않는다.

## 현재 프로젝트 기준 정보

- 엔진 버전: UE 5.7
- 메인 프로젝트 파일: `WP_4th.uproject`
- 메인 소스 루트: `Source/WP_4th`
- 주요 사용 모듈: `EnhancedInput`, `AIModule`, `StateTreeModule`, `GameplayStateTreeModule`, `UMG`

## Jun 문서 구조

- `../../plans/`
  - [zone_component_plan](../../plans/zone_component_plan.md)
- `../../docs/reports/`
  - [zone_component_research](../../docs/reports/zone_component_research.md)
  - [zone_component_api](../../docs/reports/zone_component_api.md)
- `Overview/`
  - [JunProjectCharter](Overview/JunProjectCharter.md)
  - [JunEngineeringHarness](Overview/JunEngineeringHarness.md)
  - [JunBacklog](Overview/JunBacklog.md)
- `Design/`
  - [JunRingDeathmatchDesign](Design/JunRingDeathmatchDesign.md)
- `Data/`
  - [JunRingBalanceSample.csv](Data/JunRingBalanceSample.csv)
  - [JunDeathmatchSettingsSample.csv](Data/JunDeathmatchSettingsSample.csv)
- `Guides/`
  - [JunUnrealSetupChecklist](Guides/JunUnrealSetupChecklist.md)
  - [JunBalanceTuningGuide](Guides/JunBalanceTuningGuide.md)
  - [JunValidationHarness](Guides/JunValidationHarness.md)
- `Logs/`
  - [JunSessionLog](Logs/JunSessionLog.md)

## 작업 원칙

- 새 작업은 먼저 Jun 문서에 기록한다.
- 기능 구현 전 목적, 영향 범위, 검증 방법을 적는다.
- 공용 코드 변경은 하되, 문서는 Jun 소유가 분명하게 남긴다.
- Ring 관련 추가 구현은 `plans/zone_component_plan.md` 승인 후 마일스톤 단위로 진행한다.
- Codex/AI 작업 규칙은 루트 `AGENTS.md`가 아니라 [JunEngineeringHarness](Overview/JunEngineeringHarness.md)에 통합해서 관리한다.
- 저장소를 먼저 검사하고 확인된 사실, 가정, 변경 이유, 검증 결과를 남긴 뒤 구현한다.
- 네트워크 동작이 바뀌면 서버 권한 흐름과 복제 영향을 반드시 기록한다.
- CSV/DataTable 값은 기획 변경 가능성이 높으므로 코드 상수보다 DataTable authoring 경로를 우선한다.

## Jun 현재 담당

- Jun의 담당 범위는 자기장(링) 시스템과 데스매치 게임모드다.
- 자기장은 일정 시간마다 안전 구역이 줄어들어야 한다.
- 안전 구역 밖에 있는 플레이어 또는 대상은 주기적으로 데미지를 받아야 한다.
- 데스매치 게임모드는 매치 시작, 진행, 종료, 승패 조건, 스폰 흐름을 관리해야 한다.
- 두 시스템 모두 멀티플레이 기준에서 서버 권한으로 처리하는 방향을 기본 전제로 둔다.
- 자기장 로직은 `JunRingComponent` 중심으로 컴포넌트화해서, 다른 모드에도 재사용 가능한 구조로 유지한다.

## 팀 작업 경계

- Jun: 자기장(링) 시스템, 데스매치 게임모드
- 작업자 A: 캐릭터 시스템
- 작업자 B: 무기 시스템
- Jun 작업은 캐릭터/무기 구현 내부보다 게임 규칙과 전장 제어에 집중한다.
- 병합 시 충돌을 줄이기 위해 공용 인터페이스와 연동 지점은 먼저 문서로 고정한다.
## UE 5.7 API Hygiene

- Do not introduce legacy Unreal iteration APIs when a current UE5 alternative exists.
- Avoid deprecated or brittle patterns such as `FConstPawnIterator`, `GetPawnIterator()`, and old iterator-specific headers when the same intent can be expressed with `TActorIterator`, `TActorRange`, or current subsystem/query APIs.
- Before adding engine-facing code, check whether the API matches the project's current engine target: `UE 5.7`.
- If code was copied from an older tutorial, sample, forum post, or UE4-era snippet, re-check it against the current engine headers before committing it.
- Prefer patterns that are stable across recent UE5 versions and that do not depend on legacy iterator typedefs.
