// ============================================================================
// CHẾ ĐỘ ĐẠI DỊCH ZOMBIE SINH TỒN NÂNG CẤP (ZOMBIE APOCALYPSE SURVIVAL)
// Hỗ trợ: Sương mù Halloween, xe cháy rụi, Zombie Sprinters & Walkers,
// cơ chế cắn trừ máu, tiếng thét rùng rợn, FPV và Điểm sơ tán trực thăng LSIA.
// ============================================================================
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#define MAX_ZOMBIES 16
#define MAX_WRECKS 6

// Danh sách các model Zombie đa dạng (Vinewood Zombie, Dân dị dạng, Bác sĩ biến dị)
static const uint32_t zombieModels[] = {
  0xAC36843C, // u_m_y_zombie_01 (Zombie chính gốc GTA V)
  0xF49A3219, // a_m_m_hillbilly_01 (Dân điên máu me)
  0x6C9B2849, // s_m_m_paramedic_01 (Y tá nhiễm virus)
  0xB5FE0435  // s_m_y_clown_01 (Hề kinh dị biến dị)
};

// Khai báo hàm engine gốc
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

// Quản lý trạng thái
static bool zombieModeActive = true;
static bool weaponsEquipped = false;
static bool extractionPointCreated = false;
static u32 activeZombies[MAX_ZOMBIES];
static u32 activeWrecks[MAX_WRECKS];
static double lastZombieSpawnTime = 0;
static double lastSoundGroanTime = 0;
static double lastBiteDamageTime = 0;
static uint64_t zombieVectorBuffer = 0;

// Gọi native engine qua Script Hook V hash
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

// Cấp kho vũ khí sinh tồn cực mạnh & góc nhìn FPV
static void pg_zombie_equip_player(w2c_game *instance, u32 playerPed) {
  if (weaponsEquipped || !playerPed) return;
  weaponsEquipped = true;

  // Cấp vũ khí sinh tồn: Assault Shotgun, Combat MG, Heavy Pistol, Machete, Súng pháo sáng
  PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_assaultshotgun"), 300, 0, 1);
  PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_combatmg"), 600, 0, 0);
  PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_heavypistol"), 200, 0, 0);
  PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_machete"), 1, 0, 0);
  PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_flaregun"), 30, 0, 0);

  // Khóa góc nhìn thứ nhất (First-Person View)
  PG_FN_CAMERA_PED_MODE(instance, 4);

  // Tạo điểm sơ tán cứu hộ (Helicopter Extraction Point tại Sân bay Los Santos LSIA)
  if (!extractionPointCreated) {
    extractionPointCreated = true;
    float ex = -1150.0f, ey = -2715.0f, ez = 19.8f;
    uint32_t bx, by, bz;
    memcpy(&bx, &ex, 4); memcpy(&by, &ey, 4); memcpy(&bz, &ez, 4);
    uint64_t bArgs[3] = { bx, by, bz };
    u32 blip = (u32)pg_zombie_call(instance, 0x5a039bb0bca604b6ull, bArgs, 3); // ADD_BLIP_FOR_COORD
    if (blip) {
      uint64_t sArgs[2] = { blip, 64 }; // Icon trực thăng cứu hộ
      pg_zombie_call(instance, 0xdf735600a4696dafull, sArgs, 2); // SET_BLIP_SPRITE
      uint64_t cArgs[2] = { blip, 2 }; // Màu xanh lá cứu hộ
      pg_zombie_call(instance, 0x03d7fb09e75d6b7eull, cArgs, 2); // SET_BLIP_COLOUR
    }
  }

  pg_log("ZOMBIE MODE: Weapons equipped, FPV set, Extraction Point marked at LSIA!");
}

// Spawn xe cháy bỏ hoang trên đường
static void pg_zombie_spawn_wreck(w2c_game *instance, float px, float py, float pz, float heading) {
  for (int i = 0; i < MAX_WRECKS; i++) {
    if (activeWrecks[i] && !PG_FN_ENTITY_EXISTS(instance, activeWrecks[i])) activeWrecks[i] = 0;
    if (activeWrecks[i] == 0) {
      uint32_t model = pg_joaat("rebel");
      PG_FN_REQUEST_MODEL(instance, model);
      if (PG_FN_HAS_MODEL_LOADED(instance, model)) {
        float angle = ((float)(rand() % 360)) * 3.14159f / 180.0f;
        float dist = 25.0f + (float)(rand() % 30);
        float wx = px + cosf(angle) * dist;
        float wy = py + sinf(angle) * dist;

        float *v = (float *)pg_pointer(&env, zombieVectorBuffer, 24);
        v[0] = wx; v[1] = wy; v[2] = pz;
        u32 veh = PG_FN_CREATE_VEHICLE(instance, model, zombieVectorBuffer, heading, 0, 0, 1);
        if (veh) {
          activeWrecks[i] = veh;
          uint64_t args[2] = { veh, 0 };
          pg_zombie_call(instance, 0x45f6c4ee134510e2ull, args, 2); // SET_VEHICLE_DOORS_LOCKED
        }
      }
      break;
    }
  }
}

// Spawn zombie (bao gồm cả zombie thường và zombie chạy nhanh - Sprinters)
static void pg_zombie_spawn(w2c_game *instance, u32 playerPed, float px, float py, float pz) {
  int idx = rand() % (sizeof(zombieModels) / sizeof(zombieModels[0]));
  uint32_t chosenModel = zombieModels[idx];

  PG_FN_REQUEST_MODEL(instance, chosenModel);
  if (!PG_FN_HAS_MODEL_LOADED(instance, chosenModel)) return;

  for (int i = 0; i < MAX_ZOMBIES; i++) {
    if (activeZombies[i] && !PG_FN_ENTITY_EXISTS(instance, activeZombies[i])) activeZombies[i] = 0;
    if (activeZombies[i] == 0) {
      float angle = ((float)(rand() % 360)) * 3.14159f / 180.0f;
      float dist = 18.0f + (float)(rand() % 22);
      float zx = px + cosf(angle) * dist;
      float zy = py + sinf(angle) * dist;

      uint32_t zx_bits, zy_bits, zz_bits;
      memcpy(&zx_bits, &zx, 4); memcpy(&zy_bits, &zy, 4); memcpy(&zz_bits, &pz, 4);
      uint64_t args[8] = { 26, chosenModel, zx_bits, zy_bits, zz_bits, 0, 0, 1 };
      u32 ped = (u32)pg_zombie_call(instance, 0xd49f9b0955c367deull, args, 8); // CREATE_PED

      if (ped) {
        activeZombies[i] = ped;

        // Tấn công điên cuồng người chơi
        uint64_t combatArgs[3] = { ped, playerPed, 0 };
        pg_zombie_call(instance, 0xf166e48407bac484ull, combatArgs, 3); // TASK_COMBAT_PED

        // Tỷ lệ 40% zombie chạy nhanh (Sprinter) cực nguy hiểm
        if ((rand() % 10) < 4) {
          float speed = 1.35f;
          uint32_t s_bits; memcpy(&s_bits, &speed, 4);
          uint64_t speedArgs[2] = { ped, s_bits };
          pg_zombie_call(instance, 0x085bf80fa50a39d1ull, speedArgs, 2); // SET_PED_MOVE_RATE_OVERRIDE
        }

        // Đánh dấu chấm đỏ zombie trên radar
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

// Hàm tick thực thi mỗi khung hình trong vòng lặp game
static void pg_zombie_mode_tick(w2c_game *instance) {
  if (!zombieModeActive || PGPauseMenuActive() || PGGameStatus() != 0 || PGLoadSceneActive()) return;

  // 1. Triệt tiêu dân thường và xe cộ lưu thông thông thường
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

  // 2. Khóa bầu không khí Halloween rùng rợn & ban đêm u ám
  static bool atmosphereConfigured = false;
  if (!atmosphereConfigured) {
    atmosphereConfigured = true;
    snprintf((char *)pg_pointer(&env, instance->w2c_g0, 32), 32, "HALLOWEEN");
    PG_FN_CLEAR_WEATHER_OVERRIDE(instance);
    PG_FN_SET_WEATHER(instance, instance->w2c_g0);
    PG_FN_SET_CLOCK(instance, 23, 0, 0); // 11 giờ đêm
  }

  // Cấp kho vũ khí sinh tồn & FPV
  pg_zombie_equip_player(instance, playerPed);

  // 3. Spawn đợt zombie và xe đổ nát mỗi 2.5 giây
  if (now - lastZombieSpawnTime > 2500) {
    lastZombieSpawnTime = now;
    u64 prev = pg_asi_enter_script(instance);
    pg_zombie_spawn(instance, playerPed, px, py, pz);
    pg_zombie_spawn_wreck(instance, px, py, pz, heading);
    *(uint64_t *)pg_pointer(&env, instance->w2c_g1 + 3088, 8) = prev;
  }

  // 4. Xử lý âm thanh gầm thét và cơ chế zombie cắn gây sát thương khi áp sát
  for (int i = 0; i < MAX_ZOMBIES; i++) {
    u32 z = activeZombies[i];
    if (z && PG_FN_ENTITY_EXISTS(instance, z)) {
      PG_FN_GET_ENTITY_COORDS(instance, zombieVectorBuffer, z, 1);
      float *zc = (float *)pg_pointer(&env, zombieVectorBuffer, 24);
      float dx = px - zc[0], dy = py - zc[1];
      float distance = sqrtf(dx * dx + dy * dy);

      // Nếu zombie ở quá xa (> 85m), tự dọn dẹp để tiết kiệm RAM trên iPhone
      if (distance > 85.0f) {
        activeZombies[i] = 0;
        continue;
      }

      // Zombie áp sát dưới 7 mét: Phát tiếng gầm rú rợn người
      if (distance < 7.0f && (now - lastSoundGroanTime > 2200)) {
        lastSoundGroanTime = now;
        u64 prev = pg_asi_enter_script(instance);
        uint64_t painArgs[3] = { z, 6, 0 };
        pg_zombie_call(instance, 0xbc9ae166038a5cecull, painArgs, 3); // PLAY_PAIN
        *(uint64_t *)pg_pointer(&env, instance->w2c_g1 + 3088, 8) = prev;
      }

      // Zombie cắn người chơi khi ở cự ly cận chiến (< 1.6 mét)
      if (distance < 1.6f && (now - lastBiteDamageTime > 1200)) {
        lastBiteDamageTime = now;
        u64 prev = pg_asi_enter_script(instance);
        uint64_t hArgs[1] = { playerPed };
        u32 currentHealth = (u32)pg_zombie_call(instance, 0xeef059fad016d209ull, hArgs, 1); // GET_ENTITY_HEALTH
        if (currentHealth > 100) {
          PG_FN_SET_ENTITY_HEALTH(instance, playerPed, currentHealth - 12, 0); // Bị cắn mất 12 HP
          uint64_t painArgs[3] = { playerPed, 8, 0 };
          pg_zombie_call(instance, 0xbc9ae166038a5cecull, painArgs, 3); // Tiếng kêu đau của người chơi
        }
        *(uint64_t *)pg_pointer(&env, instance->w2c_g1 + 3088, 8) = prev;
      }
    }
  }
}
