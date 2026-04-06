# Physical AI Handoff Design

## Core split

- `ARAL_ForceProfile` remains the canonical strategic record.
- `ARAL_EProfileState` remains the activation intent driven by the virtual simulation and player relevance queue.
- `AliveEHandoffState` is the transactional lifecycle for physical projection creation and reclaim.
- `AlivePhysicalAgentComponent` is a disposable runtime projection wrapper, not the authoritative source of war-state.
- `AliveStrategicOverlayComponent` is the separate replication surface for JIP/admin strategic state.

## Concrete class design

```c
interface IAliveSpawnCatalog
{
	bool ResolveRootPrefab(string templateId, out ResourceName rootPrefab);
}

class AliveProfileManager : Managed
{
	AliveProfileRuntimeRecord EnsureRuntimeRecord(ARAL_ForceProfile profile, float now);
	AliveRelevanceContext BuildRelevance(ARAL_ForceProfile profile, ARAL_MissionPreset preset, AliveProfileRuntimeRecord runtimeRecord, array<vector> playerObservers, float now);
	bool TryReserveForSpawn(ARAL_ForceProfile profile, AliveProfileRuntimeRecord runtimeRecord, float now);
	AliveSpawnSpec BuildSpawnSpec(ARAL_ForceProfile profile, ARAL_Order activeOrder, AliveProfileRuntimeRecord runtimeRecord);
	AliveProfileBinding CreateBinding(string profileId, IEntity entity, int spawnGeneration, float now);
	void WriteBackSnapshot(ARAL_ForceProfile profile, ARAL_Order activeOrder, AlivePhysicalSnapshot snapshot);
}

class AliveHandoffManager : Managed
{
	void Tick(IEntity owner, ARAL_WarState warState, ARAL_MissionPreset preset, array<vector> playerObservers);
	bool HasPhysicalProjection(string profileId);
	int GetPhysicalProjectionCount();
	int GetPendingProjectionCount();
	ARAL_WarState BuildPersistenceSnapshot(ARAL_WarState warState);
}

class AliveStrategicOverlayComponent : ScriptComponent
{
	void ApplyOverlay(AliveStrategicOverlaySnapshot snapshot);
}

class AliveOrderBridge : Managed
{
	void ApplyInitialTacticalState(AliveSpawnSpec spawnSpec, AlivePhysicalAgentComponent physicalAgent, ARAL_ForceProfile profile, ARAL_Order activeOrder);
	void SyncActiveOrder(AlivePhysicalAgentComponent physicalAgent, ARAL_ForceProfile profile, ARAL_Order activeOrder);
	AliveOrderProgressSnapshot CaptureOrderProgress(AlivePhysicalAgentComponent physicalAgent, ARAL_Order activeOrder);
	void WriteBackOrderProgress(ARAL_Order activeOrder, AliveOrderProgressSnapshot snapshot);
}

class AlivePhysicalAgentComponent : ScriptComponent
{
	void BindToProfile(string profileId, string templateId, int spawnGeneration, int desiredCount);
	void SetOrderContext(string orderId, string objectiveId);
	void SetOrderProgress(float progressNormalized, vector anchorPosition);
	void SetTacticalState(AliveETacticalMode tacticalMode, float estimatedSpeedMps, bool vehicleMoving, bool playerOccupancyBlocked, bool hasEnemyTarget, bool firedRecently, bool tookDamageRecently);
	void ReportAliveCount(int aliveCount);
	void ReportVehicleState(bool operational, float healthNormalized, float fuelNormalized);
	void MarkObserved(float now);
	void MarkEngaged(float now);
	bool CanBeReclaimed(float now, float engagementGraceSeconds, float visibilityGraceSeconds);
	AlivePhysicalSnapshot CaptureSnapshot(IEntity owner);
}
```

## Runtime ownership model

- `ARAL_SimulationService` owns `AliveHandoffManager`.
- `AliveHandoffManager` owns `AliveProfileManager` and `AliveOrderBridge`.
- `AliveSpawnCatalogComponent` lives on the bootstrap entity and resolves template ids like `us_rifle` or `ussr_motor` to actual root prefabs.
- `AliveStrategicOverlayComponent` lives on the bootstrap entity and replicates compact objective/task/profile digests separately from spawned AI entities.
- `AliveProfileManager` stores runtime-only `AliveProfileRuntimeRecord` and `AliveProfileBinding` objects. These are not persisted.
- Virtual movement and virtual combat pause for profiles whose activation intent is currently `MATERIALIZED`, so the physical projection is not simulated twice.
- Save-time persistence snapshots capture active physical projections back into cloned virtual profiles so restart recovery never depends on temporary entity ids or live replicated roots.

## Handoff state machine

```text
Virtual
  -> PendingSpawn      reservation acquired
  -> Spawning          server spawn transaction started
  -> Physical          entity hierarchy spawned, bound, inserted into replication
  -> PendingReclaim    relevance lost and reclaim allowed
  -> Reclaiming        snapshot/writeback/delete in progress
  -> Virtual           reclaim completed successfully
  -> Destroyed         physical projection ended with zero surviving strength
```

Transition rules:

- `Virtual -> PendingSpawn`: profile activation intent is `MATERIALIZED`, within spawn relevance, below active limiter, no cooldown, no existing binding.
- `PendingSpawn -> Spawning`: spawn transaction starts on the authority.
- `Spawning -> Physical`: root entity spawned, `AlivePhysicalAgentComponent` found, `ProfileId <-> EntityId <-> RplId` binding created.
- `Spawning -> Physical`: if the root lacks `AlivePhysicalAgentComponent`, the handoff layer can continue in raw fallback mode with manager-owned binding state as long as the root still has `RplComponent`.
- `Spawning -> Virtual`: partial spawn failure, prefab resolution failure, or replication insertion failure path.
- `Physical -> PendingReclaim`: strategic activation intent flips back to `VIRTUAL`.
- `PendingReclaim -> Reclaiming`: reclaim guard passes hysteresis, visibility grace, engagement grace, and pending-reclaim lock.
- `Reclaiming -> Virtual`: snapshot written back to canonical profile/order state and entity hierarchy deleted cleanly.
- `Reclaiming -> Destroyed`: writeback shows zero surviving force strength.

## Spawn transaction pseudocode

```text
for each profile on server:
  runtime = EnsureRuntimeRecord(profile)
  relevance = BuildRelevance(profile, preset, playerObservers)

  if profile.intent != MATERIALIZED:
    continue

  if !TryReserveForSpawn(profile, runtime, now):
    continue

  spawnSpec = BuildSpawnSpec(profile, activeOrder, runtime)
  if !spawnCatalog.ResolveRootPrefab(profile.templateId, spawnSpec.rootPrefab):
    RollbackSpawnReservation(runtime)
    return

  MarkSpawnStarted(runtime)
  spawnedRoot = SpawnEntityPrefabEx(rootPrefab, world, transform)
  if !spawnedRoot:
    RollbackSpawnReservation(runtime)
    return

  physicalAgent = spawnedRoot.FindComponent(AlivePhysicalAgentComponent)
  if !physicalAgent:
    DeleteEntityAndChildren(spawnedRoot)
    RollbackSpawnReservation(runtime)
    return

  physicalAgent.BindToProfile(profile.id, profile.templateId, runtime.spawnGeneration, profile.strength)
  orderBridge.ApplyInitialTacticalState(spawnSpec, physicalAgent, profile, activeOrder)
  binding = CreateBinding(profile.id, spawnedRoot, runtime.spawnGeneration, now)
  CompleteSpawn(profile, runtime, binding, now)
```

## Reclaim transaction pseudocode

```text
if profile.intent == VIRTUAL and runtime.state == Physical:
  if not CanBeginReclaim(runtime, relevance, now):
    return
  if not physicalAgent.CanBeReclaimed(now, engagementGrace, visibilityGrace):
    return

  BeginReclaim(runtime)
  physicalAgent.PrepareForReclaim()
  MarkReclaiming(runtime)

  snapshot = physicalAgent.CaptureSnapshot(boundEntity)
  orderBridge.WriteBackOrderProgress(activeOrder, snapshot.orderProgress)
  profileManager.WriteBackSnapshot(profile, activeOrder, snapshot)

  RplComponent.DeleteRplEntity(boundEntity, false)
  RemoveBinding(profile.id)

  if profile.strength <= 0:
    MarkDestroyed(profile, runtime, now)
  else:
    CompleteReclaim(profile, runtime, now)
```

## Binding model

- Canonical key: `ProfileId`
- Runtime entity key: `EntityID`
- Replication key: `RplId`
- Binding record: `AliveProfileBinding`

The binding model is deliberately runtime-only:

- `ProfileId` belongs to `ARAL_ForceProfile` and persists.
- `EntityID` and `RplId` belong only to the temporary spawned projection.
- The `AlivePhysicalAgentComponent` replicates only binding identity needed by proxies:
  - `m_sProfileId`
  - `m_sTemplateId`
  - `m_iSpawnGeneration`
- Raw fallback bindings do not add additional component replication; they rely on the spawned entity's native replication plus the runtime binding map on the authority.
- Order state, task state, objective ownership, and planner state stay in the strategic model and should later be replicated through lightweight summary channels, not through the AI entity hierarchy.

## Prefab and replication hierarchy

Recommended spawned hierarchy:

- Root prefab:
  - `RplComponent`
  - `AlivePhysicalAgentComponent`
  - a thin local tactical controller or bridge component later
- Child hierarchy:
  - infantry members, vehicle crew, mounted vehicle, or temporary helper entities
  - no strategic components
  - no persistence-bearing state

Rules:

- Keep the root hierarchy disposable.
- Insert only the physical projection hierarchy into replication.
- Enable streaming and spatial relevancy on the projection root.
- Do not mirror full war-state onto the physical entity tree.
- Do not let spawned entities own authoritative strategic decisions.

## What should replicate

Replicate on physical projections:

- profile binding identity
- spawn generation
- the normal entity replication required for AI, transform, and combat state
- optional binding identity (`ProfileId`, `TemplateId`, `SpawnGeneration`) on authored wrapper prefabs through `AlivePhysicalAgentComponent`

Do not replicate on physical projections:

- planner order graph
- objective ownership graph
- full task list
- strategic faction force pools
- persistence envelopes

Replicate later through lightweight overlay channels:

- top-level campaign summary
- task summaries
- objective status
- optional profile summary overlays for GM/admin UI
- multiplayer/JIP handoff digests via `AliveStrategicOverlayComponent`

## Failure handling rules

- Reservation timeout expires back to `Virtual`.
- Missing prefab mapping rolls back the spawn reservation with cooldown.
- Spawned root without `AlivePhysicalAgentComponent` stays alive as a raw fallback projection when possible.
- Lost physical binding returns to `Virtual` with cooldown if the profile still has strength.
- Lost physical binding becomes `Destroyed` if no strength remains.
- Reclaim is blocked while recently visible or recently engaged.
- Reclaim is also blocked when the current tactical snapshot marks `player-in-vehicle`, `vehicle-moving`, `recent-damage`, `recent-fire`, or `enemy-target`.

## Debug instrumentation

Runtime counters already exposed:

- physical projection count
- raw fallback projection count
- pending spawn count
- pending reclaim count
- reclaiming count
- destroyed count
- per-profile tactical mode summary
- per-profile reclaim blocker summary

Recommended next debug additions:

- a GM/admin panel showing per-profile `AliveEHandoffState`
- a profile timeline log for `Virtual -> Physical -> Virtual`
- a reclaim reason string:
  - out of relevance
  - cooldown
  - recently visible
  - recently engaged
  - missing prefab
  - lost binding
- a spawn failure ring buffer keyed by `ProfileId`

## Test scenarios

1. Single infantry profile enters spawn radius, spawns once, and creates exactly one binding.
2. Two players approach the same profile from different directions; only one reservation and one spawn occur.
3. Player leaves relevance radius briefly; reclaim does not happen until hysteresis expires.
4. Profile is engaged shortly before leaving radius; reclaim remains blocked through engagement grace.
5. Spawn prefab is missing; reservation rolls back and retries only after cooldown.
6. Spawn succeeds but the root lacks `AlivePhysicalAgentComponent`; the binding falls back to raw projection mode and still reclaims cleanly.
7. Physical entity is deleted unexpectedly by runtime damage or script; binding is removed and profile recovers safely.
8. Reclaim writes back reduced alive count, order progress, and vehicle health/fuel.
9. Reclaim with zero surviving strength ends in `Destroyed` and does not re-spawn.
10. Join-in-progress client sees replicated projection entities correctly while strategic summary remains lightweight and separate.
