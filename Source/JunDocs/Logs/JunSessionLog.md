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
