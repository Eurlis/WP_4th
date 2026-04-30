# Jun Ring Unreal Setup Checklist

이 체크리스트의 1차 목표는 데스매치 모드 완성 여부와 무관하게, 아무 테스트 맵/모드에서 `JunRingComponent`를 붙이면 Ring이 서버에서 시작되고 줄어드는지 확인하는 것이다.

## A. 컴파일 전 확인

1. 솔루션 탐색기에서 `Source/WP_4th/JunGame` 폴더가 보이는지 확인
2. `JunRingComponent`, `JunRingActor`, `JunRingDamageType`, `JunBalanceData`가 보이는지 확인
3. C++ 프로젝트를 컴파일
4. 컴파일 실패 시 에러 메시지의 첫 번째 C++ 에러부터 확인

## B. 가장 빠른 Ring 단독 테스트

5. 테스트 맵을 하나 연다
6. `Actor` 기반 블루프린트 `BP_JunRingHost`를 만든다
7. `BP_JunRingHost`에 `JunRingComponent`를 추가한다
8. `JunRingComponent`에서 `bStartAutomatically`가 켜져 있는지 확인한다
9. `bEnableDebugDraw`가 켜져 있는지 확인한다
10. `InitialRadius`를 테스트하기 쉽게 `3000` 정도로 낮춘다
11. `RingPhases` 배열을 DataTable 없이 기본값 그대로 둔다
12. DataTable 없는 기본 phase는 단독 테스트용으로 3초 대기 후 축소가 시작된다
13. `InitialRadius`를 낮추면 기본 phase target radius도 비율에 맞게 자동 조정된다
14. `BP_JunRingHost`를 맵 중앙에 배치한다
15. PIE 1인 플레이를 실행한다
16. 시안색 debug sphere가 보이는지 확인한다
17. 3초 정도 기다린 뒤 반지름이 줄어드는지 확인한다
18. Output Log에서 `JunRingComponent: StartRing`, `BeginPhase`, `StartShrink` 로그가 나오는지 확인한다
19. 플레이어가 Ring 밖에 있으면 데미지를 받는지 확인한다

## C. CSV / DataTable 테스트

20. `Source/JunDocs/Data/JunRingBalanceSample.csv`를 참고해 CSV를 준비한다
21. Unreal Editor에서 CSV를 `FJunRingPhaseRow` DataTable로 임포트한다
22. `BP_JunRingHost`의 `JunRingComponent.RingPhaseDataTable`에 연결한다
23. PIE를 다시 실행해 CSV 값대로 대기 시간, 축소 시간, 데미지가 바뀌는지 확인한다
24. CSV를 수정한 경우 Editor에서 DataTable을 리임포트한 뒤 다시 PIE를 실행한다

## D. 아무 GameMode에서 붙이는 경로

25. 기존 테스트용 GameMode 블루프린트를 하나 연다
26. 빠른 서버 단독 확인이면 GameMode 블루프린트에 `JunRingComponent`를 직접 추가한다
27. `bUseOwnerLocationAsCenter`가 켜져 있으면 GameMode 위치 기준이므로, 필요하면 끄고 `RingCenter`를 직접 지정한다
28. 멀티플레이에서 클라이언트 시각화까지 확인하려면 `BP_JunRingHost` 또는 `AJunRingActor`를 맵에 배치하거나 GameMode가 spawn하게 한다
29. GameMode가 직접 Ring을 제어해야 할 때만 `AJunRingActor`를 spawn하거나 `UJunRingComponent`를 가진 host actor를 참조한다
30. 이 방식이면 Deathmatch, Team Elimination, 임시 테스트 모드 모두 같은 Ring host를 재사용할 수 있다

## E. Jun Deathmatch 연동 테스트

31. `AJunDeathmatchGameMode` 기반 블루프린트를 만든다
32. `AJunRingActor` 기반 블루프린트를 만들고 `RingPhaseDataTable`을 연결한다
33. GameMode 블루프린트의 `RingActorClass`에 Ring actor 블루프린트를 연결한다
34. `bStartRingOnBeginPlay`가 켜져 있는지 확인한다
35. World Settings에서 GameMode를 Jun Deathmatch GameMode 블루프린트로 지정한다
36. PIE에서 match start 후 Ring이 시작되는지 확인한다
37. `JunDeathmatchGameState`의 Ring 복제값이 갱신되는지 확인한다

## F. 멀티플레이 확인

38. PIE 인원 수를 2 이상으로 설정한다
39. `BP_JunRingHost`를 쓰는 경우 Class Defaults에서 `Replicates`가 켜져 있는지 확인한다
40. Listen Server + Client에서 Ring 반지름과 phase가 일관되게 보이는지 확인한다
41. 클라이언트가 Ring 밖에 있을 때 서버 기준으로 데미지가 들어가는지 확인한다
42. 데미지 결과가 클라이언트에서 임의로 계산되지 않는지 확인한다

## G. 현재 구현 기준

- `JunRingComponent`는 기본적으로 자동 시작된다
- DataTable이 없어도 기본 3개 phase로 동작한다
- DataTable이 있으면 `PhaseIndex` 순서로 정렬하고 검증한다
- 기존 CSV 필드 `WaitTime`, `ShrinkTime`, `DamageInterval`, `DamagePerTick`도 지원한다
- 새 CSV 필드 `DelayBeforeShrink`, `ShrinkDuration`, `DamageTickInterval`, `DamagePerSecond`도 지원한다
- `StopRing`, `PauseRing`, `ResumeRing`, `ResetRing`, `ResetForRound`, `AdvanceToPhase`, `ReloadRingData`를 Blueprint에서 호출할 수 있다
