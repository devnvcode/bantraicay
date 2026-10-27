// ============================================================================
// CHẾ ĐỘ ĐẠI DỊCH ZOMBIE SINH TỒN HOÀN CHỈNH - V3 SIÊU CHÂN THẬT
// Sửa: Triệt tiêu 100% cảnh sát truy nã, dọn dẹp xe NPC, spawn xác xe cháy ngổn ngang,
// sửa triệt để lỗi không thấy zombie (spawn trực diện + lây nhiễm toàn bộ NPC xung quanh).
// ============================================================================
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#define MAX_ZOMBIES 18
#define MAX_WRECKS 12

// Danh sách các model Ped phổ biến có sẵn 100% trong game
static const uint32_t commonZombieModels[] = {
  0x19DFA98E, // a_m_y_skater_01
  0x3C2A8647, // a_m_m_bevhills_01
  0x4990C768, // a_m_y_beach_01
  0x20557434, // g_m_y_ballasout_01
  0x401BC182  // s_m_y_dealer_01
};

static const char *const wreckModelNames[] = {
  "rebel", "stanier", "voodoo2", "ingot", "emperor2"
};

// Khai báo engine functions
void PG_FN_CAMERA_PED_MODE(w2c_game*, u32);
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
u32 PG_FN_CREATE_VEHICLE(w2c_game*, u32, u64, f32, u32, u32, u32);
void PG_FN_VEHICLE_ENGINE_HEALTH(w2c_game*, u32, f32);
void PG_FN_GIVE_WEAPON(w2c_game*, u32, u32, u32, u32, u32);
u32 PG_FN_ENTITY_EXISTS(w2c_game*, u32);
void PG_FN_SET_ENTITY_HEALTH(w2c_game*, u32, u32, u32);

// Trạng thái hệ thống
static bool zombieModeActive = true;
static bool weaponsEquipped = false;
static bool extractionPointCreated = false;
static bool atmosphereConfigured = false;
static int warmupFrames = 0;
static u32 activeZombies[MAX_ZOMBIES];
static u32 activeWrecks[MAX_WRECKS];
static double lastZombieSpawnTime = 0;
static double lastWreckSpawnTime = 0;
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

// Triệt tiêu 100% cảnh sát truy nã & cấm cảnh sát phản ứng
static void pg_zombie_disable_police(w2c_game *instance) {
  u32 player = PG_FN_PLAYER_ID(instance);
  if (player < 32) {
    PG_FN_SET_MAX_WANTED(instance, 0);
    if (PG_FN_GET_WANTED(instance, player) != 0) {
      PG_FN_CLEAR_WANTED(instance, player);
    }
    // SET_POLICE_IGNORE_PLAYER(player, 1)
    uint64_t pArgs[2] = { player, 1 };
    pg_zombie_call(instance, 0x32c62aa929c2da6aull, pArgs, 2);
    // SET_DISPATCH_COPS_FOR_PLAYER(player, 0)
    uint64_t dArgs[2] = { player, 0 };
    pg_zombie_call(instance, 0xdb172424876553f4ull, dArgs, 2);
  }
}

// Cấp vũ khí sinh tồn & góc nhìn FPV
static void pg_zombie_equip_player(w2c_game *instance, u32 playerPed) {
  if (weaponsEquipped || !playerPed) return;
  weaponsEquipped = true;

  PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_assaultshotgun"), 300, 0, 1);
  PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_combatmg"), 600, 0, 0);
  PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_heavypistol"), 200, 0, 0);
  PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_machete"), 1, 0, 0);
  PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_flaregun"), 30, 0, 0);

  PG_FN_CAMERA_PED_MODE(instance, 4);

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

  pg_log("ZOMBIE MODE: Weapons and FPV initialized.");
}

// Spawn xe nát, cháy, nổ lốp, vỡ kính ngổn ngang chặn đường
static void pg_zombie_spawn_wreck(w2c_game *instance, float px, float py, float pz, float heading) {
  uint32_t chosenModel = 0;
  for (int m = 0; m < 5; m++) {
    uint32_t h = pg_joaat(wreckModelNames[m]);
    if (PG_FN_HAS_MODEL_LOADED(instance, h)) { chosenModel = h; break; }
  }
  if (!chosenModel) return;

  for (int i = 0; i < MAX_WRECKS; i++) {
    if (activeWrecks[i] && !PG_FN_ENTITY_EXISTS(instance, activeWrecks[i])) activeWrecks[i] = 0;
    if (activeWrecks[i] == 0) {
      // Spawn trực diện phía trước mặt người chơi (từ 15m đến 35m)
      float rad = heading * 0.0174532925f;
      float spread = ((float)(rand() % 80) - 40.0f) * 0.0174532925f;
      float finalAngle = rad + spread;
      float dist = 15.0f + (float)(rand() % 20);
      float wx = px - sinf(finalAngle) * dist;
      float wy = py + cosf(finalAngle) * dist;

      float *v = (float *)pg_pointer(&env, zombieScratchMem, 24);
      v[0] = wx; v[1] = wy; v[2] = pz;
      u32 veh = PG_FN_CREATE_VEHICLE(instance, chosenModel, zombieScratchMem, heading + (float)(rand() % 90), 0, 0, 1);
      if (veh) {
        activeWrecks[i] = veh;
        // Hỏng động cơ, khói bốc cháy
        PG_FN_VEHICLE_ENGINE_HEALTH(instance, veh, -4000.0f);
        // Khóa cửa xe bỏ hoang
        uint64_t lockArgs[2] = { veh, 2 };
        pg_zombie_call(instance, 0x45f6c4ee134510e2ull, lockArgs, 2); // SET_VEHICLE_DOORS_LOCKED
        // Xe bám đầy bùn đất hoang phế (15.0f dirt)
        uint32_t dirt_bits = 0x41700000;
        uint64_t dirtArgs[2] = { veh, dirt_bits };
        pg_zombie_call(instance, 0x79d3b596fe44ee8bull, dirtArgs, 2); // SET_VEHICLE_DIRT_LEVEL
        // Nổ xẹp lốp xe
        uint32_t speed_bits = 0x447a0000;
        uint64_t flatArgs[4] = { veh, 0, 1, speed_bits };
        pg_zombie_call(instance, 0xec6a202ee4960385ull, flatArgs, 4); // SET_VEHICLE_TYRE_BURST
        uint64_t flatArgs2[4] = { veh, 1, 1, speed_bits };
        pg_zombie_call(instance, 0xec6a202ee4960385ull, flatArgs2, 4);
        // Vỡ kính chắn gió
        uint64_t smashArgs[3] = { veh, 0, 1 };
        pg_zombie_call(instance, 0x9e5b5e4d2ccd2259ull, smashArgs, 3); // SMASH_VEHICLE_WINDOW
      }
      break;
    }
  }
}

// Spawn Zombie trực diện trước mắt người chơi
static void pg_zombie_spawn(w2c_game *instance, u32 playerPed, float px, float py, float pz, float heading) {
  // Tìm model nào trong danh sách đã nạp xong vào RAM
  uint32_t chosenModel = 0;
  for (int m = 0; m < 5; m++) {
    if (PG_FN_HAS_MODEL_LOADED(instance, commonZombieModels[m])) {
      chosenModel = commonZombieModels[m];
      break;
    }
  }
  if (!chosenModel) return;

  for (int i = 0; i < MAX_ZOMBIES; i++) {
    if (activeZombies[i] && !PG_FN_ENTITY_EXISTS(instance, activeZombies[i])) activeZombies[i] = 0;
    if (activeZombies[i] == 0) {
      // VỊ TRÍ CHÍNH DIỆN: Spawn ở góc nhìn phía trước người chơi (-45 đến +45 độ)
      float rad = heading * 0.0174532925f;
      float spread = ((float)(rand() % 90) - 45.0f) * 0.0174532925f;
      float finalAngle = rad + spread;
      float dist = 12.0f + (float)(rand() % 16); // Cách 12m đến 28m
      float zx = px - sinf(finalAngle) * dist;
      float zy = py + cosf(finalAngle) * dist;
      float zz = pz + 0.3f;

      uint32_t zx_bits, zy_bits, zz_bits, head_bits;
      float zHead = heading + 180.0f;
      memcpy(&zx_bits, &zx, 4); memcpy(&zy_bits, &zy, 4); memcpy(&zz_bits, &zz, 4); memcpy(&head_bits, &zHead, 4);
      uint64_t args[8] = { 26, chosenModel, zx_bits, zy_bits, zz_bits, head_bits, 0, 1 };
      u32 ped = (u32)pg_zombie_call(instance, 0xd49f9b0955c367deull, args, 8); // CREATE_PED

      if (ped) {
        activeZombies[i] = ped;

        // Bắt zombie tấn công cận chiến điên cuồng người chơi
        uint64_t combatArgs[3] = { ped, playerPed, 0 };
        pg_zombie_call(instance, 0xf166e48407bac484ull, combatArgs, 3); // TASK_COMBAT_PED

        // Tốc độ chạy nước rút lao vào người chơi (Sprinter)
        float speed = 1.35f;
        uint32_t s_bits; memcpy(&s_bits, &speed, 4);
        uint64_t speedArgs[2] = { ped, s_bits };
        pg_zombie_call(instance, 0x085bf80fa50a39d1ull, speedArgs, 2); // SET_PED_MOVE_RATE_OVERRIDE

        // Đánh dấu chấm đỏ zombie trên bản đồ radar
        uint64_t blipArgs[1] = { ped };
        u32 blip = (u32)pg_zombie_call(instance, 0x5cde92c702a8fce7ull, blipArgs, 1);
        if (blip) {
          uint64_t colorArgs[2] = { blip, 1 }; // Màu đỏ
          pg_zombie_call(instance, 0x03d7fb09e75d6b7eull, colorArgs, 2);
        }
      }
      break;
    }
  }
}

// Lây nhiễm và biến đổi mọi NPC xung quanh thành Zombie
static void pg_zombie_infect_nearby(w2c_game *instance, u32 playerPed) {
  uint64_t pedBuffer = zombieScratchMem + 128;
  *(int32_t *)pg_pointer(&env, pedBuffer, 4) = 16;
  uint64_t scanArgs[2] = { playerPed, pedBuffer };
  pg_zombie_call(instance, 0x23f8f5fc7e8c4a6bull, scanArgs, 2); // GET_PED_NEARBY_PEDS

  int32_t count = *(int32_t *)pg_pointer(&env, pedBuffer, 4);
  for (int p = 0; p < count && p < 16; p++) {
    u32 nearPed = *(u32 *)pg_pointer(&env, pedBuffer + 4 + p * 4, 4);
    if (nearPed && nearPed != playerPed && PG_FN_ENTITY_EXISTS(instance, nearPed)) {
      bool existsInList = false;
      for (int k = 0; k < MAX_ZOMBIES; k++) {
        if (activeZombies[k] == nearPed) { existsInList = true; break; }
      }
      if (!existsInList) {
        for (int k = 0; k < MAX_ZOMBIES; k++) {
          if (activeZombies[k] == 0) {
            activeZombies[k] = nearPed;
            // Ép NPC lao vào tấn công cắn người chơi
            uint64_t combatArgs[3] = { nearPed, playerPed, 0 };
            pg_zombie_call(instance, 0xf166e48407bac484ull, combatArgs, 3);
            float speed = 1.30f; uint32_t s_bits; memcpy(&s_bits, &speed, 4);
            uint64_t speedArgs[2] = { nearPed, s_bits };
            pg_zombie_call(instance, 0x085bf80fa50a39d1ull, speedArgs, 2);
            // Gắn chấm đỏ radar
            uint64_t blipArgs[1] = { nearPed };
            u32 blip = (u32)pg_zombie_call(instance, 0x5cde92c702a8fce7ull, blipArgs, 1);
            if (blip) {
              uint64_t colorArgs[2] = { blip, 1 };
              pg_zombie_call(instance, 0x03d7fb09e75d6b7eull, colorArgs, 2);
            }
            break;
          }
        }
      }
    }
  }
}

// Vòng lặp chính mỗi khung hình
static void pg_zombie_mode_tick(w2c_game *instance) {
  if (!zombieModeActive || PGPauseMenuActive() || PGLoadSceneActive()) return;

  // Bảo vệ an toàn tuyệt đối: Không chạy khi chưa có player, cutscene, hoặc pause
  if (PGGameStatus() & (16 | 4 | 2 | 32)) return;

  u32 playerPed = PG_FN_PLAYER_PED_ID(instance);
  if (!playerPed || !PG_FN_ENTITY_EXISTS(instance, playerPed)) return;

  // Đợi 10 giây sau khi vào màn chơi để load ổn định
  if (++warmupFrames < 300) return;

  if (!pg_asi_game) pg_asi_game = instance;
  if (!zombieScratchMem) zombieScratchMem = PG_FN_OPERATOR_NEW(instance, 512);
  if (!zombieScratchMem) return;

  u64 prev = pg_asi_enter_script(instance);

  // 1. LIÊN TỤC YÊU CẦU NẠP MODEL MỖI FRAME (Chìa khóa để model nạp thành công 100%)
  for (int m = 0; m < 5; m++) {
    PG_FN_REQUEST_MODEL(instance, commonZombieModels[m]);
    PG_FN_REQUEST_MODEL(instance, pg_joaat(wreckModelNames[m]));
  }

  // 2. TRIỆT TIÊU HOÀN TOÀN CẢNH SÁT TRUY NÃ
  pg_zombie_disable_police(instance);

  // 3. TRIỆT TIÊU HOÀN TOÀN XE CỘ NPC LƯU THÔNG
  PG_FN_PED_DENSITY_THIS_FRAME(instance, 0.0f);
  PG_FN_VEHICLE_DENSITY_THIS_FRAME(instance, 0.0f);
  PG_FN_PARKED_DENSITY_THIS_FRAME(instance, 0.0f);
  uint32_t zero_f = 0;
  uint64_t zeroArg[1] = { zero_f };
  pg_zombie_call(instance, 0xb3b3359379fe77d3ull, zeroArg, 1); // SET_RANDOM_VEHICLE_DENSITY_MULTIPLIER_THIS_FRAME
  pg_zombie_call(instance, 0x90b6da738a9a25daull, zeroArg, 1); // SET_AMBIENT_VEHICLE_RANGE_MULTIPLIER_THIS_FRAME

  PG_FN_GET_ENTITY_COORDS(instance, zombieScratchMem, playerPed, 1);
  float *pCoords = (float *)pg_pointer(&env, zombieScratchMem, 24);
  float px = pCoords[0], py = pCoords[1], pz = pCoords[2];
  float heading = PG_FN_GET_ENTITY_HEADING(instance, playerPed);

  double now = timeMS(CLOCK_MONOTONIC);

  // 4. Bầu không khí Halloween rùng rợn & ban đêm u ám
  if (!atmosphereConfigured) {
    atmosphereConfigured = true;
    snprintf((char *)pg_pointer(&env, zombieScratchMem + 64, 32), 32, "HALLOWEEN");
    PG_FN_CLEAR_WEATHER_OVERRIDE(instance);
    PG_FN_SET_WEATHER(instance, zombieScratchMem + 64);
    PG_FN_SET_CLOCK(instance, 23, 0, 0);
  }

  pg_zombie_equip_player(instance, playerPed);

  // 5. Spawn Zombie chính diện & spawn xe nát
  if (now - lastZombieSpawnTime > 2000) {
    lastZombieSpawnTime = now;
    pg_zombie_spawn(instance, playerPed, px, py, pz, heading);
  }
  if (now - lastWreckSpawnTime > 3500) {
    lastWreckSpawnTime = now;
    pg_zombie_spawn_wreck(instance, px, py, pz, heading);
  }

  // 6. Lây nhiễm toàn bộ NPC có sẵn xung quanh thành Zombie
  pg_zombie_infect_nearby(instance, playerPed);

  // 7. Âm thanh gầm thét rợn người và zombie cắn trừ máu
  for (int i = 0; i < MAX_ZOMBIES; i++) {
    u32 z = activeZombies[i];
    if (z && PG_FN_ENTITY_EXISTS(instance, z)) {
      PG_FN_GET_ENTITY_COORDS(instance, zombieScratchMem, z, 1);
      float *zc = (float *)pg_pointer(&env, zombieScratchMem, 24);
      float dx = px - zc[0], dy = py - zc[1];
      float distance = sqrtf(dx * dx + dy * dy);

      if (distance > 75.0f) {
        activeZombies[i] = 0;
        continue;
      }

      // Zombie ở cự ly dưới 8 mét: Gầm rú rùng rợn
      if (distance < 8.0f && (now - lastSoundGroanTime > 2000)) {
        lastSoundGroanTime = now;
        uint64_t painArgs[3] = { z, 6, 0 };
        pg_zombie_call(instance, 0xbc9ae166038a5cecull, painArgs, 3);
      }

      // Zombie áp sát dưới 1.6 mét: Cắn trừ 14 HP
      if (distance < 1.6f && (now - lastBiteDamageTime > 1100)) {
        lastBiteDamageTime = now;
        uint64_t hArgs[1] = { playerPed };
        u32 currentHealth = (u32)pg_zombie_call(instance, 0xeef059fad016d209ull, hArgs, 1);
        if (currentHealth > 100) {
          PG_FN_SET_ENTITY_HEALTH(instance, playerPed, currentHealth - 14, 0);
          uint64_t painArgs[3] = { playerPed, 8, 0 };
          pg_zombie_call(instance, 0xbc9ae166038a5cecull, painArgs, 3);
        }
      }
    }
  }

  pg_asi_leave_script(instance, prev);
}
