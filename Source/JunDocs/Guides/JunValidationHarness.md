# Jun Validation Harness

## 목적

이 문서는 Jun 담당 시스템을 검증할 때 따르는 하네스 기준이다. 구현이 끝났다고 가정하지 않고, `컴파일 -> 에디터 세팅 -> 단일 기능 검증 -> 멀티플레이 검증 -> 팀 연동 검증` 순서로 확인한다.

## 1. 컴파일 검증

- C++ 프로젝트가 에러 없이 빌드되는지 확인
- `JunGame` 아래 신규 클래스가 Unreal Editor에 정상 반영되는지 확인
- DataTable 행 구조체가 임포트 타입으로 선택 가능한지 확인

## 2. 단일 기능 검증

### 자기장

- `Actor` 기반 `BP_JunRingHost`에 `JunRingComponent`만 붙여도 Ring이 자동 시작되는지
- 매치 시작 후 링이 시작되는지
- 페이즈가 순서대로 진행되는지
- 반지름이 목표값까지 줄어드는지
- 링 밖에서 데미지가 틱 단위로 들어오는지
- CSV 변경 후 리임포트하면 수치가 바뀌는지
- 같은 `JunRingComponent`를 다른 링 호스트 액터에 붙여도 동일하게 동작 가능한지
- `PauseRing`, `ResumeRing`, `StopRing`, `ResetRing`이 서버에서만 상태를 바꾸는지
- 잘못된 DataTable 값이 들어오면 로그에 읽을 수 있는 에러가 남는지

### 데스매치

- 목표 킬 수가 DataTable 값대로 읽히는지
- 사망 후 리스폰이 동작하는지
- 리스폰 횟수가 PlayerState에 반영되는지
- GameState에 현재 리더와 링 상태가 반영되는지

## 3. 멀티플레이 검증

- Listen Server + Client PIE 2인 이상으로 실행
- 링 반경과 단계가 양쪽에서 동일하게 보이는지
- 클라이언트 사망 후 서버 기준으로 리스폰되는지
- GameState 복제값이 클라이언트에서도 읽히는지

## 4. 팀 연동 검증

- 캐릭터 담당 코드와 결합 시 사망 직후 상태 초기화가 필요한지 확인
- 무기 담당 코드와 결합 시 `RegisterKill` 호출 지점을 확정
- 킬 크레딧이 실제 점수에 반영되는지 확인

## 5. 결과 기록 방식

검증이 끝날 때마다 `Logs/JunSessionLog.md`에 아래 네 항목을 남긴다.

- 무엇을 검증했는지
- 무엇이 통과했는지
- 어떤 문제가 있었는지
- 다음 수정이 무엇인지
## UE 5.7 API Validation

- Review newly added engine API usage for UE 5.7 compatibility before closing the task.
- Flag any legacy gameplay iteration code such as `FConstPawnIterator` / `GetPawnIterator()` and replace it with current UE5 traversal APIs.
