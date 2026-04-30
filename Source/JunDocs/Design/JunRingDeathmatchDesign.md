# Jun Ring And Deathmatch Design

## 목표

Jun 담당 범위는 다음 두 시스템이다.

- 자기장(링) 시스템
- 데스매치 게임모드

두 시스템은 캐릭터 내부 구현이나 무기 내부 구현보다 상위 레벨의 전장 규칙을 담당한다.

## 왜 이 순서로 시작하는가

- 현재 프로젝트는 무기와 투사체 쪽 진행도가 높다.
- `CLAUDE.md` 기준으로 남은 큰 축에 게임모드와 링 데미지가 포함되어 있다.
- 캐릭터 담당자와 무기 담당자가 아직 세부 구현을 진행 중이어도, 게임 규칙 쪽은 독립적으로 먼저 구축할 수 있다.
- 따라서 Jun은 전장 규칙의 뼈대를 먼저 만든 뒤, 나중에 캐릭터/무기와 연결하는 방식으로 가는 것이 병합 충돌이 적다.

## 담당 경계

- Jun
  - 자기장 축소 규칙
  - 자기장 바깥 데미지 적용
  - 매치 시작, 진행, 종료
  - 리스폰 정책
  - 승리 조건
- 캐릭터 담당
  - 이동, 상태, 피격 반응, 사망 연출
- 무기 담당
  - 발사, 재장전, 데미지 계산 상세, 투사체/히트 처리

## 1차 구현 목표

### 자기장

- 서버에서만 자기장 상태를 갱신한다.
- 자기장은 페이즈 배열 기반으로 줄어든다.
- 각 페이즈는 다음 정보를 가진다.
  - 목표 반지름
  - 축소 전 대기 시간
  - 축소 시간
  - 데미지 틱 간격
  - 틱당 데미지
- 자기장 바깥의 `APawn`에게 서버에서 `ApplyDamage`를 호출한다.

### 데스매치 게임모드

- 서버가 매치 시작과 종료를 결정한다.
- 서버가 자기장 액터를 스폰하고 시작시킨다.
- 서버가 리스폰 요청을 처리한다.
- 서버가 킬 카운트 기반 승리 조건을 관리한다.
- 캐릭터/무기 담당 코드와 직접 결합하지 않기 위해, 킬 등록 함수와 리스폰 함수를 먼저 게임모드에 만든다.

## 1차 코드 구조

### 새 클래스

- `AJunDeathmatchGameMode`
  - 매치 설정 보관
  - 자기장 액터 스폰
  - 리스폰 처리
  - 킬 수 누적 및 승리 조건 판단
- `UJunRingComponent`
  - 자기장 반지름과 페이즈 상태 관리
  - 일정 시간마다 축소
  - 일정 간격으로 외곽 플레이어 데미지 적용
  - 다른 모드에서도 재사용 가능한 핵심 로직
- `AJunRingActor`
  - `UJunRingComponent`를 호스팅하는 기본 링 액터
  - 데스매치는 이 액터를 스폰해서 사용

## 밸런스 데이터 관리 방식

- 자기장 단계 값과 데스매치 규칙 값은 코드 기본값을 가지되, `DataTable`이 지정되면 그 값으로 덮어쓴다.
- 이렇게 하면 초기 개발 중에는 코드만으로 바로 실행할 수 있고, 기획 변경이 오면 CSV만 수정해서 다시 임포트하면 된다.

### 자기장 CSV 권장 컬럼

- `Name`
- `PhaseIndex`
- `TargetRadius`
- `WaitTime`
- `ShrinkTime`
- `DamageInterval`
- `DamagePerTick`

### 데스매치 CSV 권장 컬럼

- `Name`
- `MatchStartDelay`
- `PostMatchDelay`
- `TargetKillCount`
- `RespawnDelay`
- `bStartRingOnBeginPlay`
- `bAllowRespawn`

### 적용 흐름

1. CSV 작성
2. Unreal Editor에서 CSV를 `DataTable`로 임포트
3. 자기장용 DataTable은 `AJunRingActor` 내부 `JunRingComponent`의 `RingPhaseDataTable`에 할당
4. 데스매치용 DataTable은 `AJunDeathmatchGameMode`의 `DeathmatchSettingsDataTable`에 할당
5. 필요하면 행 이름은 `DeathmatchSettingsRowName`으로 조정

### 장점

- 기획 변경 시 코드 수정 없이 수치 조정 가능
- 테스트 빌드와 본 밸런스 버전을 테이블만 바꿔서 운영 가능
- 병합 충돌이 코드보다 적다

## 추가 구현 범위

- `AJunDeathmatchPlayerState`
  - 킬, 데스, 리스폰 횟수 복제
- `AJunDeathmatchGameState`
  - 현재 매치 단계, 목표 킬 수, 현재 리더, 링 상태 복제
- `UJunRingComponent`
  - 모드 독립적인 자기장 로직 컴포넌트
- `UJunPawnDeathListener`
  - 기존 캐릭터 파일을 수정하지 않고 사망 이벤트를 관찰하기 위한 프록시
- `UJunRingDamageType`
  - 링 데미지를 구분하기 위한 전용 DamageType

## 현재 한계와 팀 연동 포인트

- 링 데미지와 사망/리스폰 흐름은 Jun 코드만으로 1차 동작한다.
- 반면 무기 킬 크레딧은 현재 캐릭터/무기 원본 구조상 자동 추적이 제한된다.
- 따라서 무기 담당자는 추후 유효한 킬 판정 시 `RegisterKill(KillerController, VictimController)`를 호출해야 한다.
- 이 지점을 먼저 고정해두면 나중에 병합할 때 수정 범위가 작아진다.
- 추가 모드가 생기면 같은 `UJunRingComponent`를 새 모드 전용 링 액터에 붙여 재사용한다.

## 연동 전략

### 캐릭터와의 연동

- 현재 캐릭터는 `TakeDamage`와 `HealthComponent`를 이미 가진다.
- 따라서 자기장은 우선 `UGameplayStatics::ApplyDamage`만 호출하면 된다.
- 리스폰이나 킬 판정 연동은 나중에 캐릭터 사망 이벤트와 연결한다.

### 무기와의 연동

- 무기 담당자는 킬 판정 시 게임모드의 킬 등록 함수만 호출하면 된다.
- 자기장 데미지는 무기 시스템과 직접 결합하지 않는다.

## 구현 순서

1. 자기장/데스매치 설계 문서 고정
2. 새 GameMode 클래스 추가
3. 새 Ring 액터 추가
4. GameMode가 Ring 액터를 스폰하도록 연결
5. Ring 데미지 틱 구현
6. Respawn, KillCount, MatchEnd API 추가
7. 이후 캐릭터/무기 담당 코드와 접점 연결

## 이번 단계 완료 기준

- 새 GameMode와 Ring 액터가 코드상 존재한다.
- GameMode에서 Ring 액터를 스폰하고 시작할 수 있다.
- Ring 액터가 서버에서 페이즈를 진행하고 데미지를 줄 수 있다.
- 이후 병합을 위한 함수 경계가 문서에 정리되어 있다.
