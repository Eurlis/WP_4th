# Ring Component Plan

Terminology note: this file keeps the historical `zone_component_plan.md` filename for harness compatibility, but the implementation and user-facing documentation term is `Ring`.

## Task Type

Research-first, then implementation in narrow milestones.

## Goal

Design and implement a reusable Ring system for an Unreal Engine round-based team elimination mode.

The system should be modular enough that a gameplay programmer can integrate it into different GameModes with minimal changes, while preserving correct multiplayer authority and replication behavior.

## Context

We are building a round-based team elimination mode.

The Ring system should support:

- Phase-based shrinking
- Configurable timing and damage
- DataTable-driven tuning
- Multiplayer-safe authority flow
- Blueprint hooks for UI and audiovisual feedback
- Round reset / start / stop / pause integration

## Explicit Non-Assumptions

Do not assume:

- Unreal Engine version
- Player count
- Dedicated server vs listen server only
- Existing GameMode/GameState class names
- Existing DataTable row structs
- Existing UI architecture
- CSV authoring tool
- Current test framework coverage

Inspect the repository and write down what is actually present.

## Repository Facts Confirmed So Far

- `WP_4th.uproject` exists and records `EngineAssociation` as `5.7`.
- The runtime module is named `WP_4th`.
- Existing enabled plugins include `StateTree`, `GameplayStateTree`, `MotionWarping`, and editor-only `ModelingToolsEditorMode`.
- Existing weapon tuning uses a `FTableRowBase` DataTable pattern in `Source/WP_4th/Weapon/WeaponData.h`.
- Existing Jun work currently has `JunRingComponent`, `JunRingActor`, `JunDeathmatchGameMode`, `JunDeathmatchGameState`, and related player/death helper classes under `Source/WP_4th/JunGame`.
- Current Jun Ring work is directionally reusable, but it does not yet satisfy the full Ring requirements: target center, target radius, pause state, phase start time, elapsed time, strict validation, and GameMode-independent snapshot ownership are incomplete.

## Source Priority For Research Writeup

Prioritize in this order:

1. Unreal official documentation:
   - Components
   - DataTable / data-driven gameplay
   - Replication / actor component replication
   - GameMode / GameState / multiplayer framework
2. Epic Developer Community Forums / AnswerHub-style historical guidance
3. Relevant open-source C++ implementations when useful
4. Korean-language materials if high quality and technically aligned

## Mandatory Deliverables Before Broad Implementation

1. `docs/reports/zone_component_research.md`
   - Korean
   - Starts with an Executive Summary
   - Analytical / practical report style
   - Clearly separates confirmed facts, repo findings, assumptions, and recommendations
   - Includes citations when external browsing/tools are available

2. `docs/reports/zone_component_api.md`
   - Compact implementation summary
   - API tables
   - CSV schema
   - Replication snapshot fields
   - Integration checklist

3. Mermaid diagrams in the report:
   - State flow
   - Phase timeline
   - Shrink progression flow

## Architectural Decision To Evaluate

### Option A

Authority component attached to GameMode or a GameModeBase subclass, with replicated runtime state mirrored into GameState.

### Option B

Authority orchestration in GameMode, runtime state owned by a dedicated replicated Ring actor, and UI reads that actor state.

### Option C

An alternative repo-specific pattern if the existing architecture strongly suggests one.

## Current Recommendation

Prefer the existing `AJunRingActor + UJunRingComponent` path first, then add GameState mirroring where needed.

Reasoning:

- Unreal GameMode exists only on the server, so it is a poor direct UI data source.
- A dedicated replicated Ring actor can be reused by Deathmatch, team elimination, and future modes without requiring each GameState class to duplicate ring fields.
- The GameMode can remain the authority/orchestrator: start round, spawn/register Ring actor, call start/pause/reset.
- The Ring actor can own the replicated snapshot: clients and UI read data there.
- Core ring logic can live in `UJunRingComponent` to preserve drop-in reuse.

## Expected API Surface

### Candidate Public C++ API

- `InitFromDataTable(UDataTable* InTable, FName InRowName)`
- `InitFromPhaseRows(const TArray<FJunRingPhaseRow>& InRows)`
- `StartRing()`
- `StopRing()`
- `PauseRing()`
- `ResumeRing()`
- `ResetRing()`
- `ResetForRound()`
- `AdvanceToPhase(int32 PhaseIndex)`
- `GetCurrentRuntimeState() const`
- `IsRingActive() const`
- `IsShrinking() const`
- `GetCurrentRadius() const`
- `GetCurrentCenter() const`

### Candidate Server-Only Hooks

- `ServerStartRingInternal()`
- `ServerApplyOutsideDamage()`
- `ServerRebuildPhaseCache()`
- `ServerCommitPhaseTransition()`

### Candidate Blueprint Hooks

- `BP_OnRingInitialized`
- `BP_OnRingStarted`
- `BP_OnRingPaused`
- `BP_OnRingStopped`
- `BP_OnRingPhaseChanged`
- `BP_OnRingCenterChanged`
- `BP_OnRingRadiusChanged`
- `BP_OnRingCompleted`
- `BP_OnRingDataValidationFailed`

Use `BlueprintImplementableEvent`, `BlueprintAssignable`, and `BlueprintCallable` pragmatically.

## Required Data Design

### `FJunRingPhaseRow : FTableRowBase`

Fields to evaluate:

- `Phase`
- `Radius`
- `TargetRadius`
- `ShrinkDuration`
- `DelayBeforeShrink`
- `DamagePerSecond`
- `CenterMode`
- `FixedCenterX`
- `FixedCenterY`
- `FixedCenterZ`
- `RandomOffsetRadius`
- `WarningLeadTime`
- `UseStepDamageInterval`
- `DamageTickInterval`
- `NavAffectPolicy`
- `Notes`

### `FJunRingRuntimeState`

Designed for minimal replication and client interpolation:

- `CurrentPhase`
- `ElapsedInPhase`
- `PhaseState`
- `CurrentCenter`
- `TargetCenter`
- `CurrentRadius`
- `TargetRadius`
- `bIsActive`
- `bIsPaused`
- `bIsShrinking`
- `ServerWorldTimeAtPhaseStart`

### Supporting Enums

- `EJunRingPhaseState`
- `EJunRingCenterMode`
- `EJunRingShrinkModel`
- `EJunRingDamageModel`

## Replication Rules To Enforce

- Authority must live on the server.
- Clients must never author ring progression.
- Replicate data, not heavy behavior.
- Prefer replicated snapshot plus client-side interpolation for visuals.
- Keep damage authoritative.
- Explain whether the component itself replicates, or whether a host actor / GameState mirrors state.

## DataTable / CSV Work Requirements

Investigate and document:

- Editor CSV import path
- Editor reimport path
- Row validation path
- Runtime initialization from DataTable asset
- Whether runtime reload is supported directly in the current engine version / repo setup
- If not, propose safe alternatives:
  - Editor-only reimport
  - Reload from asset reference
  - Custom CSV string/file parsing path
  - Restart-required policy

## Implementation Milestones

### Milestone 1: Repository Audit And Architecture Proposal

Inspect:

- UE version
- Gameplay module names
- Existing GameMode / GameState / PlayerState classes
- Existing multiplayer patterns
- Existing DataTable usage
- Build/test commands

Output:

- Architecture recommendation
- File touch list
- Risk list

Acceptance:

- No code changes required yet unless needed for minimal scaffolding docs

### Milestone 2: Data Model And API Skeleton

Implement:

- Row structs
- Runtime state struct
- Enums
- Core C++ class declarations
- Initial headers with comments

Acceptance:

- Clean compile for added declarations
- No broad gameplay behavior changes yet

### Milestone 3: Authority Flow And Replication Path

Implement:

- Authority host integration
- GameState or Ring actor snapshot path
- Replication declarations / `OnRep` as justified
- Start/Stop/Pause/Reset path

Acceptance:

- Clients can observe replicated ring state changes

### Milestone 4: DataTable Load / Validation / Reload Path

Implement:

- `InitFromDataTable`
- Row validation
- Readable logs
- Editor/runtime behavior notes

Acceptance:

- Invalid data fails clearly
- Valid table initializes deterministic phase cache

### Milestone 5: Phase Progression And Shrink Model

Implement:

- Phase timer logic
- Linear / exponential / step evaluation if justified
- Center update policy
- Round reset path

Acceptance:

- Predictable phase transitions
- Reproducible behavior from DataTable values

### Milestone 6: Damage + Blueprint Hooks + Debug Tools

Implement:

- Outside-ring damage policy
- BP hooks
- Draw debug helpers
- Console/debug commands if appropriate

Acceptance:

- Designers can observe and tune the system quickly

### Milestone 7: Tests And Final Docs

Implement:

- Unit/integration tests where feasible
- Multiplayer test checklist
- Final report updates

Acceptance:

- Final docs match the code

## Done When

This task is done only when:

- The design/report files exist and are coherent
- The chosen architecture is justified against Unreal multiplayer behavior
- The API surface is documented
- CSV schema and sample CSV are present
- DataTable load/validation path exists and is explained
- Replication path is implemented or clearly scaffolded
- Basic tests or validation scripts are present
- Documentation explains unknowns and engine-version-dependent behavior

## Stop-And-Fix Rule

If validation fails, stop scope expansion and repair before moving forward.
Do not continue to later milestones while earlier milestone validation is broken.

## Reporting Format Per Milestone

Always report:

- Inspected files
- Modified files
- Exact rationale
- Replication impact
- Authority flow impact
- Validation results
- Remaining unknowns
