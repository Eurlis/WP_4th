# Jun Unreal Setup Checklist

아래 체크리스트는 Jun 코드가 추가된 뒤, 사용자가 Unreal Editor에서 직접 확인하거나 세팅해야 하는 항목들이다.

## 컴파일 전

1. 솔루션 탐색기에서 `Source/WP_4th/JunGame` 폴더 아래 새 클래스들이 보이는지 확인
2. `JunDeathmatchGameMode`, `JunDeathmatchGameState`, `JunDeathmatchPlayerState`, `JunRingActor`, `JunRingComponent`가 모두 생성됐는지 확인
3. 에디터를 열기 전에 C++ 프로젝트 컴파일

## DataTable 준비

4. `Source/JunDocs/Data/JunRingBalanceSample.csv`를 참고해서 링 밸런스 CSV 작성
5. `Source/JunDocs/Data/JunDeathmatchSettingsSample.csv`를 참고해서 데스매치 설정 CSV 작성
6. 링 CSV를 `FJunRingPhaseRow` 타입의 DataTable로 임포트
7. 데스매치 CSV를 `FJunDeathmatchSettingsRow` 타입의 DataTable로 임포트
8. 임포트 후 각 열이 의도한 타입으로 들어갔는지 확인
9. 링 DataTable에서 페이즈 순서가 `PhaseIndex` 기준으로 맞는지 확인
10. 데스매치 DataTable에서 행 이름이 `Default`인지 확인

## 블루프린트 세팅

11. `AJunRingActor` 기반 블루프린트 생성
12. 링 블루프린트 안의 `JunRingComponent`를 선택
13. `JunRingComponent`의 `RingPhaseDataTable` 연결
14. 필요하면 `JunRingComponent`의 `InitialRadius` 기본값 조정
15. 필요하면 `JunRingComponent`의 `bEnableDebugDraw`를 켜서 테스트용 시각화 활성화
16. `AJunDeathmatchGameMode` 기반 블루프린트 생성
17. 생성한 게임모드 블루프린트에 `DeathmatchSettingsDataTable` 연결
18. `DeathmatchSettingsRowName`이 실제 행 이름과 일치하는지 확인
19. 게임모드 블루프린트에 링 블루프린트 클래스를 `RingActorClass`로 연결
20. 필요한 기본 Pawn 클래스와 PlayerController 클래스를 게임모드에서 지정

## 맵 세팅

21. 테스트용 맵에 `PlayerStart`를 최소 2개 이상 배치
22. 멀티플레이 테스트용이면 스폰 위치가 서로 겹치지 않게 조정
23. World Settings에서 기본 GameMode를 `BP_JunDeathmatchGameMode`로 설정
24. Maps & Modes 프로젝트 설정에서도 기본 게임모드를 확인

## 링/데스매치 테스트

25. PIE 1인 플레이로 링이 자동 시작하는지 확인
26. 시간이 지나면 링 반지름이 줄어드는지 확인
27. 링 바깥에 나가면 일정 간격으로 데미지를 받는지 확인
28. 사망 후 리스폰이 되는지 확인
29. 데스매치 킬 목표치가 DataTable 값대로 반영되는지 확인
30. `JunDeathmatchPlayerState`의 킬/데스/리스폰 수가 갱신되는지 확인
31. 멀티플레이 PIE에서 링 상태와 리스폰이 모든 클라이언트에 일관되게 보이는지 확인

## 후속 연동

32. 무기 담당자에게 `RegisterKill(AController* KillerController, AController* VictimController)` 호출 지점을 전달
33. 캐릭터 담당자와 리스폰 직후 상태 초기화 필요 여부를 협의
34. HUD 담당이 생기면 `JunDeathmatchGameState` 값을 바인딩 대상으로 전달
35. 다른 모드에서 재사용하려면 `JunRingComponent`를 가진 새 링 호스트 액터 또는 모드 전용 링 액터를 생성
## UE 5.7 API Re-check

4. When touching gameplay traversal or world queries, re-check that the code uses UE 5.7-safe APIs rather than legacy iterator patterns.
5. Reject code that introduces `FConstPawnIterator`, `GetPawnIterator()`, or similar outdated UE4-style traversal unless there is a documented engine requirement.
