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

## 문서 운영 규칙

- 방향성은 `Overview/JunProjectCharter.md` 기준으로 관리한다.
- 우선순위는 `Overview/JunBacklog.md`에 반영한다.
- 실제 세션 기록은 `Logs/JunSessionLog.md`에 남긴다.
- 긴 설계는 `docs/` 아래 `Jun*.md` 파일로 확장한다.

## 코드 작업 규칙

- 기존 구조를 읽고 그 위에 얹는다.
- 한 번에 하나의 책임만 바꾼다.
- Unreal C++ 네이밍과 구조를 따른다.
- 테스트가 없으면 최소한 수동 검증 기준이라도 적는다.

## 세션 종료 시 반드시 남길 것

- 완료한 것
- 검증한 것
- 남은 일
- 다음 세션 첫 작업

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
