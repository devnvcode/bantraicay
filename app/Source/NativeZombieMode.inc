// ============================================================================
// CHẾ ĐỘ ĐẠI DỊCH ZOMBIE SINH TỒN - BẢN ỔN ĐỊNH CHỐNG CRASH KHI VÀO GAME
// ============================================================================
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#define MAX_ZOMBIES 10
#define MAX_WRECKS 4
#define ZOMBIE_MODEL_HASH 0xAC36843C // u_m_y_zombie_01

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
u32 PG_FN_CREATE_VEHICLE(w2c_game*, u32, u64, f32, u32, u32, u32);
void PG_FN_GIVE_WEAPON(w2c_game*, u32, u32, u32, u32, u32);
u32 PG_FN_ENTITY_EXISTS(w2c_game*, u32);
void PG_FN_SET_ENTITY_HEALTH(w2c_game*, u32, u32, u32);

static bool zombieModeActive = true;
static bool weaponsEquipped = false;
static bool extractionPointCreated = false;
static bool atmosphereConfigured = false;
static int warmupFrames = 0;
static u32 activeZombies[MAX_ZOMBIES];
static u32 activeWrecks[MAX_WRECKS];
static double lastZombieSpawnTime = 0;
static double lastSoundGroanTime = 0;
static double lastBiteDamageTime = 0;
static uint64_t zombieScratchMem = 0;

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

static void pg_zombie_equip_player(w2c_game *instance, u32 playerPed) {
  if (weaponsEquipped || !playerPed) return;
  weaponsEquipped = true;

  // Cấp kho vũ khí sinh tồn
  PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_assaultshotgun"), 300, 0, 1);
  PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_combatmg"), 600, 0, 0);
  PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_heavypistol"), 200, 0, 0);
  PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_machete"), 1, 0, 0);
  PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_flaregun"), 30, 0, 0);

  // Kích hoạt góc nhìn thứ nhất FPV
  PG_FN_CAMERA_PED_MODE(instance, 4);

  // Tạo điểm sơ tán trực thăng tại sân bay LSIA
  if (!extractionPointCreated) {
    extractionPointCreated = true;
    float ex = -1150.0f, ey = -2715.0f, ez = 19.8f;
    uint32_t bx, by, bz;
    memcpy(&bx, &ex, 4); memcpy(&by, &ey, 4); memcpy(&bz, &ez, 4);
    uint64_t bArgs[3] = { bx, by, bz };
    u32 blip = (u32)pg_zombie_call(instance, 0x5a039bb0bca604b6ull, bArgs, 3);
    if (blip) {
      uint64_t sArgs[2] = { blip, 64 };
      pg_zombie_call(instance, 0xdf735600a4696dafull, sArgs, 2);
      uint64_t cArgs[2] = { blip, 2 };
      pg_zombie_call(instance, 0x03d7fb09e75d6b7eull, cArgs, 2);
    }
  }

  pg_log("ZOMBIE MODE: Weapons and FPV ready.");
}

static void pg_zombie_spawn_wreck(w2c_game *instance, float px, float py, float pz, float heading) {
  uint32_t model = pg_joaat("rebel");
  PG_FN_REQUEST_MODEL(instance, model);
  if (!PG_FN_HAS_MODEL_LOADED(instance, model)) return;

  for (int i = 0; i < MAX_WRECKS; i++) {
    if (activeWrecks[i] && !PG_FN_ENTITY_EXISTS(instance, activeWrecks[i])) activeWrecks[i] = 0;
    if (activeWrecks[i] == 0) {
      float angle = ((float)(rand() % 360)) * 3.14159f / 180.0f;
      float dist = 25.0f + (float)(rand() % 25);
      float wx = px + cosf(angle) * dist;
      float wy = py + sinf(angle) * dist;

      float *v = (float *)pg_pointer(&env, zombieScratchMem, 24);
      v[0] = wx; v[1] = wy; v[2] = pz;
      u32 veh = PG_FN_CREATE_VEHICLE(instance, model, zombieScratchMem, heading, 0, 0, 1);
      if (veh) {
        activeWrecks[i] = veh;
        uint64_t args[2] = { veh, 0 };
        pg_zombie_call(instance, 0x45f6c4ee134510e2ull, args, 2);
      }
      break;
    }
  }
}

static void pg_zombie_spawn(w2c_game *instance, u32 playerPed, float px, float py, float pz) {
  PG_FN_REQUEST_MODEL(instance, ZOMBIE_MODEL_HASH);
  if (!PG_FN_HAS_MODEL_LOADED(instance, ZOMBIE_MODEL_HASH)) return;

  for (int i = 0; i < MAX_ZOMBIES; i++) {
    if (activeZombies[i] && !PG_FN_ENTITY_EXISTS(instance, activeZombies[i])) activeZombies[i] = 0;
    if (activeZombies[i] == 0) {
      float angle = ((float)(rand() % 360)) * 3.14159f / 180.0f;
      float dist = 18.0f + (float)(rand() % 20);
      float zx = px + cosf(angle) * dist;
      float zy = py + sinf(angle) * dist;

      uint32_t zx_bits, zy_bits, zz_bits;
      memcpy(&zx_bits, &zx, 4); memcpy(&zy_bits, &zy, 4); memcpy(&zz_bits, &pz, 4);
      uint64_t args[8] = { 26, ZOMBIE_MODEL_HASH, zx_bits, zy_bits, zz_bits, 0, 0, 1 };
      u32 ped = (u32)pg_zombie_call(instance, 0xd49f9b0955c367deull, args, 8);

      if (ped) {
        activeZombies[i] = ped;
        uint64_t combatArgs[3] = { ped, playerPed, 0 };
        pg_zombie_call(instance, 0xf166e48407bac484ull, combatArgs, 3);

        if ((rand() % 10) < 4) {
          float speed = 1.30f;
          uint32_t s_bits; memcpy(&s_bits, &speed, 4);
          uint64_t speedArgs[2] = { ped, s_bits };
          pg_zombie_call(instance, 0x085bf80fa50a39d1ull, speedArgs, 2);
        }

        uint64_t blipArgs[1] = { ped };
        u32 blip = (u32)pg_zombie_call(instance, 0x5cde92c702a8fce7ull, blipArgs, 1);
        if (blip) {
          uint64_t colorArgs[2] = { blip, 1 };
          pg_zombie_call(instance, 0x03d7fb09e75d6b7eull, colorArgs, 2);
        }
      }
      break;
    }
  }
}

static void pg_zombie_mode_tick(w2c_game *instance) {
  if (!zombieModeActive || PGPauseMenuActive() || PGLoadSceneActive()) return;

  // QUAN TRỌNG: Chỉ chạy khi game đã nạp xong, player đã spawn và không bị khóa
  // (16: chưa có player, 4: cutscene mở đầu, 2: đang chuyển cảnh, 32: pause)
  if (PGGameStatus() & (16 | 4 | 2 | 32)) return;

  u32 playerPed = PG_FN_PLAYER_PED_ID(instance);
  if (!playerPed || !PG_FN_ENTITY_EXISTS(instance, playerPed)) return;

  // Đợi 10 giây (300 frames) sau khi vào màn chơi để cốt truyện khởi tạo xong hoàn toàn
  if (++warmupFrames < 300) return;

  if (!pg_asi_game) pg_asi_game = instance;
  if (!zombieScratchMem) zombieScratchMem = PG_FN_OPERATOR_NEW(instance, 256);
  if (!zombieScratchMem) return;

  u64 prev = pg_asi_enter_script(instance);

  // 1. Duy trì bầu không khí tận thế
  PG_FN_PED_DENSITY_THIS_FRAME(instance, 0.0f);
  PG_FN_VEHICLE_DENSITY_THIS_FRAME(instance, 0.0f);
  PG_FN_PARKED_DENSITY_THIS_FRAME(instance, 0.0f);

  PG_FN_GET_ENTITY_COORDS(instance, zombieScratchMem, playerPed, 1);
  float *pCoords = (float *)pg_pointer(&env, zombieScratchMem, 24);
  float px = pCoords[0], py = pCoords[1], pz = pCoords[2];
  float heading = PG_FN_GET_ENTITY_HEADING(instance, playerPed);

  double now = timeMS(CLOCK_MONOTONIC);

  if (!atmosphereConfigured) {
    atmosphereConfigured = true;
    snprintf((char *)pg_pointer(&env, zombieScratchMem + 64, 32), 32, "HALLOWEEN");
    PG_FN_CLEAR_WEATHER_OVERRIDE(instance);
    PG_FN_SET_WEATHER(instance, zombieScratchMem + 64);
    PG_FN_SET_CLOCK(instance, 23, 0, 0);
  }

  pg_zombie_equip_player(instance, playerPed);

  // 2. Spawn zombie và xe hỏng ngẫu nhiên
  if (now - lastZombieSpawnTime > 3000) {
    lastZombieSpawnTime = now;
    pg_zombie_spawn(instance, playerPed, px, py, pz);
    pg_zombie_spawn_wreck(instance, px, py, pz, heading);
  }

  // 3. Tương tác cận chiến & âm thanh
  for (int i = 0; i < MAX_ZOMBIES; i++) {
    u32 z = activeZombies[i];
    if (z && PG_FN_ENTITY_EXISTS(instance, z)) {
      PG_FN_GET_ENTITY_COORDS(instance, zombieScratchMem, z, 1);
      float *zc = (float *)pg_pointer(&env, zombieScratchMem, 24);
      float dx = px - zc[0], dy = py - zc[1];
      float distance = sqrtf(dx * dx + dy * dy);

      if (distance > 80.0f) {
        activeZombies[i] = 0;
        continue;
      }

      if (distance < 7.0f && (now - lastSoundGroanTime > 2500)) {
        lastSoundGroanTime = now;
        uint64_t painArgs[3] = { z, 6, 0 };
        pg_zombie_call(instance, 0xbc9ae166038a5cecull, painArgs, 3);
      }

      if (distance < 1.6f && (now - lastBiteDamageTime > 1200)) {
        lastBiteDamageTime = now;
        uint64_t hArgs[1] = { playerPed };
        u32 currentHealth = (u32)pg_zombie_call(instance, 0xeef059fad016d209ull, hArgs, 1);
        if (currentHealth > 100) {
          PG_FN_SET_ENTITY_HEALTH(instance, playerPed, currentHealth - 12, 0);
          uint64_t painArgs[3] = { playerPed, 8, 0 };
          pg_zombie_call(instance, 0xbc9ae166038a5cecull, painArgs, 3);
        }
      }
    }
  }

  pg_asi_leave_script(instance, prev);
}
