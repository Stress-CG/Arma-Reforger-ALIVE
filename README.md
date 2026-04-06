# Arma Reforger ALIVE

This addon now contains a real framework scaffold for an ALIVE-style Reforger rewrite instead of just a placeholder repo.

The current implementation focuses on the first executable foundation:

- a valid addon project file in `addon.gproj`
- a locked V1 project policy: hybrid prefab strategy, GM/admin overlay only, local JSON persistence only, and Everon-only validation scope
- stable campaign data models for presets, objectives, profiles, orders, tasks, budgets, and persistence envelopes
- a bootstrap component you can attach to any world entity to start the framework on the server
- a replicated runtime summary component for lightweight client visibility
- local JSON persistence for presets and war saves under `$profile:/ReforgerAlive`
- preset and save catalogs plus faction normalization against the currently loaded faction set
- a hybrid world scanner that prefers author-placed objective seeds and falls back to map descriptor discovery
- a first-pass planner plus activation queue that virtualizes profiles and promotes them near player spawn sources
- a server-authoritative physical AI handoff layer with runtime-only profile/entity bindings, reclaim writeback, cooldowns, and configurable spawn catalogs
- a separate lightweight strategic overlay replication channel for JIP-safe UI/admin state
- campaign lifecycle controls in runtime code for load/start, save, pause, resume, rebuild, and end
- optional text status reports under `$profile:/ReforgerAlive/Reports`
- vanilla `US` and `USSR` force templates for initial profile seeding
- materialization request tracking that now reflects pending versus physically projected profiles
- reclaim-safe persistence snapshots that fold active physical projections back into a virtual-only save payload without persisting runtime entity state
- a lightweight Workbench setup plugin that can generate mission-header and server-config files

## Workbench usage

1. Open this addon in Workbench and compile scripts.
2. Add `ARAL_BootstrapComponent` to a world entity in your test scenario.
3. Optionally add one or more `ARAL_ObjectiveSeedComponent` overrides to specific map entities.
4. Add `AliveSpawnCatalogComponent` to the same bootstrap entity and populate the vanilla `US` / `USSR` template-to-prefab mappings you want the handoff layer to use.
5. Ensure the bootstrap entity also has `AliveStrategicOverlayComponent`, `ARAL_RuntimeStateComponent`, `RplComponent`, and the usual hierarchy support required by replicated entities.
6. Start the scenario on a dedicated server or hosted session.

If `Auto Start On Authority` is enabled on the bootstrap component, the framework will:

- build and normalize a preset from the component attributes
- optionally load an existing save with the same preset name
- scan or seed objectives
- create virtual force profiles for both factions
- save the preset and war state
- begin planner and activation ticks on the authority

Additional bootstrap controls now support:

- auto-loading an existing save before creating a fresh campaign
- starting paused for GM/admin staging
- writing a text status report every time the framework saves
- editor button helpers for start/load, save, pause, resume, and objective rebuild during testing
- exporting a setup snapshot report for quick inspection of factions, presets, and saves

The physical handoff layer will only spawn projections when:

- a profile has been promoted to `MATERIALIZED` by the activation queue
- the server can resolve a root prefab for that profile template from `AliveSpawnCatalogComponent`
- the profile is not inside spawn/reclaim cooldown and does not already have a reserved or active binding

If a resolved vanilla prefab does not include `AlivePhysicalAgentComponent`, the framework now keeps it alive as a raw projection fallback instead of treating that as a fatal error. That keeps the server-authoritative spawn/reclaim loop usable while wrapper prefabs and richer tactical bindings are still being authored. The root still has to be replicable with `RplComponent`, so multiplayer/JIP safety stays intact.

When a replicated projection is reclaimed, the runtime now prefers the replication-safe delete path via `RplComponent.DeleteRplEntity(...)` and only falls back to non-replicated entity deletion when no root replication component is present. That keeps the virtual/physical handoff aligned with dedicated-server authority rules.

The strategic overlay replication layer now provides a separate low-bandwidth channel for:

- connected player count
- objective/task digests
- high-priority profile digests
- handoff/debug digest strings
- per-profile admin overlay summaries
- reclaim blocker summaries for GM/admin debugging

## Included validation artifact

`Configs/ReforgerAlive_GM_Everon.conf` is a small dedicated-server harness that points at the official Game Master Everon mission. It is useful as a starting server config while you wire the framework into a world save or GM-authored scenario.

`Docs/WorkbenchValidationRunbook.md` is the current manual validation checklist for opening up the remaining engine-facing work in Workbench, dedicated-server testing, and prefab finalization.

## Workbench helper plugin

`ARAL_WorkbenchSetupPlugin` is available under the Workbench plugin menu as `Reforger ALIVE Setup Helper`. It provides a simple dialog for generating:

- a mission-header `.conf`
- a dedicated-server `.conf`

You still need to supply the correct world resource path, but it removes the repetitive hand-editing step while you iterate.

## Current scope

This pass implements the core scaffold, a functioning server-side simulation loop, and the first real physical AI handoff transaction layer. It still does not yet include:

- a polished startup widget/UI flow
- Workbench-validated vanilla prefab paths or a finished tactical AI controller hookup inside authored spawned hierarchies
- logistics, civilians, CQB, or full C2ISTAR
- a custom packaged validation scenario world

Those pieces now have concrete code seams to build on, rather than needing a fresh architecture pass.
