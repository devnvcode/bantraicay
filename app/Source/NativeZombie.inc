// ============================================================================
// NATIVE ZOMBIE APOCALYPSE SURVIVAL SYSTEM
// Mugoat NewGen / PlayGTA5Native for iOS
// ============================================================================
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdatomic.h>
#include <stdlib.h>

#define ZOMBIE_MAX_ACTIVE 14
#define ZOMBIE_MAX_WRECKS 8

// Native declarations from EngineHost / engine-functions.h
void PG_FN_SET_WEATHER(w2c_game*, u64);
void PG_FN_CLEAR_WEATHER_OVERRIDE(w2c_game*);
void PG_FN_SET_CLOCK(w2c_game*, u32, u32, u32);
void PG_FN_PED_DENSITY_THIS_FRAME(w2c_game*, f32);
void PG_FN_VEHICLE_DENSITY_THIS_FRAME(w2c_game*, f32);
void PG_FN_PARKED_DENSITY_THIS_FRAME(w2c_game*, f32);
u32 PG_FN_PLAYER_PED_ID(w2c_game*);
u32 PG_FN_PLAYER_ID(w2c_game*);
void PG_FN_CLEAR_WANTED(w2c_game*, u32);
u32 PG_FN_GET_WANTED(w2c_game*, u32);
void PG_FN_SET_MAX_WANTED(w2c_game*, u32);
void PG_FN_GET_ENTITY_COORDS(w2c_game*, u64, u32, u32);
f32 PG_FN_GET_ENTITY_HEADING(w2c_game*, u32);
void PG_FN_REQUEST_MODEL(w2c_game*, u32);
u32 PG_FN_HAS_MODEL_LOADED(w2c_game*, u32);
void PG_FN_RELEASE_MODEL(w2c_game*, u32);
u32 PG_FN_CREATE_VEHICLE(w2c_game*, u32, u64, f32, u32, u32, u32);
void PG_FN_VEHICLE_ENGINE_HEALTH(w2c_game*, u32, f32);
void PG_FN_VEHICLE_DIRT(w2c_game*, u32, f32);
void PG_FN_GROUND_VEHICLE(w2c_game*, u32, f32);
void PG_FN_GIVE_WEAPON(w2c_game*, u32, u32, u32, u32, u32);
u32 PG_FN_ENTITY_EXISTS(w2c_game*, u32);
void PG_FN_SET_ENTITY_HEALTH(w2c_game*, u32, u32, u32);
u32 PG_FN_GET_ENTITY_MODEL(w2c_game*, u32);
void PG_FN_SET_ENTITY_COORDS_NO_OFFSET(w2c_game*, u32, u64, u32, u32, u32);

// Models for ambient zombies
static const uint32_t zombiePedModels[] = {
  0x19DFA98E, // a_m_y_skater_01
  0x3C2A8647, // a_m_m_bevhills_01
  0x4990C768, // a_m_y_beach_01
  0x20557434, // g_m_y_ballasout_01
  0x401BC182  // s_m_y_dealer_01
};

// Models for abandoned roadside wrecks
static const char *const zombieWreckModels[] = {
  "rebel", "stanier", "voodoo2", "ingot", "emperor2", "tornado3", "rancherxl", "glendale"
};

// Survivor companion model (Tough Marine / Security Guard)
static const uint32_t companionModel = 0x8D8F1B10; // s_m_y_marine_01

// Global state
static _Atomic bool zombieActive = true;
static _Atomic int zombieKills = 0;
static _Atomic int zombieTier = 0;
static _Atomic bool zombieResetRequested = false;

// Internal runtime tracking
static u32 activeZombies[ZOMBIE_MAX_ACTIVE];
static u32 zombieBlips[ZOMBIE_MAX_ACTIVE];
static u32 activeWrecks[ZOMBIE_MAX_WRECKS];
static u32 companionPed = 0;
static u32 companionBlip = 0;

static double lastZombieSpawn = 0;
static double lastWreckSpawn = 0;
static double lastCompanionCheck = 0;
static double companionRespawnAt = 0;
static double lastBiteDamage = 0;
static double lastGroanSound = 0;
static double lastWorldSync = 0;

static uint64_t zombieScratch = 0;
static uint64_t zombieStrWeather = 0;
static uint64_t zombieStrTimecycle = 0;
static uint64_t zombieVectorBuf = 0;

static char zombieStatusText[160] = "ZOMBIE SURVIVAL: SAN SANG";

// --- Bitwise Float Helpers for ASI Native Calls ---
static inline uint64_t pg_zarg_f32(float f) {
  uint32_t bits = 0;
  memcpy(&bits, &f, 4);
  return (uint64_t)bits;
}
static inline float pg_zret_f32(uint64_t v) {
  float f = 0.0f;
  uint32_t bits = (uint32_t)v;
  memcpy(&f, &bits, 4);
  return f;
}

static uint64_t pg_zombie_call(w2c_game *instance, uint64_t hash, const uint64_t *args, uint32_t count) {
  if (!pg_asi_game) pg_asi_game = instance;
  static uint64_t buf;
  if (!buf) buf = PG_FN_OPERATOR_NEW(instance, 8 * 40);
  if (!buf) return 0;
  if (count > 32) count = 32;
  if (count) memcpy(pg_pointer(&env, buf, 8ull * count), args, 8ull * count);
  memset(pg_pointer(&env, buf + 8 * 32, 64), 0, 64);
  if (!pg_asi_native(hash, buf, count, buf + 8 * 32)) return 0;
  uint64_t val = 0;
  memcpy(&val, pg_pointer(&env, buf + 8 * 32, 8), 8);
  return val;
}

// 1. Atmosphere: Daytime, Bright Daylight, Desolate Eerie Fog, Grayish Tint
static void pg_zombie_apply_atmosphere(w2c_game *instance) {
  // Broad daylight at 11:00 AM
  PG_FN_SET_CLOCK(instance, 11, 0, 0);

  // Weather: Foggy, overcast haze
  if (!zombieStrWeather) {
    zombieStrWeather = PG_FN_OPERATOR_NEW(instance, 32);
    if (zombieStrWeather) snprintf((char*)pg_pointer(&env, zombieStrWeather, 32), 32, "FOGGY");
  }
  if (zombieStrWeather) {
    PG_FN_CLEAR_WEATHER_OVERRIDE(instance);
    PG_FN_SET_WEATHER(instance, zombieStrWeather);
  }

  // Timecycle: Desaturated apocalyptic cold filter (0x2c933abf17a1df41: SET_TIMECYCLE_MODIFIER)
  if (!zombieStrTimecycle) {
    zombieStrTimecycle = PG_FN_OPERATOR_NEW(instance, 48);
    if (zombieStrTimecycle) snprintf((char*)pg_pointer(&env, zombieStrTimecycle, 48), 48, "prologue_shootout");
  }
  if (zombieStrTimecycle) {
    uint64_t tcArgs[1] = { zombieStrTimecycle };
    pg_zombie_call(instance, 0x2c933abf17a1df41ull, tcArgs, 1);
    // Strength 0.85f (0x82e7ffcd5b2326b3: SET_TIMECYCLE_MODIFIER_STRENGTH)
    uint64_t strArgs[1] = { pg_zarg_f32(0.85f) };
    pg_zombie_call(instance, 0x82e7ffcd5b2326b3ull, strArgs, 1);
  }
}

// 2. Clear Normal Peds, Traffic and Cops
static void pg_zombie_suppress_ambient(w2c_game *instance) {
  // Density multipliers to 0 each frame
  PG_FN_PED_DENSITY_THIS_FRAME(instance, 0.0f);
  PG_FN_VEHICLE_DENSITY_THIS_FRAME(instance, 0.0f);
  PG_FN_PARKED_DENSITY_THIS_FRAME(instance, 0.0f);

  uint64_t zeroArg[1] = { 0 };
  pg_zombie_call(instance, 0xb3b3359379fe77d3ull, zeroArg, 1); // SET_RANDOM_VEHICLE_DENSITY_MULTIPLIER_THIS_FRAME
  pg_zombie_call(instance, 0x90b6da738a9a25daull, zeroArg, 1); // SET_AMBIENT_VEHICLE_RANGE_MULTIPLIER_THIS_FRAME

  // Police suppression
  u32 player = PG_FN_PLAYER_ID(instance);
  if (player < 32) {
    PG_FN_SET_MAX_WANTED(instance, 0);
    if (PG_FN_GET_WANTED(instance, player) > 0) PG_FN_CLEAR_WANTED(instance, player);
    uint64_t pArgs[2] = { player, 1 };
    pg_zombie_call(instance, 0x32c62aa929c2da6aull, pArgs, 2); // SET_POLICE_IGNORE_PLAYER
    uint64_t dArgs[2] = { player, 0 };
    pg_zombie_call(instance, 0xdb172424876553f4ull, dArgs, 2); // SET_DISPATCH_COPS_FOR_PLAYER
  }
}

// 3. Abandoned Wrecked Vehicles Spawning & Cleanup
static void pg_zombie_manage_wrecks(w2c_game *instance, float px, float py, float pz, float heading, double now) {
  // Clean up distant wrecks (> 85m)
  for (int i = 0; i < ZOMBIE_MAX_WRECKS; i++) {
    u32 veh = activeWrecks[i];
    if (veh && PG_FN_ENTITY_EXISTS(instance, veh)) {
      PG_FN_GET_ENTITY_COORDS(instance, zombieVectorBuf, veh, 1);
      float *c = (float*)pg_pointer(&env, zombieVectorBuf, 24);
      float dx = px - c[0], dy = py - c[1];
      if (dx * dx + dy * dy > 85.0f * 85.0f) {
        // Mark no longer needed so engine reclaims it
        uint64_t rArgs[1] = { veh };
        pg_zombie_call(instance, 0xb736a491e64a32cfull, rArgs, 1); // SET_ENTITY_AS_NO_LONGER_NEEDED
        activeWrecks[i] = 0;
      }
    } else {
      activeWrecks[i] = 0;
    }
  }

  // Spawn new wreck if slot available
  if (now - lastWreckSpawn < 3500) return;
  lastWreckSpawn = now;

  int freeSlot = -1;
  for (int i = 0; i < ZOMBIE_MAX_WRECKS; i++) {
    if (!activeWrecks[i]) { freeSlot = i; break; }
  }
  if (freeSlot < 0) return;

  // Pick model
  static unsigned wreckIndex = 0;
  const char *modelName = zombieWreckModels[wreckIndex % 8];
  wreckIndex++;
  uint32_t modelHash = pg_joaat(modelName);

  PG_FN_REQUEST_MODEL(instance, modelHash);
  if (!PG_FN_HAS_MODEL_LOADED(instance, modelHash)) return;

  // Calculate roadside position (20m - 40m ahead or slightly to the side)
  float dist = 22.0f + (float)(rand() % 18);
  float angOffset = (float)((rand() % 90) - 45) * 0.0174533f;
  float rad = heading * 0.0174533f + angOffset;

  float *v = (float*)pg_pointer(&env, zombieVectorBuf, 24);
  v[0] = px - sinf(rad) * dist;
  v[1] = 0;
  v[2] = py + cosf(rad) * dist;
  v[3] = 0;
  v[4] = pz + 0.5f;
  v[5] = 0;

  float wreckHeading = heading + (float)(rand() % 180);
  u32 veh = PG_FN_CREATE_VEHICLE(instance, modelHash, zombieVectorBuf, wreckHeading, 0, 0, 1);
  if (veh && PG_FN_ENTITY_EXISTS(instance, veh)) {
    PG_FN_GROUND_VEHICLE(instance, veh, 5.0f);
    PG_FN_VEHICLE_DIRT(instance, veh, 15.0f); // Maximum rust and grime
    PG_FN_VEHICLE_ENGINE_HEALTH(instance, veh, -4000.0f); // Engine dead

    // 0x8aba6af54b942b95: SET_VEHICLE_UNDRIVEABLE
    uint64_t undriveArgs[2] = { veh, 1 };
    pg_zombie_call(instance, 0x8aba6af54b942b95ull, undriveArgs, 2);

    // 0xb664292eaecf7fa6: SET_VEHICLE_DOORS_LOCKED (2 = locked)
    uint64_t lockArgs[2] = { veh, 2 };
    pg_zombie_call(instance, 0xb664292eaecf7fa6ull, lockArgs, 2);

    // 0xec6a202ee4960385: SET_VEHICLE_TYRE_BURST
    uint64_t burstArgs1[4] = { veh, 0, 1, pg_zarg_f32(1000.0f) };
    pg_zombie_call(instance, 0xec6a202ee4960385ull, burstArgs1, 4);
    uint64_t burstArgs2[4] = { veh, 4, 1, pg_zarg_f32(1000.0f) };
    pg_zombie_call(instance, 0xec6a202ee4960385ull, burstArgs2, 4);

    // 0x9e5b5e4d2ccd2259: SMASH_VEHICLE_WINDOW
    uint64_t winArgs[2] = { veh, 0 };
    pg_zombie_call(instance, 0x9e5b5e4d2ccd2259ull, winArgs, 2);

    activeWrecks[freeSlot] = veh;
  }
  PG_FN_RELEASE_MODEL(instance, modelHash);
}

// 4. Weapon Milestone Progression
static void pg_zombie_update_weapons(w2c_game *instance, u32 playerPed, int kills) {
  int currentTier = atomic_load(&zombieTier);
  int targetTier = 0;
  if (kills >= 75) targetTier = 5;
  else if (kills >= 50) targetTier = 4;
  else if (kills >= 30) targetTier = 3;
  else if (kills >= 15) targetTier = 2;
  else if (kills >= 5)  targetTier = 1;

  if (targetTier > currentTier) {
    atomic_store(&zombieTier, targetTier);
    if (targetTier == 1) {
      // Pistol (Combat Pistol) with 60 rounds
      PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_combatpistol"), 60, 0, 1);
      snprintf(zombieStatusText, sizeof(zombieStatusText), "MO KHOA VU KHI: SUNG LUC (5 KILLS)");
    } else if (targetTier == 2) {
      // Pump Shotgun with 30 shells
      PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_pumpshotgun"), 30, 0, 1);
      snprintf(zombieStatusText, sizeof(zombieStatusText), "MO KHOA VU KHI: PUMP SHOTGUN (15 KILLS)");
    } else if (targetTier == 3) {
      // Micro SMG with 150 rounds
      PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_microsmg"), 150, 0, 1);
      snprintf(zombieStatusText, sizeof(zombieStatusText), "MO KHOA VU KHI: MICRO SMG (30 KILLS)");
    } else if (targetTier == 4) {
      // Carbine Assault Rifle with 200 rounds
      PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_carbinerifle"), 200, 0, 1);
      snprintf(zombieStatusText, sizeof(zombieStatusText), "MO KHOA VU KHI: CARBINE RIFLE (50 KILLS)");
    } else if (targetTier == 5) {
      // Heavy Combat MG & Assault Shotgun
      PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_combatmg"), 300, 0, 1);
      PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_assaultshotgun"), 80, 0, 0);
      snprintf(zombieStatusText, sizeof(zombieStatusText), "VU KHI CUOI: COMBAT MG & ASSAULT SHOTGUN (75 KILLS)");
    }
  }
}

// 5. Survivor Companion Management
static void pg_zombie_manage_companion(w2c_game *instance, u32 playerPed, float px, float py, float pz, float heading, double now) {
  // Check companion health or respawn timer
  if (companionPed && PG_FN_ENTITY_EXISTS(instance, companionPed)) {
    // 0x3317dedb88c95038: IS_PED_DEAD_OR_DYING
    uint64_t deadArgs[1] = { companionPed };
    bool isDead = (bool)pg_zombie_call(instance, 0x3317dedb88c95038ull, deadArgs, 1);
    if (isDead) {
      if (companionBlip) {
        uint64_t rmArgs[1] = { companionBlip };
        pg_zombie_call(instance, 0x86a652570e5f25ddull, rmArgs, 1); // REMOVE_BLIP
        companionBlip = 0;
      }
      companionPed = 0;
      companionRespawnAt = now + 40000; // Respawn after 40 seconds
      snprintf(zombieStatusText, sizeof(zombieStatusText), "DONG DOI DA HY SINH! DANG CHO TIEP VIEN...");
      return;
    }

    // Distance check to player
    PG_FN_GET_ENTITY_COORDS(instance, zombieVectorBuf, companionPed, 1);
    float *cc = (float*)pg_pointer(&env, zombieVectorBuf, 24);
    float dx = px - cc[0], dy = py - cc[1];
    float distSq = dx * dx + dy * dy;

    // If companion fell behind (> 70m), warp near player
    if (distSq > 70.0f * 70.0f) {
      float *v = (float*)pg_pointer(&env, zombieVectorBuf, 24);
      v[0] = px - 2.0f; v[1] = 0; v[2] = py - 2.0f; v[3] = 0; v[4] = pz + 0.5f; v[5] = 0;
      PG_FN_SET_ENTITY_COORDS_NO_OFFSET(instance, companionPed, zombieVectorBuf, 0, 0, 0);
    }

    // Combat AI: Find closest zombie to defend player
    if (now - lastCompanionCheck > 1500) {
      lastCompanionCheck = now;
      u32 closestZombie = 0;
      float closestDistSq = 30.0f * 30.0f;
      for (int i = 0; i < ZOMBIE_MAX_ACTIVE; i++) {
        u32 z = activeZombies[i];
        if (z && PG_FN_ENTITY_EXISTS(instance, z)) {
          PG_FN_GET_ENTITY_COORDS(instance, zombieVectorBuf, z, 1);
          float *zc = (float*)pg_pointer(&env, zombieVectorBuf, 24);
          float zdx = px - zc[0], zdy = py - zc[1];
          float zdSq = zdx * zdx + zdy * zdy;
          if (zdSq < closestDistSq) { closestDistSq = zdSq; closestZombie = z; }
        }
      }

      if (closestZombie) {
        // 0xf166e48407bac484: TASK_COMBAT_PED
        uint64_t cArgs[4] = { companionPed, closestZombie, 0, 16 };
        pg_zombie_call(instance, 0xf166e48407bac484ull, cArgs, 4);
      } else if (distSq > 10.0f * 10.0f) {
        // Follow player (0x6a071245eb0d1882: TASK_GO_TO_ENTITY)
        uint64_t fArgs[7] = { companionPed, playerPed, (uint64_t)-1, pg_zarg_f32(2.5f), pg_zarg_f32(2.0f), 1073741824, 0 };
        pg_zombie_call(instance, 0x6a071245eb0d1882ull, fArgs, 7);
      }
    }
    return;
  }

  // Spawn companion if absent and respawn timer ready
  if (now < companionRespawnAt) return;
  PG_FN_REQUEST_MODEL(instance, companionModel);
  if (!PG_FN_HAS_MODEL_LOADED(instance, companionModel)) return;

  // 0xd49f9b0955c367de: CREATE_PED (CIVMALE = 4)
  float spawnX = px - sinf(heading * 0.0174533f) * 2.0f;
  float spawnY = py + cosf(heading * 0.0174533f) * 2.0f;
  uint64_t pArgs[8] = {
    4, companionModel, pg_zarg_f32(spawnX), pg_zarg_f32(spawnY), pg_zarg_f32(pz), pg_zarg_f32(heading), 0, 1
  };
  companionPed = (u32)pg_zombie_call(instance, 0xd49f9b0955c367deull, pArgs, 8);

  if (companionPed && PG_FN_ENTITY_EXISTS(instance, companionPed)) {
    PG_FN_SET_ENTITY_HEALTH(instance, companionPed, 300, 0);

    // Armour 100 (0xcea04d83135264cc: SET_PED_ARMOUR)
    uint64_t armArgs[2] = { companionPed, 100 };
    pg_zombie_call(instance, 0xcea04d83135264ccull, armArgs, 2);

    // Accuracy 80 (0x7aefb85c1d49deb6: SET_PED_ACCURACY)
    uint64_t accArgs[2] = { companionPed, 80 };
    pg_zombie_call(instance, 0x7aefb85c1d49deb6ull, accArgs, 2);

    // Weapons for companion
    PG_FN_GIVE_WEAPON(instance, companionPed, pg_joaat("weapon_pumpshotgun"), 500, 0, 1);
    PG_FN_GIVE_WEAPON(instance, companionPed, pg_joaat("weapon_combatpistol"), 500, 0, 0);

    // Group with player (0x9f3480fe65db31b5: SET_PED_AS_GROUP_MEMBER)
    // 0xf162e133b4e7a675: GET_PED_GROUP_INDEX
    uint64_t pGroup = pg_zombie_call(instance, 0xf162e133b4e7a675ull, (uint64_t[]){ playerPed }, 1);
    if (!pGroup) {
      pGroup = pg_zombie_call(instance, 0x90370ebe0fee1a3dull, (uint64_t[]){ 0 }, 1); // CREATE_GROUP
    }
    uint64_t grpArgs[2] = { companionPed, pGroup };
    pg_zombie_call(instance, 0x9f3480fe65db31b5ull, grpArgs, 2);

    // 0x3dbfc55d5c9bb447: SET_PED_NEVER_LEAVES_GROUP
    uint64_t stayArgs[2] = { companionPed, 1 };
    pg_zombie_call(instance, 0x3dbfc55d5c9bb447ull, stayArgs, 2);

    // Friendly Blue Radar Blip (0x5cde92c702a8fce7: ADD_BLIP_FOR_ENTITY)
    companionBlip = (u32)pg_zombie_call(instance, 0x5cde92c702a8fce7ull, (uint64_t[]){ companionPed }, 1);
    if (companionBlip) {
      uint64_t colArgs[2] = { companionBlip, 3 }; // Color 3 = Blue
      pg_zombie_call(instance, 0x03d7fb09e75d6b7eull, colArgs, 2);
      uint64_t scArgs[2] = { companionBlip, pg_zarg_f32(0.85f) };
      pg_zombie_call(instance, 0xd38744167b2fa257ull, scArgs, 2);
    }

    snprintf(zombieStatusText, sizeof(zombieStatusText), "DONG DOI DA XUAT HIEN! HO TRO TAC CHIEN");
  }
  PG_FN_RELEASE_MODEL(instance, companionModel);
}

// 6. Controlled Zombie Spawning, Aggro AI, Sounds & Bite Damage
static void pg_zombie_manage_horde(w2c_game *instance, u32 playerPed, float px, float py, float pz, float heading, double now) {
  // Preload zombie models
  for (int m = 0; m < 5; m++) {
    PG_FN_REQUEST_MODEL(instance, zombiePedModels[m]);
  }

  // Iterate over active zombies
  for (int i = 0; i < ZOMBIE_MAX_ACTIVE; i++) {
    u32 z = activeZombies[i];
    if (!z) continue;

    if (!PG_FN_ENTITY_EXISTS(instance, z)) {
      if (zombieBlips[i]) {
        uint64_t rmArgs[1] = { zombieBlips[i] };
        pg_zombie_call(instance, 0x86a652570e5f25ddull, rmArgs, 1);
        zombieBlips[i] = 0;
      }
      activeZombies[i] = 0;
      continue;
    }

    // Check if zombie is dead
    // 0x3317dedb88c95038: IS_PED_DEAD_OR_DYING
    uint64_t deadArgs[1] = { z };
    bool isDead = (bool)pg_zombie_call(instance, 0x3317dedb88c95038ull, deadArgs, 1);
    if (isDead) {
      if (zombieBlips[i]) {
        uint64_t rmArgs[1] = { zombieBlips[i] };
        pg_zombie_call(instance, 0x86a652570e5f25ddull, rmArgs, 1);
        zombieBlips[i] = 0;
      }
      // Increment kill count!
      int currentKills = atomic_fetch_add(&zombieKills, 1) + 1;
      pg_zombie_update_weapons(instance, playerPed, currentKills);

      // Release ped
      uint64_t relArgs[1] = { z };
      pg_zombie_call(instance, 0xb736a491e64a32cfull, relArgs, 1);
      activeZombies[i] = 0;
      continue;
    }

    // Distance to player
    PG_FN_GET_ENTITY_COORDS(instance, zombieVectorBuf, z, 1);
    float *zc = (float*)pg_pointer(&env, zombieVectorBuf, 24);
    float dx = px - zc[0], dy = py - zc[1];
    float dist = sqrtf(dx * dx + dy * dy);

    // Despawn if too far (> 65m)
    if (dist > 65.0f) {
      if (zombieBlips[i]) {
        uint64_t rmArgs[1] = { zombieBlips[i] };
        pg_zombie_call(instance, 0x86a652570e5f25ddull, rmArgs, 1);
        zombieBlips[i] = 0;
      }
      uint64_t relArgs[1] = { z };
      pg_zombie_call(instance, 0xb736a491e64a32cfull, relArgs, 1);
      activeZombies[i] = 0;
      continue;
    }

    // Sound: Guttural groaning when nearby (3m - 10m), throttled to once every 3.5 seconds
    if (dist > 3.0f && dist < 10.0f && (now - lastGroanSound > 3500)) {
      lastGroanSound = now;
      uint64_t painArgs[3] = { z, 6, 0 }; // PLAY_PAIN (0xbc9ae166038a5cec)
      pg_zombie_call(instance, 0xbc9ae166038a5cecull, painArgs, 3);
    }

    // Attack & Bite Damage: Distance < 1.6m
    if (dist < 1.6f && (now - lastBiteDamage > 1150)) {
      lastBiteDamage = now;
      // 0xeef059fad016d209: GET_ENTITY_HEALTH
      uint64_t hArgs[1] = { playerPed };
      u32 pHealth = (u32)pg_zombie_call(instance, 0xeef059fad016d209ull, hArgs, 1);
      if (pHealth > 100) {
        u32 newHealth = (pHealth > 116) ? (pHealth - 16) : 100;
        PG_FN_SET_ENTITY_HEALTH(instance, playerPed, newHealth, 0);

        // Pain sound and flinch for player
        uint64_t pPainArgs[3] = { playerPed, 8, 0 };
        pg_zombie_call(instance, 0xbc9ae166038a5cecull, pPainArgs, 3);

        // Zombie bite growl
        uint64_t zBiteArgs[3] = { z, 7, 0 };
        pg_zombie_call(instance, 0xbc9ae166038a5cecull, zBiteArgs, 3);
      }
    }
  }

  // Spawn new zombie batch if below cap
  if (now - lastZombieSpawn < 2200) return;
  lastZombieSpawn = now;

  int freeSlot = -1;
  for (int i = 0; i < ZOMBIE_MAX_ACTIVE; i++) {
    if (!activeZombies[i]) { freeSlot = i; break; }
  }
  if (freeSlot < 0) return;

  // Pick random loaded model
  static unsigned zombieModelIdx = 0;
  uint32_t model = zombiePedModels[zombieModelIdx % 5];
  zombieModelIdx++;

  if (!PG_FN_HAS_MODEL_LOADED(instance, model)) return;

  // Spawn at random angle 25m - 42m away from player
  float dist = 26.0f + (float)(rand() % 16);
  float angle = (float)(rand() % 360) * 0.0174533f;
  float zx = px + cosf(angle) * dist;
  float zy = py + sinf(angle) * dist;

  // 0xd49f9b0955c367de: CREATE_PED (CIVMALE = 4)
  uint64_t pArgs[8] = {
    4, model, pg_zarg_f32(zx), pg_zarg_f32(zy), pg_zarg_f32(pz), pg_zarg_f32(0.0f), 0, 1
  };
  u32 zPed = (u32)pg_zombie_call(instance, 0xd49f9b0955c367deull, pArgs, 8);

  if (zPed && PG_FN_ENTITY_EXISTS(instance, zPed)) {
    // Disarm (0xf25df915fa38c5f3: REMOVE_ALL_PED_WEAPONS)
    uint64_t disarmArgs[2] = { zPed, 1 };
    pg_zombie_call(instance, 0xf25df915fa38c5f3ull, disarmArgs, 2);

    // Set fearless (0x9f8aa94d6d97dbf4: SET_BLOCKING_OF_NON_TEMPORARY_EVENTS)
    uint64_t blockArgs[2] = { zPed, 1 };
    pg_zombie_call(instance, 0x9f8aa94d6d97dbf4ull, blockArgs, 2);

    // Move rate override: fast aggressive rush 1.25f (0x085bf80fa50a39d1)
    uint64_t spdArgs[2] = { zPed, pg_zarg_f32(1.25f) };
    pg_zombie_call(instance, 0x085bf80fa50a39d1ull, spdArgs, 2);

    // Health 160
    PG_FN_SET_ENTITY_HEALTH(instance, zPed, 160, 0);

    // Can ragdoll (0xb128377056a54e2a: SET_PED_CAN_RAGDOLL)
    uint64_t ragArgs[2] = { zPed, 1 };
    pg_zombie_call(instance, 0xb128377056a54e2aull, ragArgs, 2);

    // Target player or companion
    u32 targetPed = playerPed;
    if (companionPed && PG_FN_ENTITY_EXISTS(instance, companionPed) && (rand() % 3 == 0)) {
      targetPed = companionPed;
    }
    // 0xf166e48407bac484: TASK_COMBAT_PED
    uint64_t cArgs[4] = { zPed, targetPed, 0, 16 };
    pg_zombie_call(instance, 0xf166e48407bac484ull, cArgs, 4);

    // Red Blip on Radar (0x5cde92c702a8fce7: ADD_BLIP_FOR_ENTITY)
    u32 zBlip = (u32)pg_zombie_call(instance, 0x5cde92c702a8fce7ull, (uint64_t[]){ zPed }, 1);
    if (zBlip) {
      uint64_t colArgs[2] = { zBlip, 1 }; // 1 = Red
      pg_zombie_call(instance, 0x03d7fb09e75d6b7eull, colArgs, 2);
      uint64_t scArgs[2] = { zBlip, pg_zarg_f32(0.68f) };
      pg_zombie_call(instance, 0xd38744167b2fa257ull, scArgs, 2);
    }

    activeZombies[freeSlot] = zPed;
    zombieBlips[freeSlot] = zBlip;
  }
}

// 7. Initial Loadout Setup (Melee Only at Start)
static void pg_zombie_equip_initial_loadout(w2c_game *instance, u32 playerPed) {
  // Disarm all firearms
  uint64_t disarmArgs[2] = { playerPed, 1 };
  pg_zombie_call(instance, 0xf25df915fa38c5f3ull, disarmArgs, 2);

  // Give melee weapons only
  PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_machete"), 1, 0, 1);
  PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_knife"), 1, 0, 0);
  PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_crowbar"), 1, 0, 0);

  // Set health & armour
  PG_FN_SET_ENTITY_HEALTH(instance, playerPed, 200, 0);
  uint64_t armArgs[2] = { playerPed, 50 };
  pg_zombie_call(instance, 0xcea04d83135264ccull, armArgs, 2);

  atomic_store(&zombieKills, 0);
  atomic_store(&zombieTier, 0);
  snprintf(zombieStatusText, sizeof(zombieStatusText), "BAT DAU SINH TON: TRANG BI CAN CHIEN (0 KILLS)");
}

// Main Frame Tick Loop (called on main script thread)
static void pg_zombie_tick(w2c_game *instance) {
  if (!atomic_load(&zombieActive) || PGPauseMenuActive() || PGLoadSceneActive()) return;
  if (PGGameStatus() & (16 | 4 | 2 | 32)) return;

  u32 playerPed = PG_FN_PLAYER_PED_ID(instance);
  if (!playerPed || !PG_FN_ENTITY_EXISTS(instance, playerPed)) return;

  static int warmup = 0;
  if (++warmup < 240) return; // Wait 4 seconds for world load

  if (!pg_asi_game) pg_asi_game = instance;
  if (!zombieScratch) zombieScratch = PG_FN_OPERATOR_NEW(instance, 256);
  if (!zombieVectorBuf) zombieVectorBuf = PG_FN_OPERATOR_NEW(instance, 64);
  if (!zombieScratch || !zombieVectorBuf) return;

  u64 prev = pg_asi_enter_script(instance);
  double now = timeMS(CLOCK_MONOTONIC);

  // Check reset request
  static bool initialized = false;
  if (!initialized || atomic_exchange(&zombieResetRequested, false)) {
    initialized = true;
    pg_zombie_equip_initial_loadout(instance, playerPed);
  }

  // 1. Atmosphere & Lighting (every 2 seconds)
  if (now - lastWorldSync > 2000) {
    lastWorldSync = now;
    pg_zombie_apply_atmosphere(instance);
  }

  // 2. Clear Ambient Cops & Traffic
  pg_zombie_suppress_ambient(instance);

  // Player position
  PG_FN_GET_ENTITY_COORDS(instance, zombieVectorBuf, playerPed, 1);
  float *pc = (float*)pg_pointer(&env, zombieVectorBuf, 24);
  float px = pc[0], py = pc[1], pz = pc[2];
  float heading = PG_FN_GET_ENTITY_HEADING(instance, playerPed);

  // 3. Roadside Abandoned Wrecks
  pg_zombie_manage_wrecks(instance, px, py, pz, heading, now);

  // 4. Survivor Companion
  pg_zombie_manage_companion(instance, playerPed, px, py, pz, heading, now);

  // 5. Zombie Horde & Combat
  pg_zombie_manage_horde(instance, playerPed, px, py, pz, heading, now);

  pg_asi_leave_script(instance, prev);
}

// Public API for Swift / UIKit
bool PGZombieIsActive(void) { return atomic_load(&zombieActive); }
void PGZombieSetActive(bool active) {
  atomic_store(&zombieActive, active);
  if (!active) {
    // Clear timecycle modifier on disable (0x0f07e7745a236711: CLEAR_TIMECYCLE_MODIFIER)
    if (pg_asi_game) {
      uint64_t prev = pg_asi_enter_script(pg_asi_game);
      pg_zombie_call(pg_asi_game, 0x0f07e7745a236711ull, NULL, 0);
      pg_asi_leave_script(pg_asi_game, prev);
    }
  }
}
int PGZombieGetKills(void) { return atomic_load(&zombieKills); }
int PGZombieGetTier(void) { return atomic_load(&zombieTier); }
bool PGZombieCompanionAlive(void) { return companionPed != 0; }
void PGZombieResetSurvival(void) { atomic_store(&zombieResetRequested, true); }
void PGZombieGetStatus(char *buffer, size_t maxLen) {
  if (!buffer || maxLen == 0) return;
  snprintf(buffer, maxLen, "%s", zombieStatusText);
}
