// ============================================================================
// NATIVE ZOMBIE APOCALYPSE SURVIVAL SYSTEM - V2 FULL UPGRADE
// PlayGTA5Native / Mugoat NewGen for iOS
// Sửa toàn diện: Không mù sương, triệt tiêu tiếng xe NPC, lây nhiễm toàn bộ NPC
// xung quanh thành Zombie, hiện đầy đủ radar từ xa, mục tiêu thoát khỏi thành phố.
// ============================================================================
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdatomic.h>
#include <stdlib.h>

#define ZOMBIE_MAX_ACTIVE 28
#define ZOMBIE_MAX_WRECKS 12

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
u32 PG_FN_GROUND_VEHICLE(w2c_game*, u32, f32);
void PG_FN_GIVE_WEAPON(w2c_game*, u32, u32, u32, u32, u32);
u32 PG_FN_ENTITY_EXISTS(w2c_game*, u32);
void PG_FN_SET_ENTITY_HEALTH(w2c_game*, u32, u32, u32);
u32 PG_FN_GET_ENTITY_MODEL(w2c_game*, u32);
void PG_FN_SET_ENTITY_COORDS_NO_OFFSET(w2c_game*, u32, u64, u32, u32, u32);

// Models for abandoned roadside wrecks
static const char *const zombieWreckModels[] = {
  "rebel", "stanier", "voodoo2", "ingot", "emperor2", "tornado3", "rancherxl", "glendale"
};

// Pinned direct zombie model (guaranteed loaded)
static const uint32_t pinnedZombieModel = 0x19DFA98E; // a_m_y_skater_01

// Survivor companion model (Tough Marine / Security Guard)
static const uint32_t companionModel = 0x8D8F1B10; // s_m_y_marine_01

// Evacuation / Escape Destination: Del Perro Pier
static const float evacX = -1600.0f;
static const float evacY = -1050.0f;
static const float evacZ = 13.0f;

// Global state
static _Atomic bool zombieActive = true;
static _Atomic int zombieKills = 0;
static _Atomic int zombieTier = 0;
static _Atomic bool zombieResetRequested = false;
static _Atomic float zombieDistanceToEvac = 2000.0f;
static _Atomic bool zombieEscaped = false;

// Internal runtime tracking
static u32 activeZombies[ZOMBIE_MAX_ACTIVE];
static u32 zombieBlips[ZOMBIE_MAX_ACTIVE];
static u32 activeWrecks[ZOMBIE_MAX_WRECKS];
static u32 companionPed = 0;
static u32 companionBlip = 0;
static u32 evacBlip = 0;

static double lastZombieSpawn = 0;
static double lastWreckSpawn = 0;
static double lastCompanionCheck = 0;
static double companionRespawnAt = 0;
static double lastBiteDamage = 0;
static double lastGroanSound = 0;
static double lastWorldSync = 0;
static double lastPedScan = 0;
static double lastVehScan = 0;
static double evacHoldStartTime = 0;

static uint64_t zombieScratch = 0;
static uint64_t zombieStrWeather = 0;
static uint64_t zombieStrTimecycle = 0;
static uint64_t zombieVectorBuf = 0;
static uint64_t zombiePedScanBuf = 0;
static uint64_t zombieVehScanBuf = 0;
static uint64_t audioSceneStr = 0;

static char zombieStatusText[160] = "ZOMBIE SURVIVAL: SAN SANG XUAT PHAT";

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

// 1. Atmosphere: Daytime 12:00 PM, Bright Daylight, Overcast Sky, Subtle Grayish Tint (NO Dense Fog)
static void pg_zombie_apply_atmosphere(w2c_game *instance) {
  // Broad daylight at 12:00 PM
  PG_FN_SET_CLOCK(instance, 12, 0, 0);

  // Weather: OVERCAST (bright daylight, eerie clouds, clear vision without blinding white fog)
  if (!zombieStrWeather) {
    zombieStrWeather = PG_FN_OPERATOR_NEW(instance, 32);
    if (zombieStrWeather) snprintf((char*)pg_pointer(&env, zombieStrWeather, 32), 32, "OVERCAST");
  }
  if (zombieStrWeather) {
    PG_FN_CLEAR_WEATHER_OVERRIDE(instance);
    PG_FN_SET_WEATHER(instance, zombieStrWeather);
  }

  // Timecycle: Subtle desaturated cold filter (prologue_shootout at 0.38f - light and visible)
  if (!zombieStrTimecycle) {
    zombieStrTimecycle = PG_FN_OPERATOR_NEW(instance, 48);
    if (zombieStrTimecycle) snprintf((char*)pg_pointer(&env, zombieStrTimecycle, 48), 48, "prologue_shootout");
  }
  if (zombieStrTimecycle) {
    uint64_t tcArgs[1] = { zombieStrTimecycle };
    pg_zombie_call(instance, 0x2c933abf17a1df41ull, tcArgs, 1); // SET_TIMECYCLE_MODIFIER
    uint64_t strArgs[1] = { pg_zarg_f32(0.38f) };
    pg_zombie_call(instance, 0x82e7ffcd5b2326b3ull, strArgs, 1); // SET_TIMECYCLE_MODIFIER_STRENGTH
  }

  // Audio: Silence ambient city traffic sounds completely
  if (!audioSceneStr) {
    audioSceneStr = PG_FN_OPERATOR_NEW(instance, 64);
    if (audioSceneStr) snprintf((char*)pg_pointer(&env, audioSceneStr, 64), 64, "DLC_AW_Arena_Wars_Disable_Ambient_Traffic");
  }
  if (audioSceneStr) {
    pg_zombie_call(instance, 0x013a80fc08f6e4f2ull, (uint64_t[]){ audioSceneStr }, 1); // START_AUDIO_SCENE
  }
}

// 2. Clear Normal Traffic and Police, ALLOW Ped Density for Zombie Conversion
static void pg_zombie_suppress_traffic(w2c_game *instance) {
  // Ped density at 1.0f so natural pedestrians spawn, then get instantly infected into zombies!
  PG_FN_PED_DENSITY_THIS_FRAME(instance, 1.0f);

  // Absolutely NO driving vehicle traffic or parked vehicle density
  PG_FN_VEHICLE_DENSITY_THIS_FRAME(instance, 0.0f);
  PG_FN_PARKED_DENSITY_THIS_FRAME(instance, 0.0f);

  uint64_t zeroArg[1] = { 0 };
  pg_zombie_call(instance, 0xb3b3359379fe77d3ull, zeroArg, 1); // SET_RANDOM_VEHICLE_DENSITY_MULTIPLIER_THIS_FRAME
  pg_zombie_call(instance, 0x90b6da738a9a25daull, zeroArg, 1); // SET_AMBIENT_VEHICLE_RANGE_MULTIPLIER_THIS_FRAME

  // Police suppression: zero wanted, ignore player, cancel dispatch
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

// 3. Scan & Turn OFF All Ambient Vehicles, Eject Drivers & Create Abandoned Wrecks
static void pg_zombie_silence_and_wreck_vehicles(w2c_game *instance, u32 playerPed) {
  if (!zombieVehScanBuf) zombieVehScanBuf = PG_FN_OPERATOR_NEW(instance, 256);
  if (!zombieVehScanBuf) return;

  int32_t *vehs = (int32_t*)pg_pointer(&env, zombieVehScanBuf, 256);
  vehs[0] = 30; // Max query
  uint64_t vArgs[2] = { playerPed, zombieVehScanBuf };
  // 0xcff869cbfa210d82: GET_PED_NEARBY_VEHICLES
  int vFound = (int)pg_zombie_call(instance, 0xcff869cbfa210d82ull, vArgs, 2);

  if (vFound > 0) {
    for (int k = 1; k <= vFound && k <= 30; k++) {
      u32 veh = (u32)vehs[k];
      if (!veh || !PG_FN_ENTITY_EXISTS(instance, veh)) continue;

      // Check and eject/remove any driver
      // 0xbb40dd2270b65366: GET_PED_IN_VEHICLE_SEAT (-1 = driver)
      uint64_t seatArgs[2] = { veh, (uint64_t)-1 };
      u32 driver = (u32)pg_zombie_call(instance, 0xbb40dd2270b65366ull, seatArgs, 2);
      if (driver && PG_FN_ENTITY_EXISTS(instance, driver) && driver != playerPed && driver != companionPed) {
        // Eject driver
        uint64_t outArgs[3] = { driver, veh, 16 };
        pg_zombie_call(instance, 0xd3dbce61a490be02ull, outArgs, 3); // TASK_LEAVE_VEHICLE
      }

      // Turn off engine immediately (0x2497c4717c8b881e: SET_VEHICLE_ENGINE_ON)
      uint64_t engArgs[4] = { veh, 0, 1, 1 };
      pg_zombie_call(instance, 0x2497c4717c8b881eull, engArgs, 4);

      // Make it an abandoned dirty wreck
      PG_FN_VEHICLE_DIRT(instance, veh, 15.0f);
      PG_FN_VEHICLE_ENGINE_HEALTH(instance, veh, -4000.0f);

      // 0x8aba6af54b942b95: SET_VEHICLE_UNDRIVEABLE
      uint64_t undriveArgs[2] = { veh, 1 };
      pg_zombie_call(instance, 0x8aba6af54b942b95ull, undriveArgs, 2);

      // 0xb664292eaecf7fa6: SET_VEHICLE_DOORS_LOCKED (2 = locked)
      uint64_t lockArgs[2] = { veh, 2 };
      pg_zombie_call(instance, 0xb664292eaecf7fa6ull, lockArgs, 2);
    }
  }
}

// 4. Abandoned Roadside Wrecks
static void pg_zombie_manage_wrecks(w2c_game *instance, float px, float py, float pz, float heading, double now) {
  // Clean up distant wrecks (> 85m)
  for (int i = 0; i < ZOMBIE_MAX_WRECKS; i++) {
    u32 veh = activeWrecks[i];
    if (veh && PG_FN_ENTITY_EXISTS(instance, veh)) {
      PG_FN_GET_ENTITY_COORDS(instance, zombieVectorBuf, veh, 1);
      float *c = (float*)pg_pointer(&env, zombieVectorBuf, 24);
      float dx = px - c[0], dy = py - c[1];
      if (dx * dx + dy * dy > 85.0f * 85.0f) {
        uint64_t rArgs[1] = { veh };
        pg_zombie_call(instance, 0xb736a491e64a32cfull, rArgs, 1); // SET_ENTITY_AS_NO_LONGER_NEEDED
        activeWrecks[i] = 0;
      }
    } else {
      activeWrecks[i] = 0;
    }
  }

  if (now - lastWreckSpawn < 3000) return;
  lastWreckSpawn = now;

  int freeSlot = -1;
  for (int i = 0; i < ZOMBIE_MAX_WRECKS; i++) {
    if (!activeWrecks[i]) { freeSlot = i; break; }
  }
  if (freeSlot < 0) return;

  static unsigned wreckIndex = 0;
  const char *modelName = zombieWreckModels[wreckIndex % 8];
  wreckIndex++;
  uint32_t modelHash = pg_joaat(modelName);

  PG_FN_REQUEST_MODEL(instance, modelHash);
  if (!PG_FN_HAS_MODEL_LOADED(instance, modelHash)) return;

  float dist = 22.0f + (float)(rand() % 22);
  float angOffset = (float)((rand() % 90) - 45) * 0.0174533f;
  float rad = heading * 0.0174533f + angOffset;

  float *v = (float*)pg_pointer(&env, zombieVectorBuf, 24);
  v[0] = px - sinf(rad) * dist; v[1] = 0;
  v[2] = py + cosf(rad) * dist; v[3] = 0;
  v[4] = pz + 0.5f; v[5] = 0;

  float wreckHeading = heading + (float)(rand() % 180);
  u32 veh = PG_FN_CREATE_VEHICLE(instance, modelHash, zombieVectorBuf, wreckHeading, 0, 0, 1);
  if (veh && PG_FN_ENTITY_EXISTS(instance, veh)) {
    PG_FN_GROUND_VEHICLE(instance, veh, 5.0f);
    PG_FN_VEHICLE_DIRT(instance, veh, 15.0f);
    PG_FN_VEHICLE_ENGINE_HEALTH(instance, veh, -4000.0f);

    uint64_t undriveArgs[2] = { veh, 1 };
    pg_zombie_call(instance, 0x8aba6af54b942b95ull, undriveArgs, 2);

    uint64_t lockArgs[2] = { veh, 2 };
    pg_zombie_call(instance, 0xb664292eaecf7fa6ull, lockArgs, 2);

    uint64_t burstArgs1[4] = { veh, 0, 1, pg_zarg_f32(1000.0f) };
    pg_zombie_call(instance, 0xec6a202ee4960385ull, burstArgs1, 4);
    uint64_t burstArgs2[4] = { veh, 4, 1, pg_zarg_f32(1000.0f) };
    pg_zombie_call(instance, 0xec6a202ee4960385ull, burstArgs2, 4);

    uint64_t winArgs[2] = { veh, 0 };
    pg_zombie_call(instance, 0x9e5b5e4d2ccd2259ull, winArgs, 2);

    activeWrecks[freeSlot] = veh;
  }
  PG_FN_RELEASE_MODEL(instance, modelHash);
}

// 5. Weapon Milestone Progression
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
      PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_combatpistol"), 80, 0, 1);
    } else if (targetTier == 2) {
      PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_pumpshotgun"), 40, 0, 1);
    } else if (targetTier == 3) {
      PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_microsmg"), 180, 0, 1);
    } else if (targetTier == 4) {
      PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_carbinerifle"), 250, 0, 1);
    } else if (targetTier == 5) {
      PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_combatmg"), 400, 0, 1);
      PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_assaultshotgun"), 100, 0, 0);
    }
  }
}

// 6. Survivor Companion Management
static void pg_zombie_manage_companion(w2c_game *instance, u32 playerPed, float px, float py, float pz, float heading, double now) {
  if (companionPed && PG_FN_ENTITY_EXISTS(instance, companionPed)) {
    uint64_t deadArgs[1] = { companionPed };
    bool isDead = (bool)pg_zombie_call(instance, 0x3317dedb88c95038ull, deadArgs, 1);
    if (isDead) {
      if (companionBlip) {
        uint64_t rmArgs[1] = { companionBlip };
        pg_zombie_call(instance, 0x86a652570e5f25ddull, rmArgs, 1);
        companionBlip = 0;
      }
      companionPed = 0;
      companionRespawnAt = now + 35000;
      return;
    }

    PG_FN_GET_ENTITY_COORDS(instance, zombieVectorBuf, companionPed, 1);
    float *cc = (float*)pg_pointer(&env, zombieVectorBuf, 24);
    float dx = px - cc[0], dy = py - cc[1];
    float distSq = dx * dx + dy * dy;

    if (distSq > 75.0f * 75.0f) {
      float *v = (float*)pg_pointer(&env, zombieVectorBuf, 24);
      v[0] = px - 2.0f; v[1] = 0; v[2] = py - 2.0f; v[3] = 0; v[4] = pz + 0.5f; v[5] = 0;
      PG_FN_SET_ENTITY_COORDS_NO_OFFSET(instance, companionPed, zombieVectorBuf, 0, 0, 0);
    }

    if (now - lastCompanionCheck > 1200) {
      lastCompanionCheck = now;
      u32 closestZombie = 0;
      float closestDistSq = 35.0f * 35.0f;
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
        uint64_t cArgs[4] = { companionPed, closestZombie, 0, 16 };
        pg_zombie_call(instance, 0xf166e48407bac484ull, cArgs, 4);
      } else if (distSq > 10.0f * 10.0f) {
        uint64_t fArgs[7] = { companionPed, playerPed, (uint64_t)-1, pg_zarg_f32(2.5f), pg_zarg_f32(2.0f), 1073741824, 0 };
        pg_zombie_call(instance, 0x6a071245eb0d1882ull, fArgs, 7);
      }
    }
    return;
  }

  if (now < companionRespawnAt) return;
  PG_FN_REQUEST_MODEL(instance, companionModel);
  if (!PG_FN_HAS_MODEL_LOADED(instance, companionModel)) return;

  float spawnX = px - sinf(heading * 0.0174533f) * 2.0f;
  float spawnY = py + cosf(heading * 0.0174533f) * 2.0f;
  uint64_t pArgs[8] = {
    4, companionModel, pg_zarg_f32(spawnX), pg_zarg_f32(spawnY), pg_zarg_f32(pz), pg_zarg_f32(heading), 0, 1
  };
  companionPed = (u32)pg_zombie_call(instance, 0xd49f9b0955c367deull, pArgs, 8);

  if (companionPed && PG_FN_ENTITY_EXISTS(instance, companionPed)) {
    PG_FN_SET_ENTITY_HEALTH(instance, companionPed, 350, 0);

    uint64_t armArgs[2] = { companionPed, 100 };
    pg_zombie_call(instance, 0xcea04d83135264ccull, armArgs, 2);

    uint64_t accArgs[2] = { companionPed, 85 };
    pg_zombie_call(instance, 0x7aefb85c1d49deb6ull, accArgs, 2);

    PG_FN_GIVE_WEAPON(instance, companionPed, pg_joaat("weapon_pumpshotgun"), 500, 0, 1);
    PG_FN_GIVE_WEAPON(instance, companionPed, pg_joaat("weapon_combatpistol"), 500, 0, 0);

    uint64_t pGroup = pg_zombie_call(instance, 0xf162e133b4e7a675ull, (uint64_t[]){ playerPed }, 1);
    if (!pGroup) pGroup = pg_zombie_call(instance, 0x90370ebe0fee1a3dull, (uint64_t[]){ 0 }, 1);
    uint64_t grpArgs[2] = { companionPed, pGroup };
    pg_zombie_call(instance, 0x9f3480fe65db31b5ull, grpArgs, 2);

    uint64_t stayArgs[2] = { companionPed, 1 };
    pg_zombie_call(instance, 0x3dbfc55d5c9bb447ull, stayArgs, 2);

    companionBlip = (u32)pg_zombie_call(instance, 0x5cde92c702a8fce7ull, (uint64_t[]){ companionPed }, 1);
    if (companionBlip) {
      pg_zombie_call(instance, 0x03d7fb09e75d6b7eull, (uint64_t[]){ companionBlip, 3 }, 2); // Blue
      pg_zombie_call(instance, 0xd38744167b2fa257ull, (uint64_t[]){ companionBlip, pg_zarg_f32(0.9f) }, 2);
      pg_zombie_call(instance, 0xbe8be4fe60e27b72ull, (uint64_t[]){ companionBlip, 0 }, 2); // Always on map
    }
  }
  PG_FN_RELEASE_MODEL(instance, companionModel);
}

// Helper: Convert Any Ped Into a Bloodthirsty Sprinting Zombie with Global Radar Blip
static void pg_zombie_convert_ped(w2c_game *instance, u32 ped, u32 playerPed) {
  if (!ped || !PG_FN_ENTITY_EXISTS(instance, ped) || ped == playerPed || ped == companionPed) return;

  // Check if already in activeZombies
  for (int i = 0; i < ZOMBIE_MAX_ACTIVE; i++) {
    if (activeZombies[i] == ped) return;
  }

  // Find free slot
  int freeSlot = -1;
  for (int i = 0; i < ZOMBIE_MAX_ACTIVE; i++) {
    if (!activeZombies[i]) { freeSlot = i; break; }
  }
  if (freeSlot < 0) return;

  // 1. Disarm (0xf25df915fa38c5f3: REMOVE_ALL_PED_WEAPONS)
  uint64_t disarmArgs[2] = { ped, 1 };
  pg_zombie_call(instance, 0xf25df915fa38c5f3ull, disarmArgs, 2);

  // 2. Fearless / Aggressive (0x9f8aa94d6d97dbf4: SET_BLOCKING_OF_NON_TEMPORARY_EVENTS)
  uint64_t blockArgs[2] = { ped, 1 };
  pg_zombie_call(instance, 0x9f8aa94d6d97dbf4ull, blockArgs, 2);

  // 3. Sprint override: 1.30f fast rush (0x085bf80fa50a39d1)
  uint64_t spdArgs[2] = { ped, pg_zarg_f32(1.30f) };
  pg_zombie_call(instance, 0x085bf80fa50a39d1ull, spdArgs, 2);

  // 4. Health 160
  PG_FN_SET_ENTITY_HEALTH(instance, ped, 160, 0);

  // 5. Can ragdoll (0xb128377056a54e2a)
  uint64_t ragArgs[2] = { ped, 1 };
  pg_zombie_call(instance, 0xb128377056a54e2aull, ragArgs, 2);

  // 6. Target player or companion
  u32 target = playerPed;
  if (companionPed && PG_FN_ENTITY_EXISTS(instance, companionPed) && (rand() % 3 == 0)) {
    target = companionPed;
  }
  uint64_t cArgs[4] = { ped, target, 0, 16 };
  pg_zombie_call(instance, 0xf166e48407bac484ull, cArgs, 4);

  // 7. Red Radar Blip - SET_BLIP_AS_SHORT_RANGE = 0 SO IT SHOWS FROM ANYWHERE ON THE MAP!
  u32 zBlip = (u32)pg_zombie_call(instance, 0x5cde92c702a8fce7ull, (uint64_t[]){ ped }, 1);
  if (zBlip) {
    pg_zombie_call(instance, 0x03d7fb09e75d6b7eull, (uint64_t[]){ zBlip, 1 }, 2); // 1 = Red
    pg_zombie_call(instance, 0xd38744167b2fa257ull, (uint64_t[]){ zBlip, pg_zarg_f32(0.72f) }, 2);
    // 0xbe8be4fe60e27b72: SET_BLIP_AS_SHORT_RANGE (0 = visible across the whole map & radar edge!)
    pg_zombie_call(instance, 0xbe8be4fe60e27b72ull, (uint64_t[]){ zBlip, 0 }, 2);
    // 0x9029b2f3da924928: SET_BLIP_DISPLAY (2 = Both map & minimap)
    pg_zombie_call(instance, 0x9029b2f3da924928ull, (uint64_t[]){ zBlip, 2 }, 2);
  }

  activeZombies[freeSlot] = ped;
  zombieBlips[freeSlot] = zBlip;
}

// 7. Infect All Ambient Pedestrians Into Zombies
static void pg_zombie_infect_ambient_peds(w2c_game *instance, u32 playerPed) {
  if (!zombiePedScanBuf) zombiePedScanBuf = PG_FN_OPERATOR_NEW(instance, 256);
  if (!zombiePedScanBuf) return;

  int32_t *peds = (int32_t*)pg_pointer(&env, zombiePedScanBuf, 256);
  peds[0] = 30; // Max count to query
  uint64_t args[3] = { playerPed, zombiePedScanBuf, (uint64_t)-1 };
  // 0x23f8f5fc7e8c4a6b: GET_PED_NEARBY_PEDS
  int found = (int)pg_zombie_call(instance, 0x23f8f5fc7e8c4a6bull, args, 3);

  if (found > 0) {
    for (int k = 1; k <= found && k <= 30; k++) {
      u32 p = (u32)peds[k];
      pg_zombie_convert_ped(instance, p, playerPed);
    }
  }
}

// 8. Controlled Spawning & Direct Horde Aggression
static void pg_zombie_manage_horde(w2c_game *instance, u32 playerPed, float px, float py, float pz, float heading, double now) {
  // Always request pinned model every frame so it stays 100% loaded
  PG_FN_REQUEST_MODEL(instance, pinnedZombieModel);

  // Scan and update active zombies
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

    // Check if dead
    uint64_t deadArgs[1] = { z };
    bool isDead = (bool)pg_zombie_call(instance, 0x3317dedb88c95038ull, deadArgs, 1);
    if (isDead) {
      if (zombieBlips[i]) {
        uint64_t rmArgs[1] = { zombieBlips[i] };
        pg_zombie_call(instance, 0x86a652570e5f25ddull, rmArgs, 1);
        zombieBlips[i] = 0;
      }
      int currentKills = atomic_fetch_add(&zombieKills, 1) + 1;
      pg_zombie_update_weapons(instance, playerPed, currentKills);

      uint64_t relArgs[1] = { z };
      pg_zombie_call(instance, 0xb736a491e64a32cfull, relArgs, 1);
      activeZombies[i] = 0;
      continue;
    }

    PG_FN_GET_ENTITY_COORDS(instance, zombieVectorBuf, z, 1);
    float *zc = (float*)pg_pointer(&env, zombieVectorBuf, 24);
    float dx = px - zc[0], dy = py - zc[1];
    float dist = sqrtf(dx * dx + dy * dy);

    // Keep active within 95m
    if (dist > 95.0f) {
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

    // Guttural sound when near
    if (dist > 3.0f && dist < 12.0f && (now - lastGroanSound > 3200)) {
      lastGroanSound = now;
      uint64_t painArgs[3] = { z, 6, 0 };
      pg_zombie_call(instance, 0xbc9ae166038a5cecull, painArgs, 3);
    }

    // Real bite damage when close (< 1.6m)
    if (dist < 1.6f && (now - lastBiteDamage > 1100)) {
      lastBiteDamage = now;
      uint64_t hArgs[1] = { playerPed };
      u32 pHealth = (u32)pg_zombie_call(instance, 0xeef059fad016d209ull, hArgs, 1);
      if (pHealth > 100) {
        u32 newHealth = (pHealth > 116) ? (pHealth - 16) : 100;
        PG_FN_SET_ENTITY_HEALTH(instance, playerPed, newHealth, 0);

        uint64_t pPainArgs[3] = { playerPed, 8, 0 };
        pg_zombie_call(instance, 0xbc9ae166038a5cecull, pPainArgs, 3);

        uint64_t zBiteArgs[3] = { z, 7, 0 };
        pg_zombie_call(instance, 0xbc9ae166038a5cecull, zBiteArgs, 3);
      }
    }
  }

  // Direct spawn if below cap and pinned model loaded
  if (now - lastZombieSpawn < 1800) return;
  lastZombieSpawn = now;

  int freeSlot = -1;
  for (int i = 0; i < ZOMBIE_MAX_ACTIVE; i++) {
    if (!activeZombies[i]) { freeSlot = i; break; }
  }
  if (freeSlot < 0) return;

  if (!PG_FN_HAS_MODEL_LOADED(instance, pinnedZombieModel)) return;

  float dist = 24.0f + (float)(rand() % 18);
  float angle = (float)(rand() % 360) * 0.0174533f;
  float zx = px + cosf(angle) * dist;
  float zy = py + sinf(angle) * dist;

  uint64_t pArgs[8] = {
    4, pinnedZombieModel, pg_zarg_f32(zx), pg_zarg_f32(zy), pg_zarg_f32(pz), pg_zarg_f32(0.0f), 0, 1
  };
  u32 zPed = (u32)pg_zombie_call(instance, 0xd49f9b0955c367deull, pArgs, 8);
  if (zPed && PG_FN_ENTITY_EXISTS(instance, zPed)) {
    pg_zombie_convert_ped(instance, zPed, playerPed);
  }
}

// 9. Extraction & City Escape Objective
static void pg_zombie_manage_extraction(w2c_game *instance, u32 playerPed, float px, float py, float pz, double now) {
  // Evacuation blip at Del Perro Pier
  if (!evacBlip) {
    uint64_t bArgs[3] = { pg_zarg_f32(evacX), pg_zarg_f32(evacY), pg_zarg_f32(evacZ) };
    evacBlip = (u32)pg_zombie_call(instance, 0x5a039bb0bca604b6ull, bArgs, 3); // ADD_BLIP_FOR_COORD
    if (evacBlip) {
      pg_zombie_call(instance, 0xdf735600a4696dafull, (uint64_t[]){ evacBlip, 38 }, 2); // Sprite 38 (Helicopter rescue)
      pg_zombie_call(instance, 0x03d7fb09e75d6b7eull, (uint64_t[]){ evacBlip, 2 }, 2);  // Color 2 (Green)
      pg_zombie_call(instance, 0xd38744167b2fa257ull, (uint64_t[]){ evacBlip, pg_zarg_f32(1.25f) }, 2);
      pg_zombie_call(instance, 0xbe8be4fe60e27b72ull, (uint64_t[]){ evacBlip, 0 }, 2); // Visible across entire map!
      pg_zombie_call(instance, 0x9029b2f3da924928ull, (uint64_t[]){ evacBlip, 2 }, 2); // Map & minimap
    }
  }

  float dx = px - evacX;
  float dy = py - evacY;
  float distToEvac = sqrtf(dx * dx + dy * dy);
  atomic_store(&zombieDistanceToEvac, distToEvac);

  int kills = atomic_load(&zombieKills);
  int tier = atomic_load(&zombieTier);

  if (atomic_load(&zombieEscaped)) {
    snprintf(zombieStatusText, sizeof(zombieStatusText), "CHIEN THANG: DA THOAT KHOI THANH PHO THANH CONG!");
    return;
  }

  // Inside extraction zone (< 45m from Del Perro Pier rescue point)
  if (distToEvac < 45.0f) {
    if (evacHoldStartTime == 0) evacHoldStartTime = now;
    int secondsLeft = 45 - (int)((now - evacHoldStartTime) / 1000.0);

    if (secondsLeft > 0) {
      snprintf(zombieStatusText, sizeof(zombieStatusText), "DANG CO THU DIEM SO TAN! CON %d GIAY DE THOAT", secondsLeft);
    } else {
      atomic_store(&zombieEscaped, true);
      snprintf(zombieStatusText, sizeof(zombieStatusText), "CHIEN THANG: TRUC THANG CUU HO DA DEN! THOAT THANH CONG!");
      PG_FN_CLEAR_WANTED(instance, PG_FN_PLAYER_ID(instance));
    }
  } else {
    evacHoldStartTime = 0;
    snprintf(zombieStatusText, sizeof(zombieStatusText), "DIEM SO TAN: %.0f M | KILLS: %d | CAP VU KHI: %d", distToEvac, kills, tier);
  }
}

// 10. Initial Loadout Setup (Melee Only at Start)
static void pg_zombie_equip_initial_loadout(w2c_game *instance, u32 playerPed) {
  uint64_t disarmArgs[2] = { playerPed, 1 };
  pg_zombie_call(instance, 0xf25df915fa38c5f3ull, disarmArgs, 2);

  PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_machete"), 1, 0, 1);
  PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_knife"), 1, 0, 0);
  PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_crowbar"), 1, 0, 0);

  PG_FN_SET_ENTITY_HEALTH(instance, playerPed, 200, 0);
  uint64_t armArgs[2] = { playerPed, 50 };
  pg_zombie_call(instance, 0xcea04d83135264ccull, armArgs, 2);

  atomic_store(&zombieKills, 0);
  atomic_store(&zombieTier, 0);
  atomic_store(&zombieEscaped, false);
  evacHoldStartTime = 0;
  snprintf(zombieStatusText, sizeof(zombieStatusText), "BAT DAU SINH TON: DI CHUYEN DEN DIEM SO TAN");
}

// Main Frame Tick Loop
static void pg_zombie_tick(w2c_game *instance) {
  if (!atomic_load(&zombieActive) || PGPauseMenuActive() || PGLoadSceneActive()) return;
  if (PGGameStatus() & (16 | 4 | 2 | 32)) return;

  u32 playerPed = PG_FN_PLAYER_PED_ID(instance);
  if (!playerPed || !PG_FN_ENTITY_EXISTS(instance, playerPed)) return;

  static int warmup = 0;
  if (++warmup < 240) return;

  if (!pg_asi_game) pg_asi_game = instance;
  if (!zombieScratch) zombieScratch = PG_FN_OPERATOR_NEW(instance, 256);
  if (!zombieVectorBuf) zombieVectorBuf = PG_FN_OPERATOR_NEW(instance, 64);
  if (!zombieScratch || !zombieVectorBuf) return;

  u64 prev = pg_asi_enter_script(instance);
  double now = timeMS(CLOCK_MONOTONIC);

  static bool initialized = false;
  if (!initialized || atomic_exchange(&zombieResetRequested, false)) {
    initialized = true;
    pg_zombie_equip_initial_loadout(instance, playerPed);
  }

  // 1. Atmosphere: 12:00 PM, bright daylight, overcast sky (every 2.5s)
  if (now - lastWorldSync > 2500) {
    lastWorldSync = now;
    pg_zombie_apply_atmosphere(instance);
  }

  // 2. Traffic suppression & Ped spawning enable
  pg_zombie_suppress_traffic(instance);

  // Player position
  PG_FN_GET_ENTITY_COORDS(instance, zombieVectorBuf, playerPed, 1);
  float *pc = (float*)pg_pointer(&env, zombieVectorBuf, 24);
  float px = pc[0], py = pc[1], pz = pc[2];
  float heading = PG_FN_GET_ENTITY_HEADING(instance, playerPed);

  // 3. Scan & Silence All Ambient Vehicles (every 1s)
  if (now - lastVehScan > 1000) {
    lastVehScan = now;
    pg_zombie_silence_and_wreck_vehicles(instance, playerPed);
  }

  // 4. Roadside Abandoned Wrecks
  pg_zombie_manage_wrecks(instance, px, py, pz, heading, now);

  // 5. INFECT ALL AMBIENT PEDS INTO ZOMBIES (every 0.5s)
  if (now - lastPedScan > 500) {
    lastPedScan = now;
    pg_zombie_infect_ambient_peds(instance, playerPed);
  }

  // 6. Direct Horde Management & Bite Attacks
  pg_zombie_manage_horde(instance, playerPed, px, py, pz, heading, now);

  // 7. Survivor Companion
  pg_zombie_manage_companion(instance, playerPed, px, py, pz, heading, now);

  // 8. Extraction & City Escape Objective
  pg_zombie_manage_extraction(instance, playerPed, px, py, pz, now);

  pg_asi_leave_script(instance, prev);
}

// Public API for Swift / UIKit
bool PGZombieIsActive(void) { return atomic_load(&zombieActive); }
void PGZombieSetActive(bool active) {
  atomic_store(&zombieActive, active);
  if (!active) {
    if (pg_asi_game) {
      uint64_t prev = pg_asi_enter_script(pg_asi_game);
      pg_zombie_call(pg_asi_game, 0x0f07e7745a236711ull, NULL, 0); // CLEAR_TIMECYCLE_MODIFIER
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
float PGZombieGetEscapeDistance(void) { return atomic_load(&zombieDistanceToEvac); }
bool PGZombieIsEscaped(void) { return atomic_load(&zombieEscaped); }
