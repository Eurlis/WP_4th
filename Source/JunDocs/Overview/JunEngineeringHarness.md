# Jun Engineering Harness

## 목적

이 문서는 Jun이 이 프로젝트를 진행할 때 쓰는 최소 작업 하네스다. 핵심은 작업 전에 기준을 쓰고, 작업 후 검증 흔적을 남기는 것이다.

## 기본 루프

1. 작업 목적을 문서에 적는다.
2. 영향을 받는 코드와 자산 범위를 확인한다.
3. 최소 변경으로 구현한다.
4. 검증 결과를 기록한다.
5. 남은 이슈와 다음 작업을 남긴다.

## 시작 전 체크리스트

- 무엇을 바꾸는가
- 왜 지금 필요한가
- 어느 파일과 시스템이 영향 받는가
- 완료 기준이 무엇인가
- 어떤 방식으로 검증할 것인가

## 저장소 작업 모드

- 이 저장소는 Unreal Engine 멀티플레이 게임 프로젝트로 취급한다.
- Codex는 저장소를 인지하는 엔지니어링 팀원으로 동작하되, 임의로 전체 구조를 재설계하지 않는다.
- 항상 저장소를 먼저 검사하고, 확인된 사실을 문서화한 뒤 가정한다.
- Unreal Engine 버전, GameMode/GameState/PlayerState 클래스명, 플레이어 수, plugin availability, CSV workflow는 추정하지 않는다.
- 불가피한 가정은 명시하고 범위를 최소화한다.

## 문서 운영 규칙

- 방향성은 `Overview/JunProjectCharter.md` 기준으로 관리한다.
- 우선순위는 `Overview/JunBacklog.md`에 반영한다.
- 실제 세션 기록은 `Logs/JunSessionLog.md`에 남긴다.
- 긴 설계는 `docs/` 아래 `Jun*.md` 파일로 확장한다.
- Ring 시스템의 공식 계획은 `plans/zone_component_plan.md`를 기준으로 한다.
- Ring 시스템의 공식 보고서는 `docs/reports/zone_component_research.md`와 `docs/reports/zone_component_api.md`를 기준으로 한다.
- Ring 구현 코드는 계획 승인 후 마일스톤 단위로만 진행한다.
- Ring 작업은 research-first 방식으로 진행하며, broad implementation 전에 Milestone 1 산출물 승인을 받는다.
- 네트워크 동작이 바뀌는 변경은 authority flow와 replication impact를 함께 기록한다.
- 루트 `AGENTS.md`는 별도 기준 문서로 유지하지 않는다. Jun 작업 규칙은 이 문서와 `JunReadme.md`에 통합한다.

## Ring 아키텍처 규칙

- 서버 권한이 Ring 상태와 phase 전환의 source of truth다.
- GameMode 코드는 authority/orchestration으로 취급하고, GameState 또는 replicated Ring actor/component는 client-visible state 경로로 취급한다.
- UI나 client-side logic은 GameMode-only state에 의존하지 않는다.
- 클라이언트에는 최소 런타임 스냅샷만 복제한다.
- 최소 스냅샷 기준: current phase index, current center, target center, current radius, target radius, phase start time 또는 elapsed time, started/paused/shrinking flags.
- per-tick damage 결과는 복제하지 않는다.
- 불필요한 Tick보다 timer-driven 또는 state-driven update를 우선한다.
- UI-facing hook은 Blueprint-friendly API로 열되, 핵심 Ring 로직은 C++에 둔다.

## Ring 재사용 규칙

- 저장소가 단일 GameMode 계층을 강제하지 않는 한 Ring 기능을 특정 GameMode에 hard-code하지 않는다.
- 우선 고려할 decoupling pattern은 `UJunRingComponent`를 가진 lightweight host actor, GameState의 최소 replicated state, 필요한 경우 UInterface 계약이다.
- GameMode에 ActorComponent를 직접 붙이는 것이 기술적으로 애매하면, 목표를 유지한다: "GameMode에 쉽게 붙일 수 있는 authority integration + clients가 읽을 수 있는 replicated state path".
- 현재 빠른 검증 경로는 `BP_JunRingHost + JunRingComponent`를 맵에 배치하는 방식이다.

## 데이터 규칙

- CSV -> DataTable workflow를 1급 authoring path로 지원한다.
- 기존 row struct와 naming convention을 조사한 뒤 새 row/asset을 추가한다.
- Editor reimport와 packaged runtime reload 차이는 명확히 문서화한다.
- CSV/DataTable validation은 조용히 실패하지 말고 읽을 수 있는 로그를 남긴다.

## 코드 작업 규칙

- 기존 구조를 읽고 그 위에 얹는다.
- 한 번에 하나의 책임만 바꾼다.
- Unreal C++ 네이밍과 구조를 따른다.
- 테스트가 없으면 최소한 수동 검증 기준이라도 적는다.
- 계획 우선으로 진행한다. 단, 사용자가 명시적으로 구현을 승인한 마일스톤은 좁은 범위에서 끝까지 진행한다.
- broad rename이나 unrelated replication policy 변경은 피한다.
- third-party plugin은 명시적 필요성과 승인 없이는 추가하지 않는다.

## 필수 산출물

- 한국어 research/design report
- API specification table
- CSV schema proposal and sample CSV
- DataTable load / validation / reload path 설명 또는 코드
- phase progression pseudocode와 Mermaid flowchart
- GameMode/GameState integration example
- prioritized test scenario list
- 구현 후 validation 기록

## 마일스톤 보고 형식

항상 아래 항목을 남긴다.

1. 무엇을 검사했는지
2. 무엇을 바꿨는지
3. 왜 바꿨는지
4. 수정한 파일
5. 리스크와 미확정 사항
6. 수행한 검증
7. 다음 권장 마일스톤

## 검증 정책

- 기존 build/test/automation command가 있으면 사용한다.
- 테스트 하네스가 없으면 최소 실용 검증 경로를 만든다.
- 가능하면 data validation, phase progression, authority-only execution, replication snapshot correctness를 검증한다.
- 검증 실패 시 범위 확장을 멈추고 수정하거나 명확히 보고한다.

## 세션 종료 시 반드시 남길 것

- 완료한 것
- 검증한 것
- 남은 일
- 다음 세션 첫 작업

## 대화 연속성 규칙

- Jun 관련 작업 또는 링/데스매치/캐릭터 데미지 연동 작업을 새 채팅이나 새 환경에서 이어갈 때는 먼저 `Source/JunDocs/JunReadme.md`와 `Source/JunDocs/Logs/JunSessionLog.md`의 최신 기록을 확인한다.
- 사용자와 대화하면서 확정한 구현 의도, 실제 코드 변경, 검증 결과, 남은 리스크는 매 세션 종료 전에 `Logs/JunSessionLog.md`에 남긴다.
- 기록되지 않은 대화 내용은 다음 세션에서 유지된다고 가정하지 않는다.

## Jun 현재 작업 초점

- Jun은 자기장(링) 시스템과 데스매치 게임모드를 담당한다.
- 자기장 핵심 요구사항은 다음 두 가지다.
  - 일정 시간마다 안전 구역이 줄어들 것
  - 안전 구역 밖 대상에게 주기 데미지를 줄 것
- 데스매치 게임모드 핵심 요구사항은 다음과 같다.
  - 매치 시작과 종료를 제어할 것
  - 점수 또는 킬 기준 승패를 관리할 것
  - 스폰과 리스폰 흐름을 관리할 것
- 다른 팀원의 핵심 범위는 캐릭터와 무기이므로, Jun은 게임 규칙과 월드 룰 제어에 집중한다.
- 네트워크 처리 기준은 `CLAUDE.md`의 서버 권한 원칙을 따른다.
## UE 5.7 API Hygiene

- Engine API hygiene is mandatory for gameplay code.
- For UE 5.7, do not add legacy iterator code such as `FConstPawnIterator` or `GetPawnIterator()`.
- Prefer `TActorIterator`, `TActorRange`, and current UE5 APIs over tutorial-era snippets.
- Treat copied sample code as untrusted until it is re-checked against the current engine version and headers.
- If an API choice is version-sensitive, document the chosen modern alternative in the related JunDocs note or session log.
