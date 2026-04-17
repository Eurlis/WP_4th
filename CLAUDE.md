# WP_4th — Apex Legends FFA 모작

## 프로젝트 개요
- UE5 C++ First Person 템플릿 기반
- Apex Legends 스타일 FFA(개인전) + AI 봇
- 플레이어 3명 + AI 봇 5~7마리
- Listen Server 방식 멀티플레이
- 모듈 이름: WP_4th

## ⚠️ 절대 규칙
1. **main 브랜치 절대 금지** — Branch/OJJ 에서만 작업
2. **기존 템플릿 파일 수정 금지** — Variant_Shooter, Variant_Horror, WP_4thCharacter, WP_4thGameMode 등 원본 파일 절대 건드리지 않음
3. **Build.cs만 예외** — 필요한 모듈 추가 시에만 수정 가능
4. **CLAUDE.md는 500줄 미만 유지**
5. **새 파일만 생성/수정** — 우리가 만든 Weapon/, Test/ 폴더 내 파일만 작업

## 프로젝트 구조 (우리가 만든 것만)
```
Source/WP_4th/
├── Weapon/                    ← 무기 시스템 (재준 담당)
│   ├── WeaponTypes.h          ── enum: EItemState, EFireMode, EAmmoType
│   ├── ItemBase.h/.cpp        ── 모든 아이템 최상위
│   ├── WeaponBase.h/.cpp      ── 총기 공통 (히트스캔, 반동, RPC)
│   ├── Weapon_AR.h/.cpp       ── 돌격소총 (풀오토, 데미지14, 탄창20)
│   ├── Weapon_Pistol.h/.cpp   ── 권총 (반자동, 데미지18, 탄창12)
│   ├── Weapon_Shotgun.h/.cpp  ── 산탄총 (펠릿8x11, 탄창6)
│   ├── ThrowableBase.h/.cpp   ── 투척 공통
│   ├── Grenade.h/.cpp         ── 수류탄 (퓨즈2초, 데미지80, 반경5m)
│   ├── ProjectileBase.h/.cpp  ── 투사체 베이스 (Object Pool용)
│   ├── BulletProjectile.h/.cpp── 총알 투사체
│   └── BulletPoolManager.h/.cpp ── Object Pool 매니저
├── Test/                      ← 무기 테스트용 (나중에 삭제)
│   ├── WeaponTestCharacter.h/.cpp
│   └── WeaponTestGameMode.h/.cpp
```

## 네트워크 아키텍처
- **서버 권한**: 히트 판정, 데미지 계산, 아이템 스폰/픽업, AI 봇, 링 데미지
- **Replicated**: HP/실드, CurrentAmmo, bIsReloading, bIsFiring, ItemState
- **Server RPC**: ServerFire, ServerStartReload, ServerThrow
- **Multicast RPC**: MulticastFireEffects, MulticastExplosionEffects
- **Client RPC**: ClientShowHitMarker

## 캐릭터↔웨폰 인터페이스 (합의 사항)
캐릭터베이스는 다른 팀원이 담당. 연동 시 필요한 함수:

### 웨폰 → 캐릭터 호출
- `ServerApplyDamage(float Damage, ACharacter* DamageInstigator, FHitResult HitResult)`
- `ClientShowHitMarker(bool bIsHeadshot)`
- `GetAimDirection() → FVector`

### 캐릭터 → 웨폰 호출
- `StartFire()` / `StopFire()`
- `StartReload()`
- `OnEquipped()` / `OnUnequipped()`
- `GetCurrentAmmo()` / `GetMaxAmmo()` / `CanFire()`

## 무기 스탯 요약
| 무기 | 데미지 | 연사 | 탄창 | 타입 |
|------|--------|------|------|------|
| AR | 14 | 0.1초 | 20 | Auto/Light |
| 권총 | 18 | 0.15초 | 12 | Semi/Light |
| 산탄총 | 11x8 | 1.0초 | 6 | Pump/Shotgun |
| 수류탄 | 80 | - | - | 범위5m/퓨즈2초 |

## 부위별 배율
- 헤드샷: x2.0
- 다리: x0.75
- 몸통: x1.0

## Claude Code 플러그인 활용
- **OMC 키워드**: ulw(UE5), plan(계획), team(팀), ralph
- **Codex**: /codex:review(코드리뷰), /codex:rescue(에러복구)

## 작업 진행 상황
- [x] 웨폰 베이스 시스템 (AR/권총/산탄총/수류탄)
- [x] 테스트 캐릭터 (WeaponTestCharacter)
- [x] 사격/재장전/무기교체 동작 확인
- [x] 투사체 시스템 + Object Pool (ProjectileBase/BulletProjectile/BulletPoolManager)
- [ ] 수류탄 던지기 테스트
- [ ] 투사체 풀 PIE 테스트 (탄속/탄낙차 확인)
- [ ] 멀티플레이 PIE 테스트
- [ ] 캐릭터베이스 합치기
- [ ] AI 봇
- [ ] 게임모드 (데스매치)
