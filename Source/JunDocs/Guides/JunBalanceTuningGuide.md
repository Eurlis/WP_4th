# Jun Balance Tuning Guide

## 목적

이 문서는 자기장과 데스매치 수치를 코드 수정 없이 조절하는 기준서다.
자기장 수치는 데스매치 전용 액터가 아니라 `JunRingComponent` 중심 구조를 기준으로 관리한다.

## 자기장 수치 조절

링 수치는 `FJunRingPhaseRow` DataTable에서 조절한다.

### 주요 컬럼

- `PhaseIndex`
  - 자기장 단계 순서
  - 낮은 값부터 먼저 실행된다
- `TargetRadius`
  - 해당 단계가 끝났을 때의 목표 반지름
  - 값이 작을수록 후반 교전이 더 빠르게 붙는다
- `WaitTime`
  - 축소 시작 전 대기 시간
  - 값이 크면 플레이어가 이동할 여유가 늘어난다
- `ShrinkTime`
  - 실제 축소에 걸리는 시간
  - 값이 짧으면 압박감이 커진다
- `DamageInterval`
  - 자기장 바깥 데미지 틱 주기
  - 값이 작을수록 더 자주 맞는다
- `DamagePerTick`
  - 틱당 데미지
  - 값이 클수록 자기장 바깥 생존 시간이 짧아진다

### 추천 조정 방식

- 초반 페이즈는 `WaitTime`을 길게 두고 `DamagePerTick`은 낮게 둔다
- 후반 페이즈는 `WaitTime`을 짧게 두고 `DamagePerTick`을 높인다
- 테스트는 한 번에 하나의 컬럼만 크게 바꾸지 말고, 반지름과 시간 값을 같이 본다

## 데스매치 수치 조절

데스매치 수치는 `FJunDeathmatchSettingsRow` DataTable에서 조절한다.

### 주요 컬럼

- `MatchStartDelay`
  - 매치 시작 전 대기 시간
- `PostMatchDelay`
  - 매치 종료 후 정리 시간
- `TargetKillCount`
  - 승리 조건 킬 수
- `RespawnDelay`
  - 사망 후 리스폰까지 걸리는 시간
- `bStartRingOnBeginPlay`
  - 매치 시작 시 자기장을 자동으로 시작할지 여부
- `bAllowRespawn`
  - 데스매치 리스폰 허용 여부

### 추천 조정 방식

- 빠른 테스트 맵은 `MatchStartDelay`를 짧게 둔다
- 교전 템포가 너무 끊기면 `RespawnDelay`를 낮춘다
- 한 판이 너무 오래 가면 `TargetKillCount`를 낮추거나 링 후반 데미지를 올린다

## 수정 절차

1. CSV 수정
2. Unreal Editor에서 DataTable 리임포트
3. 링 블루프린트와 게임모드 블루프린트에 테이블이 연결되어 있는지 확인
4. PIE에서 변경값 검증

## 변경 시 주의점

- `PhaseIndex`는 중복되지 않게 유지
- `TargetRadius`는 일반적으로 단계가 갈수록 감소하도록 구성
- `DamageInterval`이 0 이하가 되지 않게 유지
- `RespawnDelay`를 너무 낮추면 스폰 직후 교전이 과밀해질 수 있음
