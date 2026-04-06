# Workbench Validation Runbook

This is the shortest path to opening the remaining engine-facing work so the rest of V1 can be finished quickly.

## 1. Open the addon in Workbench

1. Launch `D:\SteamLibrary\steamapps\common\Arma Reforger Tools\Workbench\ArmaReforgerWorkbenchSteamDiag.exe`.
2. Open the project at `C:\Users\njsmi\Documents\My Games\ArmaReforgerWorkbench\addons\Reforger ALIVE\Arma Reforger ALIVE\addon.gproj`.
3. Let the project finish loading before doing any validation.

## 2. Compile scripts

1. Run the normal Workbench script compile/build step for the addon.
2. If compilation fails, save the full error list.
3. Stop here and return the error text if there are any compile failures.

What to bring back:

- the exact compile errors
- the file paths and line numbers reported by Workbench

## 3. Create the Everon validation scene

1. Create or open an Everon-based test world/sub-scene.
2. Place one bootstrap world entity that will own the ALIVE runtime.
3. Ensure that entity has:
   - `RplComponent`
   - `ARAL_BootstrapComponent`
   - `ARAL_RuntimeStateComponent`
   - `AliveSpawnCatalogComponent`
   - `AliveStrategicOverlayComponent`
4. Save the scene.

Recommended bootstrap defaults:

- `Auto Start On Authority = true`
- `Auto Load Save Before Start = true`
- `Write Status Reports = true`
- `Admin Overlay Enabled = true`
- `Player Faction = US`
- `Enemy Faction = USSR`
- `Use Author Overrides = true`
- `Auto Scan Map = true`

What to bring back:

- the saved world or sub-scene resource path
- confirmation that the bootstrap entity contains all five required components

## 4. Create at least one wrapper prefab

Start with infantry first.

1. Create a wrapper prefab for one infantry formation root.
2. On the root entity, add:
   - `RplComponent`
   - `AlivePhysicalAgentComponent`
3. Keep the hierarchy simple and disposable.
4. Avoid duplicate `RplComponent` instances inside the same replicated root hierarchy.
5. Save the prefab.

What to bring back:

- the wrapper prefab resource path
- confirmation that the root has exactly one authoritative replication root

## 5. Populate the spawn catalog

On `AliveSpawnCatalogComponent`, assign validated prefabs for these template ids:

- `us_rifle`
- `us_weapons`
- `us_recon`
- `us_motor`
- `us_armor`
- `ussr_rifle`
- `ussr_weapons`
- `ussr_recon`
- `ussr_motor`
- `ussr_armor`

Use the wrapper prefabs for the core formations you want full control over. Use raw vanilla prefabs only where wrapper coverage is not ready yet.

What to bring back:

- the exact prefab resource path chosen for each of the 10 template ids
- which ones are wrapper prefabs versus raw vanilla fallbacks

## 6. Run the local authority test

1. Start the scene as a local hosted session or local dedicated server plus client.
2. Confirm the bootstrap entity initializes.
3. Confirm the campaign auto-starts or starts via the editor buttons.
4. Confirm log output appears for:
   - campaign start/load
   - spawn bindings
   - reclaim bindings
5. Confirm the bootstrap entity exists on both server and client.

Expected outputs:

- status reports under `$profile:/ReforgerAlive/Reports`
- saves under `$profile:/ReforgerAlive/Saves`

What to bring back:

- server log path
- client log path
- one example status report file
- any replication warnings or errors

## 7. Validate handoff behavior

1. Move a player near projected profile areas and confirm spawn happens once.
2. Move away and confirm reclaim happens only after hysteresis/cooldown.
3. Confirm no despawn occurs while units are recently engaged or player-visible.
4. Confirm profile-to-entity and entity-to-profile logs appear.
5. Confirm admin overlay digests update as profiles spawn and reclaim.

What to bring back:

- whether spawn worked for infantry
- whether reclaim worked
- whether raw fallback paths were used
- any profiles that double-spawned, ghosted, or failed to reclaim

## 8. Validate persistence

1. Let the mission save at least once, or manually trigger `Save Campaign`.
2. Restart the session/server.
3. Load the same preset again.
4. Confirm war-state restores from JSON.
5. Confirm no physical entities were serialized directly.

What to bring back:

- whether save/load succeeded
- one example JSON save file path
- any mismatch between pre-restart and post-restart war-state

## 9. Validate multiplayer

1. Run a dedicated server.
2. Connect one client and validate projected AI replication.
3. Connect a second client if available.
4. Check for:
   - ghost entities
   - duplicate spawns
   - reclaim desync
   - replication errors

What to bring back:

- dedicated server log path
- client log path(s)
- whether the same projection appeared correctly for all clients

## 10. Return these artifacts so the remaining pass can be finished

Please send back as many of these as you have:

- Workbench compile errors, if any
- world/sub-scene resource path
- wrapper prefab resource path(s)
- the 10 final template-to-prefab mappings
- server and client log paths
- any replication warnings/errors
- one status report sample
- one JSON save sample
- notes on what worked versus what failed

Once those are available, the remaining pass is mainly:

- fixing compile/runtime issues surfaced by Workbench
- replacing any bad prefab mappings
- tightening the tactical behavior hookup on real spawned wrappers
- validating the Everon scenario and dedicated multiplayer loop
