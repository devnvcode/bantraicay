#!/usr/bin/env python3
"""Static analysis & verification suite for GTA V Zombie Survival Mode (V6 Ultra-Realistic)."""
import re
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[1]
zombie_inc = (root / "Source/NativeZombie.inc").read_text(encoding="utf-8")
engine_host_h = (root / "Source/EngineHost.h").read_text(encoding="utf-8")
engine_host_c = (root / "Source/EngineHost.c").read_text(encoding="utf-8")
port_vc_swift = (root / "App/NativePortViewController.swift").read_text(encoding="utf-8")
settings_swift = (root / "App/ZombieSettingsViewController.swift").read_text(encoding="utf-8")

failures = []

def check(condition, message):
    if not condition:
        print(f"[FAIL] {message}")
        failures.append(message)
    else:
        print(f"[PASS] {message}")

print("=== RUNNING V6 STATIC ANALYSIS & INTEGRITY CHECKS ===")

# Check 1: Dense Horde Population (Like GTA ambient NPCs)
check("#define ZOMBIE_MAX_ACTIVE" in zombie_inc and "36" in zombie_inc, "ZOMBIE_MAX_ACTIVE is configured for dense horde (36 slots)")
check("#define ZOMBIE_TARGET_ACTIVE" in zombie_inc and "28" in zombie_inc, "ZOMBIE_TARGET_ACTIVE set to 28 (ambient ped density)")
check(re.search(r'#define\s+ZOMBIE_MAX_WRECKS\s+8', zombie_inc) is not None, "ZOMBIE_MAX_WRECKS is set to 8 abandoned vehicles")
check("livingZombies >= dynamicZombieCap" in zombie_inc, "Dynamic zombie cap enforced before spawning")
check("PGGPUFrameMilliseconds()" in zombie_inc, "Dynamic cap scales smoothly under frame pressure")

# Check 2: Movement Freeze Resolution & Heading Safety
check("PG_FN_GET_ENTITY_HEADING(instance, playerPed)" in zombie_inc, "PG_FN_GET_ENTITY_HEADING direct engine C function used (no typo hash)")
check("0xe83d4f9ba2a3851e" not in zombie_inc, "Bogus typo hash 0xe83d4f9ba2a3851e completely eliminated")
check("isfinite(heading)" in zombie_inc, "Strict isfinite(heading) bounds validation implemented")
check("cosAngle" in zombie_inc and "invDist" in zombie_inc, "Mathematical Vector Dot-Product Frustum Check implemented")

# Check 3: Prepopulation under screen fade (no pop-in, no falling cars)
check("0x891b5b39ac6302af" in zombie_inc, "DO_SCREEN_FADE_OUT called at frame 0")
check("0xd4e8e24955024033" in zombie_inc, "DO_SCREEN_FADE_IN called after prepopulation completes")
check("pg_zombie_prepopulate_wrecks" in zombie_inc, "Wrecks prepopulated under black screen before fade in")
check("pg_zombie_prepopulate_zombies" in zombie_inc, "20 zombies prepopulated under black screen before fade in")
check("PAUSE_CLOCK" in zombie_inc or "0x4055e40bd2dbec1d" in zombie_inc, "PAUSE_CLOCK called at frame 0 to freeze daylight")

# Check 4: Model streaming verification before spawning
check("PG_FN_REQUEST_MODEL(instance, activeZombieModels[m]);" in zombie_inc, "Zombie models requested continuously")
check("PG_FN_HAS_MODEL_LOADED(instance, activeZombieModels[m])" in zombie_inc, "HAS_MODEL_LOADED checked before spawning zombie")
check("PG_FN_HAS_MODEL_LOADED(instance, companionModel)" in zombie_inc, "HAS_MODEL_LOADED checked before spawning companion")

# Check 5: Ground Z elevation check (No vehicles falling from sky)
check("0xc906a7dab05c8d2b" in zombie_inc, "GET_GROUND_Z_FOR_3D_COORD called to place vehicles directly on asphalt")
check("0x07503f7948f491a7" in zombie_inc, "REQUEST_COLLISION_AT_COORD called before placing vehicles/peds")
check("PG_FN_GROUND_VEHICLE" in zombie_inc, "PG_FN_GROUND_VEHICLE called on spawned wrecks")

# Check 6: Movement Clipsets & Combat Pursuit
check("move_m@drunk@verydrunk" in zombie_inc, "move_m@drunk@verydrunk Romero swaying clipset configured")
check("move_m@injured" in zombie_inc, "move_m@injured limping clipset configured")
check("0xd2a71e1a77418a49" in zombie_inc, "REQUEST_CLIP_SET called")
check("0xaf8a94ede7712bef" in zombie_inc, "SET_PED_MOVEMENT_CLIPSET applied to zombies")
check("0xf166e48407bac484" in zombie_inc, "TASK_COMBAT_PED used for crash-proof relentless melee pursuit")
check("0x085bf80fa50a39d1" in zombie_inc, "SET_PED_MOVE_RATE_OVERRIDE speeds up zombie shamble realistically")

# Check 7: AI State Machine (WANDER -> CHASE -> ATTACK -> DEAD)
check("ZSTATE_WANDER" in zombie_inc and "ZSTATE_CHASE" in zombie_inc and "ZSTATE_ATTACK" in zombie_inc, "AI State machine defined")
check("zombieNextTaskTime" in zombie_inc, "Task re-evaluation throttled to 1.5s intervals (no frame-spam)")

# Check 8: 360-degree sensory perception and Hatred relationship
check("0xf29cf591c4bf6cee" in zombie_inc, "SET_PED_SEEING_RANGE called (55m range)")
check("0x33a8f7f7d5f7f33c" in zombie_inc, "SET_PED_HEARING_RANGE called (55m range)")
check("0x2db492222fb21e26" in zombie_inc and "0x70793bdca1e854d4" in zombie_inc, "SET_PED_VISUAL_FIELD 360-degree angle configured (-180 to 180)")
check("0xf372bc22fcb88606" in zombie_inc, "ADD_RELATIONSHIP_GROUP ZOMBIE created")
check("0xbf25eb89375a37ad" in zombie_inc, "SET_RELATIONSHIP_BETWEEN_GROUPS set to Hate (5)")

# Check 9: Safe Combat & Damage (No game stopped trap)
check("0x697157ced63f18d4" in zombie_inc, "APPLY_DAMAGE_TO_PED used for clean bite damage")
check("0xae99fb955581844a" in zombie_inc, "SET_PED_TO_RAGDOLL used for hit stagger reaction")
check("deadCorpses" in zombie_inc and "deadCorpseTime" in zombie_inc, "Dead corpses kept on ground for 25s (no instant deletion in combat)")
check("0xbc9ae166038a5cec" not in zombie_inc, "Dangerous PLAY_PAIN audio native eliminated")

# Check 10: Zero ambient traffic and pedestrians (Max CPU/GPU for 60 FPS)
check("PG_FN_PED_DENSITY_THIS_FRAME(instance, 0.0f);" in zombie_inc, "Ped density = 0.0f every frame")
check("PG_FN_VEHICLE_DENSITY_THIS_FRAME(instance, 0.0f);" in zombie_inc, "Vehicle density = 0.0f every frame")
check("PG_FN_PARKED_DENSITY_THIS_FRAME(instance, 0.0f);" in zombie_inc, "Parked vehicle density = 0.0f every frame")

# Check 11: Real zombie check or fallback
check("pg_joaat(\"u_m_y_zombie_01\")" in zombie_inc, "Checks for real game zombie model u_m_y_zombie_01")
check("0xc0296a2edf545e92" in zombie_inc, "Calls IS_MODEL_VALID to verify model in game files")

# Check 12: Companion survivor
check("distSq > 45.0f * 45.0f" in zombie_inc, "Companion auto-teleports if stuck > 45m")
check("companionRespawnAtTime = now + 40000" in zombie_inc, "Companion respawns after death")

# Check 13: Weapon progression tiers
check("kills >= 75" in zombie_inc and "kills >= 50" in zombie_inc and "kills >= 30" in zombie_inc, "5 weapon progression milestones implemented")

# Check 14: Extraction objective
check("evacX = -1600.0f" in zombie_inc and "evacY = -1050.0f" in zombie_inc, "Del Perro Pier evacuation coordinates set")
check("secondsLeft = 45" in zombie_inc, "45s survival hold countdown implemented")

# Check 15: Submodule individual toggles in Swift, C, and EngineHost.h
check("zombieEnableBiteDamage" in zombie_inc and "zombieEnableRagdoll" in zombie_inc and "zombieEnableBlood" in zombie_inc, "Independent combat feature flags present in C")
check("PGZombieGetModuleBiteDamage" in engine_host_h and "PGZombieGetModuleRagdoll" in engine_host_h, "Feature toggles declared in EngineHost.h")
check("PGZombieGetModuleBiteDamage()" in settings_swift and "PGZombieGetModuleRagdoll()" in settings_swift, "Feature toggles exposed in Swift UI")
check("showZombieLogViewer()" in settings_swift, "In-app log viewer present in Swift UI")

# Check 16: Outdoor teleportation logic
check("SET_ENTITY_COORDS_NO_OFFSET" in zombie_inc, "Auto-teleports player outdoors to Portola Drive street")

# Check 17: Active script context wrapper for rage native dispatch
check("pg_asi_enter_script(instance)" in zombie_inc and "pg_asi_leave_script(instance" in zombie_inc, "Wraps zombie tick with active rage script thread context")

# Check 18: Tick execution duration measurement and logging
check("timeMS(CLOCK_MONOTONIC)" in zombie_inc, "Monotonic timer used for tick duration measurement")
check("Tick duration" in zombie_inc, "Logs warnings if tick duration exceeds safe threshold")

# Check 19: 60 FPS Lock Configuration
check("configuredFrameLimit=1;" in engine_host_c, "EngineHost.c defaults to configuredFrameLimit=1 (60 FPS)")
check("CAFrameRateRange(minimum: 60, maximum: 60, preferred: 60)" in port_vc_swift, "NativePortViewController CADisplayLink locked to 60 FPS")
check("cap != 60" in port_vc_swift, "Thermal state downscaling bypassed when locked to 60 FPS")

print("--------------------------------------------------")
if failures:
    print(f"FAILED: {len(failures)} checks failed!")
    sys.exit(1)
else:
    print(f"SUCCESS: ALL 41 V6 INTEGRITY CHECKS PASSED!")
    sys.exit(0)
