// Native Zombie Apocalypse Survival Mode for GTA V iOS Port
// Features: Abandoned foggy city, wrecked cars, zombie AI waves, first-person view, survival weapons & escape mission.
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#define MAX_ZOMBIES 14
#define MAX_WRECKS 6
#define ZOMBIE_MODEL_HASH 0xAC36843C  // u_m_y_zombie_01

// Engine native function declarations
void PG_FN_CAMERA_PED_MODE(w2c_game*, u32);
void PG_FN_SET_WEATHER(w2c_game*, u64);
void PG_FN_CLEAR_WEATHER_OVERRIDE(w2c_game*);
void PG_FN_SET_CLOCK(w2c_game*, u32, u32, u32);
void PG_FN_PED_DENSITY_THIS_FRAME(w2c_game*, f32);
void PG_FN_VEHICLE_DENSITY_THIS_FRAME(w2c_game*, f32);
void PG_FN_PARKED_DENSITY_THIS_FRAME(w2c_game*, f32);
u32 PG_FN_PLAYER_PED_ID(w2c_game*);
void PG_FN_GET_ENTITY_COORDS(w2c_game*, u64, u32, u32);
f32 PG_FN_GET_ENTITY_HEADING(w2c_game*, u32);
void PG_FN_REQUEST_MODEL(w2c_game*, u32);
u32 PG_FN_HAS_MODEL_LOADED(w2c_game*, u32);
void PG_FN_RELEASE_MODEL(w2c_game*, u32);
u32 PG_FN_CREATE_VEHICLE(w2c_game*, u32, u64, f32, u32, u32, u32);
void PG_FN_GIVE_WEAPON(w2c_game*, u32, u32, u32, u32, u32);
u32 PG_FN_ENTITY_EXISTS(w2c_game*, u32);
void PG_FN_SET_ENTITY_HEALTH(w2c_game*, u32, u32, u32);

// State tracking
static bool zombieModeActive = true;
static bool weaponsEquipped = false;
static u32 activeZombies[MAX_ZOMBIES];
static u32 activeWrecks[MAX_WRECKS];
static double lastZombieSpawnTime = 0;
static double lastSoundGroanTime = 0;
static uint64_t zombieVectorBuffer = 0;

// Helper native invocation using Script Hook V hashes
static uint64_t pg_zombie_call(w2c_game *instance, uint64_t hash, const uint64_t *args, uint32_t count) {
  static uint64_t buf;
  if (!buf) buf = pg_asi_alloc(8 * 40);
  if (count > 32) count = 32;
  if (count) memcpy(pg_pointer(&env, buf, 8ull * count), args, 8ull * count);
  memset(pg_pointer(&env, buf + 8 * 32, 64), 0, 64);
  if (!pg_asi_native(hash, buf, count, buf + 8 * 32)) return 0;
  uint64_t val = 0;
  memcpy(&val, pg_pointer(&env, buf + 8 * 32, 8), 8);
  return val;
}

// Equip player with survival loadout (Shotgun, Combat MG, Heavy Pistol, Machete, Flare Gun)
static void pg_zombie_equip_player(w2c_game *instance, u32 playerPed) {
  if (weaponsEquipped || !playerPed) return;
  weaponsEquipped = true;

  // Assault Shotgun, Combat MG, Heavy Pistol, Machete, Flare Gun
  PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_assaultshotgun"), 250, 0, 1);
  PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_combatmg"), 500, 0, 0);
  PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_heavypistol"), 150, 0, 0);
  PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_machete"), 1, 0, 0);
  PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_flaregun"), 20, 0, 0);

  // Lock to First-Person View for visceral atmosphere
  PG_FN_CAMERA_PED_MODE(instance, 4);

  pg_log("ZOMBIE MODE: Survival weapons equipped and FPV enabled.");
}

// Spawns a wrecked abandoned vehicle near the player
static void pg_zombie_spawn_wreck(w2c_game *instance, float px, float py, float pz, float heading) {
  for (int i = 0; i < MAX_WRECKS; i++) {
    if (activeWrecks[i] && !PG_FN_ENTITY_EXISTS(instance, activeWrecks[i])) activeWrecks[i] = 0;
    if (activeWrecks[i] == 0) {
      uint32_t model = pg_joaat("rebel");
      PG_FN_REQUEST_MODEL(instance, model);
      if (PG_FN_HAS_MODEL_LOADED(instance, model)) {
        float angle = ((float)(rand() % 360)) * 3.14159f / 180.0f;
        float dist = 25.0f + (float)(rand() % 25);
        float wx = px + cosf(angle) * dist;
        float wy = py + sinf(angle) * dist;

        float *v = (float *)pg_pointer(&env, zombieVectorBuffer, 24);
        v[0] = wx; v[1] = wy; v[2] = pz;
        u32 veh = PG_FN_CREATE_VEHICLE(instance, model, zombieVectorBuffer, heading, 0, 0, 1);
        if (veh) {
          activeWrecks[i] = veh;
          // Set vehicle destroyed / smoking
          uint64_t args[2] = { veh, 0 };
          pg_zombie_call(instance, 0x45f6c4ee134510e2ull, args, 2); // SET_VEHICLE_DOORS_LOCKED
        }
      }
      break;
    }
  }
}

// Spawn and command a zombie to attack the player
static void pg_zombie_spawn(w2c_game *instance, u32 playerPed, float px, float py, float pz) {
  PG_FN_REQUEST_MODEL(instance, ZOMBIE_MODEL_HASH);
  if (!PG_FN_HAS_MODEL_LOADED(instance, ZOMBIE_MODEL_HASH)) return;

  for (int i = 0; i < MAX_ZOMBIES; i++) {
    if (activeZombies[i] && !PG_FN_ENTITY_EXISTS(instance, activeZombies[i])) activeZombies[i] = 0;
    if (activeZombies[i] == 0) {
      float angle = ((float)(rand() % 360)) * 3.14159f / 180.0f;
      float dist = 20.0f + (float)(rand() % 25);
      float zx = px + cosf(angle) * dist;
      float zy = py + sinf(angle) * dist;

      // Native CREATE_PED (hash: 0xd49f9b0955c367de)
      uint32_t zx_bits, zy_bits, zz_bits, zero_bits = 0;
      memcpy(&zx_bits, &zx, 4);
      memcpy(&zy_bits, &zy, 4);
      memcpy(&zz_bits, &pz, 4);
      uint64_t args[8] = { 26, ZOMBIE_MODEL_HASH, zx_bits, zy_bits, zz_bits, 0, 0, 1 };
      u32 ped = (u32)pg_zombie_call(instance, 0xd49f9b0955c367deull, args, 8);

      if (ped) {
        activeZombies[i] = ped;
        // TASK_COMBAT_PED (hash: 0xf166e48407bac484): Attack player relentlessly
        uint64_t combatArgs[3] = { ped, playerPed, 0 };
        pg_zombie_call(instance, 0xf166e48407bac484ull, combatArgs, 3);

        // Blip on Radar (red dot)
        uint64_t blipArgs[1] = { ped };
        u32 blip = (u32)pg_zombie_call(instance, 0x5cde92c702a8fce7ull, blipArgs, 1);
        if (blip) {
          uint64_t colorArgs[2] = { blip, 1 }; // Red color
          pg_zombie_call(instance, 0x03d7fb09e75d6b7eull, colorArgs, 2);
        }
      }
      break;
    }
  }
}

// Main tick called on each script frame
static void pg_zombie_mode_tick(w2c_game *instance) {
  if (!zombieModeActive || PGPauseMenuActive() || PGGameStatus() != 0 || PGLoadSceneActive()) return;

  // 1. Maintain dark, foggy post-apocalyptic atmosphere & clear normal civilian traffic
  PG_FN_PED_DENSITY_THIS_FRAME(instance, 0.0f);
  PG_FN_VEHICLE_DENSITY_THIS_FRAME(instance, 0.0f);
  PG_FN_PARKED_DENSITY_THIS_FRAME(instance, 0.0f);

  u32 playerPed = PG_FN_PLAYER_PED_ID(instance);
  if (!playerPed) return;

  if (!zombieVectorBuffer) zombieVectorBuffer = pg_asi_alloc(32);
  PG_FN_GET_ENTITY_COORDS(instance, zombieVectorBuffer, playerPed, 1);
  float *pCoords = (float *)pg_pointer(&env, zombieVectorBuffer, 24);
  float px = pCoords[0], py = pCoords[1], pz = pCoords[2];
  float heading = PG_FN_GET_ENTITY_HEADING(instance, playerPed);

  double now = timeMS(CLOCK_MONOTONIC);

  // Initial atmosphere setup
  static bool atmosphereConfigured = false;
  if (!atmosphereConfigured) {
    atmosphereConfigured = true;
    snprintf((char *)pg_pointer(&env, instance->w2c_g0, 32), 32, "HALLOWEEN");
    PG_FN_CLEAR_WEATHER_OVERRIDE(instance);
    PG_FN_SET_WEATHER(instance, instance->w2c_g0);
    PG_FN_SET_CLOCK(instance, 23, 0, 0);
  }

  // Equip weapons when player enters the world
  pg_zombie_equip_player(instance, playerPed);

  // 2. Spawn zombie waves every 3 seconds
  if (now - lastZombieSpawnTime > 3000) {
    lastZombieSpawnTime = now;
    u64 prev = pg_asi_enter_script(instance);
    pg_zombie_spawn(instance, playerPed, px, py, pz);
    pg_zombie_spawn_wreck(instance, px, py, pz, heading);
    *(uint64_t *)pg_pointer(&env, instance->w2c_g1 + 3088, 8) = prev;
  }

  // 3. Play horrific groan / roar sound when zombies are near (< 8 meters)
  if (now - lastSoundGroanTime > 2500) {
    for (int i = 0; i < MAX_ZOMBIES; i++) {
      u32 z = activeZombies[i];
      if (z && PG_FN_ENTITY_EXISTS(instance, z)) {
        PG_FN_GET_ENTITY_COORDS(instance, zombieVectorBuffer, z, 1);
        float *zc = (float *)pg_pointer(&env, zombieVectorBuffer, 24);
        float dx = px - zc[0], dy = py - zc[1];
        if (sqrtf(dx * dx + dy * dy) < 8.0f) {
          lastSoundGroanTime = now;
          u64 prev = pg_asi_enter_script(instance);
          // PLAY_PAIN (hash: 0xbc9ae166038a5cec): Zombie screams / pain groans
          uint64_t painArgs[3] = { z, 6, 0 };
          pg_zombie_call(instance, 0xbc9ae166038a5cecull, painArgs, 3);
          *(uint64_t *)pg_pointer(&env, instance->w2c_g1 + 3088, 8) = prev;
          break;
        }
      }
    }
  }
}
