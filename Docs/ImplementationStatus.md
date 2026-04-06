# Reforger ALIVE Implementation Status

## Implemented now

- Project scaffold and addon metadata.
- V1 policy lock-in for hybrid prefabs, GM/admin overlay only, local JSON persistence only, and Everon-only validation scope.
- Core Enforce types for `WarState`, `MissionPreset`, `Objective`, `ForceProfile`, `Order`, `TaskRecord`, `FactionChoice`, `ActivationBudget`, and `PersistenceEnvelope`.
- Local persistence backend with JSON save/load using `SCR_JsonSaveContext` and `SCR_JsonLoadContext`.
- `ARAL_BootstrapComponent` for world-level startup and parameter-driven preset generation.
- Runtime lifecycle controls for start/load, save, pause, resume, objective rebuild, and end-state transitions.
- `ARAL_RuntimeStateComponent` with `RplProp` summary fields for low-cost replicated session state.
- Setup snapshots with faction discovery, preset/save catalogs, and preset normalization against available factions.
- Setup snapshot report export for fast operator visibility into available factions and persisted presets/saves.
- `ARAL_DefaultMapScanner` with hybrid objective discovery:
  - editor-authored `ARAL_ObjectiveSeedComponent`
  - automatic `MapDescriptorComponent` fallback
  - bootstrap fallback objective if no candidates are found
- Objective deduplication so nearby auto-scan descriptors do not flood the planner.
- `ARAL_PlannerService` for minimal objective-driven orders, virtual movement, virtual ownership changes, task generation, and vanilla US/USSR template-based force seeding.
- `ARAL_ActivationService` for player-distance activation/despawn queueing with active limiter, smooth dispatch intervals, and spawn-source dedupe.
- Physical AI handoff architecture with:
  - `AliveProfileManager` runtime bindings and reservation/cooldown state
  - `AliveHandoffManager` server-side spawn/reclaim transactions
  - `AliveOrderBridge` for strategic-to-tactical order initialization and writeback
  - `AlivePhysicalAgentComponent` as the disposable runtime projection wrapper
  - `AliveSpawnCatalogComponent` for template-to-prefab resolution without coupling strategic code to prefab paths
- `AliveVanillaPrefabDiscoveryService` for auto-discovery from loaded faction entity catalogs when explicit template mappings are missing.
- Raw projection fallback so vanilla prefabs can still be spawned/reclaimed safely before wrapper prefabs exist.
- `AliveTacticalControllerService` for server-side order progress updates, tactical mode resolution, reclaim blocker inference, and engagement detection while a projection is physically active.
- `AliveStrategicOverlayComponent` plus `AliveStrategicOverlayService` for separate lightweight replication of strategic overlays and handoff digests in multiplayer/JIP scenarios.
- Replication-safe reclaim cleanup through `RplComponent.DeleteRplEntity(...)` when a projection root is replicated.
- Save-time persistence sanitization that captures active projection state into a virtual-only snapshot so JSON saves never depend on temporary runtime entities.
- Text status report generation under `$profile:/ReforgerAlive/Reports`.
- Admin/debug report expansion with policy summary, profile overlay digest, and reclaim blocker digest.
- Materialization request tracking so active virtual profiles now produce pending or ready records based on real handoff state.
- A Workbench setup helper plugin that can generate mission-header and server-config files from a simple dialog.

## Intentionally deferred

- Runtime widget/dialog UI for admin setup and preset browsing.
- Direct integration with Game Master menus or custom context actions.
- Workbench-validated vanilla prefab resource paths and a finished tactical AI controller hookup inside authored spawned hierarchies.
- Database-backed persistence adapter implementation.
- Secondary systems: logistics, CQB, civilians, intel overlays.
- Author-packaged validation mission content and world assets.

## Recommended next implementation pass

1. Validate and lock down the `AliveSpawnCatalogComponent` auto-discovery results against real vanilla unit prefabs in Workbench, then replace heuristic mappings with curated defaults where needed.
2. Add a tactical controller component under the spawned root that converts `AliveOrderBridge` inputs into waypoint/combat behavior for non-fallback projections.
3. Add an admin-facing dialog or GM action layer on top of `ARAL_Runtime`, `AliveStrategicOverlayComponent`, and `ARAL_SetupService`.
4. Build the Everon validation scene with a bootstrap entity, spawn catalog, overlay component, and seeded objectives.
5. Validate dedicated-server plus multi-client replication, then freeze the 10 template mappings to known-good prefabs.
