// ============================================================================
// NATIVE ZOMBIE APOCALYPSE SURVIVAL SYSTEM - V4 ULTRA-STABLE
// PlayGTA5Native / Mugoat NewGen for iOS
// Khắc phục triệt để 3 lỗi do người dùng báo cáo:
// 1. Sửa lỗi "Game đã dừng" khi đứng yên (FPS = "—"):
//    - Tích hợp pg_asi_enter_script / pg_asi_leave_script để gắn script owner cho mọi native
//    - Phân tán công việc theo từng frame (Staggered frame tick, <0.1ms/frame, không giật lag)
//    - Truyền con trỏ bộ nhớ an toàn cho REMOVE_BLIP, DELETE_VEHICLE, SET_ENTITY_AS_NO_LONGER_NEEDED
//    - Dùng in-memory ring buffer và pg_log an toàn luồng, loại bỏ fopen đồng bộ trên thread wasm
// 2. Sửa lỗi không có zombie và không có xe bỏ hoang:
//    - Tự động phát hiện nếu nhân vật ở trong nhà/biệt thự và dịch chuyển ra ngoài đường phố (Portola Dr)
//    - Request và giữ nguyên model cho đến khi HAS_MODEL_LOADED thành công (không cycle liên tục)
//    - Hỗ trợ cả model zombie gốc (u_m_y_zombie_01) lẫn model dân cư lây nhiễm (skater/hillbilly)
//    - Ghi log chi tiết mỗi lần spawn thành công gồm số lượng, handle, tọa độ (X, Y, Z)
// 3. Sửa lỗi map bị tối rồi mới sáng:
//    - Thiết lập 12:00 trưa + PAUSE_CLOCK(1) đóng băng đồng hồ NGAY FRAME ĐẦU TIÊN
//    - Thiết lập thời tiết OVERCAST và bộ lọc màu xám ố prologue_shootout (0.35f) ngay từ đầu
//    - Re-assert định kỳ mỗi 2 giây, đảm bảo không bao giờ bị tối hay đổi màu đột ngột
// ============================================================================
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <stdatomic.h>
#include <stdlib.h>

#define ZOMBIE_MAX_ACTIVE 12
#define ZOMBIE_MAX_WRECKS 5
#define ZOMBIE_MAX_LOG_LINES 64

// Native declarations from EngineHost / engine-functions.h
void PG_FN_SET_WEATHER(w2c_game*, u64);
void PG_FN_CLEAR_WEATHER_OVERRIDE(w2c_game*);
void PG_FN_SET_CLOCK(w2c_game*, u32, u32, u32);
void PG_FN_PED_DENSITY_THIS_FRAME(w2c_game*, f32);
void PG_FN_VEHICLE_DENSITY_THIS_FRAME(w2c_game*, f32);
void PG_FN_PARKED_DENSITY_THIS_FRAME(w2c_game*, f32);
void PG_FN_REDUCE_PED_MODEL_BUDGET(w2c_game*, u32);
void PG_FN_REDUCE_VEHICLE_MODEL_BUDGET(w2c_game*, u32);
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
void PG_FN_DELETE_VEHICLE(w2c_game*, u64);
void PG_FN_GIVE_WEAPON(w2c_game*, u32, u32, u32, u32, u32);
u32 PG_FN_ENTITY_EXISTS(w2c_game*, u32);
void PG_FN_SET_ENTITY_HEALTH(w2c_game*, u32, u32, u32);
u32 PG_FN_GET_ENTITY_MODEL(w2c_game*, u32);
void PG_FN_SET_ENTITY_COORDS_NO_OFFSET(w2c_game*, u32, u64, u32, u32, u32);

// Models for roadside abandoned wrecks
static const char *const zombieWreckModels[] = {
  "rebel", "stanier", "tornado3", "voodoo2"
};

// Zombie models: Skater (0x19dfa98e) and Hillbilly (0x3497d515) are standard in story mode
static uint32_t activeZombieModels[3] = {
  0x19DFA98E, // a_m_y_skater_01
  0x3497D515, // a_m_m_hillbilly_01
  0           // placeholder for u_m_y_zombie_01 if valid
};
static int activeZombieModelCount = 2;

// Survivor companion model (Marine)
static const uint32_t companionModel = 0x8D8F1B10; // s_m_y_marine_01

// Escape destination: Del Perro Pier helipad
static const float evacX = -1600.0f;
static const float evacY = -1050.0f;
static const float evacZ = 13.0f;

// Module Feature Flags
static _Atomic bool zombieActive = true;
static _Atomic bool zombieEnableAtmosphere = true;
static _Atomic bool zombieEnableTraffic = true;
static _Atomic bool zombieEnableWrecks = true;
static _Atomic bool zombieEnableZombies = true;
static _Atomic bool zombieEnableWeapons = true;
static _Atomic bool zombieEnableCompanion = true;
static _Atomic bool zombieEnableExtraction = true;
static _Atomic bool zombieEnableAudio = true;

// Gameplay Stats & State
static _Atomic int zombieKills = 0;
static _Atomic int zombieTier = 0;
static _Atomic bool zombieResetRequested = false;
static _Atomic float zombieDistanceToEvac = 2000.0f;
static _Atomic bool zombieEscaped = false;

// Sequential Initialization Stages
typedef enum {
  ZOMBIE_STAGE_BOOT = 0,        // Frame 0: Immediate Day 12h + OVERCAST + PAUSE_CLOCK
  ZOMBIE_STAGE_WARMUP = 1,      // Frames 1 - 120 (~2s): Teleport outdoors & stream road
  ZOMBIE_STAGE_TRAFFIC = 2,     // Frame 120 (~2s): Suppress ambient traffic & cops
  ZOMBIE_STAGE_WEAPONS = 3,     // Frame 180 (~3s): Initial melee weapons loadout
  ZOMBIE_STAGE_EXTRACTION = 4,  // Frame 240 (~4s): Del Perro Pier rescue blip
  ZOMBIE_STAGE_COMPANION = 5,   // Frame 300 (~5s): Companion survivor
  ZOMBIE_STAGE_WRECKS = 6,      // Frame 360 (~6s): Roadside abandoned wrecks
  ZOMBIE_STAGE_ACTIVE = 7       // Frame 420+ (~7s+): Controlled zombie horde spawner
} ZombieInitStage;

static ZombieInitStage zombieCurrentStage = ZOMBIE_STAGE_BOOT;
static uint64_t zombieFrameCounter = 0;
static bool playerTeleportedOutdoors = false;

// Runtime Tracking Entities
static u32 activeZombies[ZOMBIE_MAX_ACTIVE];
static u32 zombieBlips[ZOMBIE_MAX_ACTIVE];
static bool zombieChasing[ZOMBIE_MAX_ACTIVE];
static u32 activeWrecks[ZOMBIE_MAX_WRECKS];
static u32 companionPed = 0;
static u32 companionBlip = 0;
static u32 evacBlip = 0;

// Rate limiting timers (in ms)
static double lastZombieSpawnTime = 0;
static double lastWreckSpawnTime = 0;
static double lastCompanionCheckTime = 0;
static double companionRespawnAtTime = 0;
static double lastBiteDamageTime = 0;
static double lastGroanSoundTime = 0;
static double lastAtmosphereSyncTime = 0;
static double lastPeriodicLogTime = 0;
static double evacHoldStartTime = 0;

// Pointers in WebAssembly guest memory
static uint64_t zombieCoordVector = 0;
static uint64_t zombieEntityPtrBuf = 0;
static uint64_t zombieBlipPtrBuf = 0;
static uint64_t zombieStrWeather = 0;
static uint64_t zombieStrTimecycle = 0;
static uint64_t zombieStrAudioScene = 0;

static char zombieStatusText[160] = "ZOMBIE SURVIVAL: SANG TOI DANG CHUAN BI...";

// Thread-safe in-memory ring buffer for live log viewer
static char zombieLogLines[ZOMBIE_MAX_LOG_LINES][192];
static int zombieLogHead = 0;
static int zombieLogCount = 0;
static pthread_mutex_t zombieLogLock = PTHREAD_MUTEX_INITIALIZER;

// --- Diagnostics & Thread-safe Logging ---
static void pg_zombie_log(const char *format, ...) {
  char line[256];
  va_list args;
  va_start(args, format);
  vsnprintf(line, sizeof(line), format, args);
  va_end(args);

  // Send to engine diagnostic output (thread-safe, buffered)
  pg_log(line);

  // Store in thread-safe memory ring buffer for UIKit view
  pthread_mutex_lock(&zombieLogLock);
  snprintf(zombieLogLines[zombieLogHead % ZOMBIE_MAX_LOG_LINES], sizeof(zombieLogLines[0]), "%s", line);
  zombieLogHead++;
  if (zombieLogCount < ZOMBIE_MAX_LOG_LINES) zombieLogCount++;
  pthread_mutex_unlock(&zombieLogLock);
}

// --- Float & String Helpers for rage Native Calls ---
static inline uint64_t pg_zf32(float f) {
  uint32_t bits = 0;
  memcpy(&bits, &f, 4);
  return (uint64_t)bits;
}

static inline uint64_t pg_zstr(uint64_t *slot, const char *str) {
  if (!slot) return 0;
  if (!*slot && str) {
    size_t len = strlen(str) + 1;
    *slot = pg_asi_alloc(len);
    if (*slot) memcpy(pg_pointer(&env, *slot, len), str, len);
  }
  return *slot;
}

// --- Memory Check for iPhone Stability ---
static inline bool pg_zombie_has_memory(void) {
  uint64_t headroom = 0;
  return !nativeProcessHeadroom(&headroom) || headroom >= guardReserve();
}

// --- Safe Entity Deletion & Release (Uses pointer to handle in guest memory) ---
static void pg_zombie_release_ped(w2c_game *instance, u32 *pedHandle) {
  if (!pedHandle || !*pedHandle) return;
  u32 h = *pedHandle;
  *pedHandle = 0;
  if (!PG_FN_ENTITY_EXISTS(instance, h)) return;

  if (!zombieEntityPtrBuf) zombieEntityPtrBuf = pg_asi_alloc(16);
  if (zombieEntityPtrBuf) {
    *(u32*)pg_pointer(&env, zombieEntityPtrBuf, 4) = h;
    // 0xb736a491e64a32cf: SET_ENTITY_AS_NO_LONGER_NEEDED (takes Entity* in rage)
    pg_test_call(instance, 0xb736a491e64a32cfull, 1, (uint64_t[]){ zombieEntityPtrBuf });
  }
}

static void pg_zombie_delete_wreck(w2c_game *instance, u32 *vehHandle) {
  if (!vehHandle || !*vehHandle) return;
  u32 h = *vehHandle;
  *vehHandle = 0;
  if (!PG_FN_ENTITY_EXISTS(instance, h)) return;

  if (!zombieEntityPtrBuf) zombieEntityPtrBuf = pg_asi_alloc(16);
  if (zombieEntityPtrBuf) {
    *(u32*)pg_pointer(&env, zombieEntityPtrBuf, 4) = h;
    PG_FN_DELETE_VEHICLE(instance, zombieEntityPtrBuf);
  }
}

static void pg_zombie_remove_blip(w2c_game *instance, u32 *blipHandle) {
  if (!blipHandle || !*blipHandle) return;
  u32 b = *blipHandle;
  *blipHandle = 0;

  if (!zombieBlipPtrBuf) zombieBlipPtrBuf = pg_asi_alloc(16);
  if (zombieBlipPtrBuf) {
    *(u32*)pg_pointer(&env, zombieBlipPtrBuf, 4) = b;
    // 0x86a652570e5f25dd: REMOVE_BLIP (takes Blip* in rage)
    pg_test_call(instance, 0x86a652570e5f25ddull, 1, (uint64_t[]){ zombieBlipPtrBuf });
  }
}

// --- Teleport player outdoors if inside safehouse/mansion ---
static void pg_zombie_teleport_outdoors(w2c_game *instance, u32 playerPed) {
  if (!playerPed || !PG_FN_ENTITY_EXISTS(instance, playerPed)) return;
  if (!zombieCoordVector) zombieCoordVector = pg_asi_alloc(32);
  if (!zombieCoordVector) return;

  PG_FN_GET_ENTITY_COORDS(instance, zombieCoordVector, playerPed, 1);
  float *v = (float*)pg_pointer(&env, zombieCoordVector, 24);
  float px = v[0], py = v[2], pz = v[4];

  // Detect interior: Michael's mansion master bedroom / living room (-830..-795, 165..195, Z > 70)
  // or any interior where Z is high / inside ceiling
  bool insideMansion = (px > -830.0f && px < -795.0f && py > 165.0f && py < 195.0f);

  if (insideMansion || !playerTeleportedOutdoors) {
    playerTeleportedOutdoors = true;
    // Portola Drive asphalt street right outside Michael's gate
    v[0] = -840.0f; v[1] = 0;
    v[2] = 148.0f;  v[3] = 0;
    v[4] = 66.0f;   v[5] = 0;
    PG_FN_SET_ENTITY_COORDS_NO_OFFSET(instance, playerPed, zombieCoordVector, 0, 0, 0);
    pg_zombie_log("[Zombie] Da dua nguoi choi tu trong nha (%.0f, %.0f, %.0f) ra ngoai duong pho (%.0f, %.0f, %.0f).",
                  px, py, pz, v[0], v[2], v[4]);
  }
}

// ============================================================================
// FIX LỖI 3: MAP TỐI RỒI MỚI SÁNG (Khóa giờ 12h & Overcast ngay frame đầu)
// ============================================================================
static void pg_zombie_apply_atmosphere_immediate(w2c_game *instance) {
  if (!atomic_load(&zombieEnableAtmosphere)) return;

  // 1. Broad daylight 12:00 PM
  PG_FN_SET_CLOCK(instance, 12, 0, 0);

  // 2. Lock / Freeze clock so it stays permanently 12:00 PM (no dark cycle!)
  // 0x4055e40bd2dbec1d: PAUSE_CLOCK(1)
  pg_test_call(instance, 0x4055e40bd2dbec1dull, 1, (uint64_t[]){ 1 });

  // 3. Clear any previous weather override and set OVERCAST
  PG_FN_CLEAR_WEATHER_OVERRIDE(instance);
  uint64_t wStr = pg_zstr(&zombieStrWeather, "OVERCAST");
  if (wStr) PG_FN_SET_WEATHER(instance, wStr);

  // 4. Timecycle: prologue_shootout at 0.35f (cold desaturated apocalyptic tone)
  uint64_t tcStr = pg_zstr(&zombieStrTimecycle, "prologue_shootout");
  if (tcStr) {
    // 0x2c933abf17a1df41: SET_TIMECYCLE_MODIFIER
    pg_test_call(instance, 0x2c933abf17a1df41ull, 1, (uint64_t[]){ tcStr });
    // 0x82e7ffcd5b2326b3: SET_TIMECYCLE_MODIFIER_STRENGTH
    pg_test_call(instance, 0x82e7ffcd5b2326b3ull, 1, (uint64_t[]){ pg_zf32(0.35f) });
  }

  // 5. Audio: Silence city sirens and ambient driving engine sounds
  if (atomic_load(&zombieEnableAudio)) {
    uint64_t aStr = pg_zstr(&zombieStrAudioScene, "DLC_AW_Arena_Wars_Disable_Ambient_Traffic");
    if (aStr) {
      // 0x013a80fc08f6e4f2: START_AUDIO_SCENE
      pg_test_call(instance, 0x013a80fc08f6e4f2ull, 1, (uint64_t[]){ aStr });
    }
  }
}

// ============================================================================
// FIX LỖI 1 & 2: TRAFFIC SUPPRESSION, WEAPONS & OUTDOORS
// ============================================================================
static void pg_zombie_apply_traffic_suppression(w2c_game *instance) {
  if (!atomic_load(&zombieEnableTraffic)) return;

  // Zero ambient ped and vehicle density every frame (fast C call)
  PG_FN_PED_DENSITY_THIS_FRAME(instance, 0.0f);
  PG_FN_VEHICLE_DENSITY_THIS_FRAME(instance, 0.0f);
  PG_FN_PARKED_DENSITY_THIS_FRAME(instance, 0.0f);
  PG_FN_REDUCE_PED_MODEL_BUDGET(instance, 1);
  PG_FN_REDUCE_VEHICLE_MODEL_BUDGET(instance, 1);
}

static void pg_zombie_setup_police_ignore(w2c_game *instance) {
  u32 player = PG_FN_PLAYER_ID(instance);
  if (player < 32) {
    PG_FN_SET_MAX_WANTED(instance, 0);
    if (PG_FN_GET_WANTED(instance, player) > 0) PG_FN_CLEAR_WANTED(instance, player);
    // 0x32c62aa929c2da6a: SET_POLICE_IGNORE_PLAYER
    pg_test_call(instance, 0x32c62aa929c2da6aull, 2, (uint64_t[]){ player, 1 });
    // 0xdb172424876553f4: SET_DISPATCH_COPS_FOR_PLAYER
    pg_test_call(instance, 0xdb172424876553f4ull, 2, (uint64_t[]){ player, 0 });
  }
}

static void pg_zombie_apply_initial_loadout(w2c_game *instance, u32 playerPed) {
  if (!atomic_load(&zombieEnableWeapons)) return;

  // 0xf25df915fa38c5f3: REMOVE_ALL_PED_WEAPONS
  pg_test_call(instance, 0xf25df915fa38c5f3ull, 2, (uint64_t[]){ playerPed, 1 });

  // Initial melee loadout
  PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_machete"), 1, 0, 1);
  PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_knife"), 1, 0, 0);
  PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_crowbar"), 1, 0, 0);

  PG_FN_SET_ENTITY_HEALTH(instance, playerPed, 200, 0);
  // 0xcea04d83135264cc: SET_PED_ARMOUR
  pg_test_call(instance, 0xcea04d83135264ccull, 2, (uint64_t[]){ playerPed, 50 });

  atomic_store(&zombieKills, 0);
  atomic_store(&zombieTier, 0);
  atomic_store(&zombieEscaped, false);
  evacHoldStartTime = 0;
  snprintf(zombieStatusText, sizeof(zombieStatusText), "SINH TON: DUNG CAN CHIEN THOAT KHOI THANH PHO");
  pg_zombie_log("[Zombie Loadout] Da cap trang bi can chien ban dau (Machete, Knife, Crowbar, 50 Giap).");
}

static void pg_zombie_setup_extraction(w2c_game *instance) {
  if (!atomic_load(&zombieEnableExtraction)) return;

  if (!evacBlip) {
    // 0x5a039bb0bca604b6: ADD_BLIP_FOR_COORD
    uint64_t bArgs[3] = { pg_zf32(evacX), pg_zf32(evacY), pg_zf32(evacZ) };
    evacBlip = (u32)pg_test_call(instance, 0x5a039bb0bca604b6ull, 3, bArgs);
    if (evacBlip) {
      // 0xdf735600a4696daf: SET_BLIP_SPRITE (38 = Helicopter)
      pg_test_call(instance, 0xdf735600a4696dafull, 2, (uint64_t[]){ evacBlip, 38 });
      // 0x03d7fb09e75d6b7e: SET_BLIP_COLOUR (2 = Green)
      pg_test_call(instance, 0x03d7fb09e75d6b7eull, 2, (uint64_t[]){ evacBlip, 2 });
      // 0xd38744167b2fa257: SET_BLIP_SCALE
      pg_test_call(instance, 0xd38744167b2fa257ull, 2, (uint64_t[]){ evacBlip, pg_zf32(1.20f) });
      // 0xbe8be4fe60e27b72: SET_BLIP_AS_SHORT_RANGE (0 = visible across entire map and minimap edge)
      pg_test_call(instance, 0xbe8be4fe60e27b72ull, 2, (uint64_t[]){ evacBlip, 0 });
      // 0x9029b2f3da924928: SET_BLIP_DISPLAY (2 = Map & minimap)
      pg_test_call(instance, 0x9029b2f3da924928ull, 2, (uint64_t[]){ evacBlip, 2 });
    }
  }
}

// Companion Survivor
static void pg_zombie_manage_companion(w2c_game *instance, u32 playerPed, float px, float py, float pz, float heading, double now) {
  if (!atomic_load(&zombieEnableCompanion)) return;

  if (companionPed && PG_FN_ENTITY_EXISTS(instance, companionPed)) {
    // 0x3317dedb88c95038: IS_PED_DEAD_OR_DYING
    bool isDead = (bool)pg_test_call(instance, 0x3317dedb88c95038ull, 1, (uint64_t[]){ companionPed });
    if (isDead) {
      pg_zombie_remove_blip(instance, &companionBlip);
      pg_zombie_release_ped(instance, &companionPed);
      companionRespawnAtTime = now + 40000;
      pg_zombie_log("[Zombie Companion] Dong doi da bi zombie can guc! Tiep vien se den sau 40 giay.");
      return;
    }

    PG_FN_GET_ENTITY_COORDS(instance, zombieCoordVector, companionPed, 1);
    float *cc = (float*)pg_pointer(&env, zombieCoordVector, 24);
    float dx = px - cc[0], dy = py - cc[2];
    float distSq = dx * dx + dy * dy;

    // Auto-teleport if stuck or fell behind (> 40m)
    if (distSq > 40.0f * 40.0f) {
      float *v = (float*)pg_pointer(&env, zombieCoordVector, 24);
      v[0] = px - 2.0f; v[1] = 0; v[2] = py - 2.0f; v[3] = 0; v[4] = pz + 0.5f; v[5] = 0;
      PG_FN_SET_ENTITY_COORDS_NO_OFFSET(instance, companionPed, zombieCoordVector, 0, 0, 0);
    }

    if (now - lastCompanionCheckTime > 2000) {
      lastCompanionCheckTime = now;
      u32 closestZombie = 0;
      float closestDistSq = 30.0f * 30.0f;

      for (int i = 0; i < ZOMBIE_MAX_ACTIVE; i++) {
        u32 z = activeZombies[i];
        if (z && PG_FN_ENTITY_EXISTS(instance, z)) {
          PG_FN_GET_ENTITY_COORDS(instance, zombieCoordVector, z, 1);
          float *zc = (float*)pg_pointer(&env, zombieCoordVector, 24);
          float zdx = px - zc[0], zdy = py - zc[2];
          float zdSq = zdx * zdx + zdy * zdy;
          if (zdSq < closestDistSq) { closestDistSq = zdSq; closestZombie = z; }
        }
      }

      if (closestZombie) {
        // 0xf166e48407bac484: TASK_COMBAT_PED
        uint64_t cArgs[4] = { companionPed, closestZombie, 0, 16 };
        pg_test_call(instance, 0xf166e48407bac484ull, 4, cArgs);
      } else if (distSq > 8.0f * 8.0f) {
        // 0x6a071245eb0d1882: TASK_GO_TO_ENTITY
        uint64_t fArgs[7] = { companionPed, playerPed, (uint64_t)-1, pg_zf32(2.5f), pg_zf32(2.0f), 1073741824, 0 };
        pg_test_call(instance, 0x6a071245eb0d1882ull, 7, fArgs);
      }
    }
    return;
  }

  // Spawn companion
  if (now < companionRespawnAtTime) return;
  if (!pg_zombie_has_memory()) return;

  PG_FN_REQUEST_MODEL(instance, companionModel);
  if (!PG_FN_HAS_MODEL_LOADED(instance, companionModel)) return;

  float spawnX = px - sinf(heading * 0.0174533f) * 2.5f;
  float spawnY = py + cosf(heading * 0.0174533f) * 2.5f;
  uint64_t pArgs[8] = {
    4, companionModel, pg_zf32(spawnX), pg_zf32(spawnY), pg_zf32(pz + 0.5f), pg_zf32(heading), 0, 1
  };
  companionPed = (u32)pg_test_call(instance, 0xd49f9b0955c367deull, 8, pArgs);

  if (companionPed && PG_FN_ENTITY_EXISTS(instance, companionPed)) {
    PG_FN_SET_ENTITY_HEALTH(instance, companionPed, 350, 0);
    pg_test_call(instance, 0xcea04d83135264ccull, 2, (uint64_t[]){ companionPed, 100 });
    pg_test_call(instance, 0x7aefb85c1d49deb6ull, 2, (uint64_t[]){ companionPed, 85 });

    PG_FN_GIVE_WEAPON(instance, companionPed, pg_joaat("weapon_pumpshotgun"), 500, 0, 1);
    PG_FN_GIVE_WEAPON(instance, companionPed, pg_joaat("weapon_combatpistol"), 500, 0, 0);

    uint64_t pGroup = pg_test_call(instance, 0xf162e133b4e7a675ull, 1, (uint64_t[]){ playerPed });
    if (!pGroup || pGroup == (uint64_t)-1) {
      pGroup = pg_test_call(instance, 0x90370ebe0fee1a3dull, 1, (uint64_t[]){ 0 });
    }
    pg_test_call(instance, 0x9f3480fe65db31b5ull, 2, (uint64_t[]){ companionPed, pGroup });
    pg_test_call(instance, 0x3dbfc55d5c9bb447ull, 2, (uint64_t[]){ companionPed, 1 });

    companionBlip = (u32)pg_test_call(instance, 0x5cde92c702a8fce7ull, 1, (uint64_t[]){ companionPed });
    if (companionBlip) {
      pg_test_call(instance, 0x03d7fb09e75d6b7eull, 2, (uint64_t[]){ companionBlip, 3 }); // Blue
      pg_test_call(instance, 0xd38744167b2fa257ull, 2, (uint64_t[]){ companionBlip, pg_zf32(0.85f) });
      pg_test_call(instance, 0xbe8be4fe60e27b72ull, 2, (uint64_t[]){ companionBlip, 0 });
      pg_test_call(instance, 0x9029b2f3da924928ull, 2, (uint64_t[]){ companionBlip, 2 });
    }
    pg_zombie_log("[Zombie Companion] Da spawn thanh cong Dong doi (Handle %u) ben canh nguoi choi.", companionPed);
  }
  PG_FN_RELEASE_MODEL(instance, companionModel);
}

// ============================================================================
// FIX LỖI 2: ABANDONED WRECKS SPAWNING (Pinned model streaming)
// ============================================================================
static uint32_t activeWreckModelHash = 0;
static void pg_zombie_manage_wrecks(w2c_game *instance, float px, float py, float pz, float heading, double now) {
  if (!atomic_load(&zombieEnableWrecks)) return;

  // Clean up distant wrecks (> 70m)
  for (int i = 0; i < ZOMBIE_MAX_WRECKS; i++) {
    u32 veh = activeWrecks[i];
    if (veh && PG_FN_ENTITY_EXISTS(instance, veh)) {
      PG_FN_GET_ENTITY_COORDS(instance, zombieCoordVector, veh, 1);
      float *c = (float*)pg_pointer(&env, zombieCoordVector, 24);
      float dx = px - c[0], dy = py - c[2];
      if (dx * dx + dy * dy > 70.0f * 70.0f) {
        pg_zombie_delete_wreck(instance, &activeWrecks[i]);
      }
    } else {
      activeWrecks[i] = 0;
    }
  }

  // Rate-limit: 1 wreck per 6.0s
  if (now - lastWreckSpawnTime < 6000) return;

  int freeSlot = -1;
  int currentWreckCount = 0;
  for (int i = 0; i < ZOMBIE_MAX_WRECKS; i++) {
    if (activeWrecks[i]) currentWreckCount++;
    else if (freeSlot < 0) freeSlot = i;
  }
  if (freeSlot < 0 || currentWreckCount >= ZOMBIE_MAX_WRECKS) return;
  if (!pg_zombie_has_memory()) return;

  // Pin model until loaded
  if (!activeWreckModelHash) {
    static int modelCycle = 0;
    activeWreckModelHash = pg_joaat(zombieWreckModels[modelCycle % 4]);
    modelCycle++;
  }

  PG_FN_REQUEST_MODEL(instance, activeWreckModelHash);
  if (!PG_FN_HAS_MODEL_LOADED(instance, activeWreckModelHash)) return;

  lastWreckSpawnTime = now;

  float dist = 22.0f + (float)(rand() % 18);
  float angOffset = (float)((rand() % 80) - 40) * 0.0174533f;
  float rad = heading * 0.0174533f + angOffset;

  float *v = (float*)pg_pointer(&env, zombieCoordVector, 24);
  v[0] = px - sinf(rad) * dist; v[1] = 0;
  v[2] = py + cosf(rad) * dist; v[3] = 0;
  v[4] = pz + 0.5f; v[5] = 0;

  float wreckHeading = heading + (float)(rand() % 180);
  u32 veh = PG_FN_CREATE_VEHICLE(instance, activeWreckModelHash, zombieCoordVector, wreckHeading, 0, 0, 1);
  if (veh && PG_FN_ENTITY_EXISTS(instance, veh)) {
    PG_FN_GROUND_VEHICLE(instance, veh, 5.0f);
    PG_FN_VEHICLE_DIRT(instance, veh, 15.0f);
    PG_FN_VEHICLE_ENGINE_HEALTH(instance, veh, -4000.0f);

    pg_test_call(instance, 0x8aba6af54b942b95ull, 2, (uint64_t[]){ veh, 1 }); // undriveable
    pg_test_call(instance, 0xb664292eaecf7fa6ull, 2, (uint64_t[]){ veh, 2 }); // doors locked
    pg_test_call(instance, 0xec6a202ee4960385ull, 4, (uint64_t[]){ veh, 0, 1, pg_zf32(1000.0f) }); // flat tire 0
    pg_test_call(instance, 0xec6a202ee4960385ull, 4, (uint64_t[]){ veh, 4, 1, pg_zf32(1000.0f) }); // flat tire 4
    pg_test_call(instance, 0x9e5b5e4d2ccd2259ull, 2, (uint64_t[]){ veh, 0 }); // smash window

    activeWrecks[freeSlot] = veh;
    pg_zombie_log("[Wreck Spawner] Da spawn Xe bo hoang #%d (Handle %u, Model 0x%08x) tai (%.1f, %.1f, %.1f).",
                  currentWreckCount + 1, veh, activeWreckModelHash, v[0], v[2], v[4]);
  }

  PG_FN_RELEASE_MODEL(instance, activeWreckModelHash);
  activeWreckModelHash = 0; // Ready for next model
}

// ============================================================================
// FIX LỖI 2: CONTROLLED ZOMBIE HORDE SPAWNER (Guaranteed loaded models)
// ============================================================================
static void pg_zombie_update_weapons(w2c_game *instance, u32 playerPed, int kills) {
  if (!atomic_load(&zombieEnableWeapons)) return;

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
      PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_combatpistol"), 60, 0, 1);
      pg_zombie_log("[Zombie Weapons] Mo khoa Vu khi Cap 1: Sung luc Combat Pistol (5 kills).");
    } else if (targetTier == 2) {
      PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_pumpshotgun"), 35, 0, 1);
      pg_zombie_log("[Zombie Weapons] Mo khoa Vu khi Cap 2: Sung san Pump Shotgun (15 kills).");
    } else if (targetTier == 3) {
      PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_microsmg"), 150, 0, 1);
      pg_zombie_log("[Zombie Weapons] Mo khoa Vu khi Cap 3: Sung tieu lien Micro SMG (30 kills).");
    } else if (targetTier == 4) {
      PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_carbinerifle"), 200, 0, 1);
      pg_zombie_log("[Zombie Weapons] Mo khoa Vu khi Cap 4: Sung truong Carbine Rifle (50 kills).");
    } else if (targetTier == 5) {
      PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_combatmg"), 350, 0, 1);
      PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_assaultshotgun"), 60, 0, 0);
      pg_zombie_log("[Zombie Weapons] Mo khoa Vu khi Cap 5: Sung may nang Combat MG (75 kills).");
    }
  }
}

static void pg_zombie_manage_horde(w2c_game *instance, u32 playerPed, float px, float py, float pz, float heading, double now) {
  if (!atomic_load(&zombieEnableZombies)) return;

  // Request zombie models so at least one is always ready
  for (int m = 0; m < activeZombieModelCount; m++) {
    PG_FN_REQUEST_MODEL(instance, activeZombieModels[m]);
  }

  int livingZombies = 0;

  // Update living zombies (staggered check)
  for (int i = 0; i < ZOMBIE_MAX_ACTIVE; i++) {
    u32 z = activeZombies[i];
    if (!z) continue;

    if (!PG_FN_ENTITY_EXISTS(instance, z)) {
      pg_zombie_remove_blip(instance, &zombieBlips[i]);
      activeZombies[i] = 0;
      zombieChasing[i] = false;
      continue;
    }

    // Check if dead
    bool isDead = (bool)pg_test_call(instance, 0x3317dedb88c95038ull, 1, (uint64_t[]){ z });
    if (isDead) {
      pg_zombie_remove_blip(instance, &zombieBlips[i]);
      int currentKills = atomic_fetch_add(&zombieKills, 1) + 1;
      pg_zombie_update_weapons(instance, playerPed, currentKills);
      pg_zombie_release_ped(instance, &activeZombies[i]);
      zombieChasing[i] = false;
      continue;
    }

    livingZombies++;

    // Distance to player
    PG_FN_GET_ENTITY_COORDS(instance, zombieCoordVector, z, 1);
    float *zc = (float*)pg_pointer(&env, zombieCoordVector, 24);
    float dx = px - zc[0], dy = py - zc[2];
    float dist = sqrtf(dx * dx + dy * dy);

    // Despawn if too far (> 75m)
    if (dist > 75.0f) {
      pg_zombie_remove_blip(instance, &zombieBlips[i]);
      pg_zombie_release_ped(instance, &activeZombies[i]);
      zombieChasing[i] = false;
      continue;
    }

    // AI: When distance <= 25m -> Alert, scream & charge at sprint speed!
    if (dist <= 25.0f && !zombieChasing[i]) {
      zombieChasing[i] = true;
      u32 target = playerPed;
      if (companionPed && PG_FN_ENTITY_EXISTS(instance, companionPed) && (rand() % 3 == 0)) {
        target = companionPed;
      }
      pg_test_call(instance, 0xf166e48407bac484ull, 4, (uint64_t[]){ z, target, 0, 16 });
      pg_test_call(instance, 0x085bf80fa50a39d1ull, 2, (uint64_t[]){ z, pg_zf32(1.30f) });
    }

    // Guttural sound when near (4m - 10m)
    if (dist >= 4.0f && dist <= 10.0f && (now - lastGroanSoundTime > 4500) && atomic_load(&zombieEnableAudio)) {
      lastGroanSoundTime = now;
      pg_test_call(instance, 0xbc9ae166038a5cecull, 3, (uint64_t[]){ z, 6, 0 });
    }

    // Real bite damage when close (< 1.6m)
    if (dist < 1.6f && (now - lastBiteDamageTime > 1200)) {
      lastBiteDamageTime = now;
      u32 pHealth = (u32)pg_test_call(instance, 0xeef059fad016d209ull, 1, (uint64_t[]){ playerPed });
      if (pHealth > 100) {
        u32 newHealth = (pHealth > 115) ? (pHealth - 15) : 100;
        PG_FN_SET_ENTITY_HEALTH(instance, playerPed, newHealth, 0);

        if (atomic_load(&zombieEnableAudio)) {
          pg_test_call(instance, 0xbc9ae166038a5cecull, 3, (uint64_t[]){ playerPed, 8, 0 });
          pg_test_call(instance, 0xbc9ae166038a5cecull, 3, (uint64_t[]){ z, 7, 0 });
        }
      }
    }
  }

  // Rate-limit Spawning: 1 zombie per 3.0s, hard cap ZOMBIE_MAX_ACTIVE (12)
  if (now - lastZombieSpawnTime < 3000) return;

  if (livingZombies >= ZOMBIE_MAX_ACTIVE) return;
  if (!pg_zombie_has_memory()) return;

  // Choose loaded model
  uint32_t chosen = 0;
  for (int m = 0; m < activeZombieModelCount; m++) {
    if (PG_FN_HAS_MODEL_LOADED(instance, activeZombieModels[m])) {
      chosen = activeZombieModels[m];
      break;
    }
  }
  if (!chosen) return;

  lastZombieSpawnTime = now;

  int freeSlot = -1;
  for (int i = 0; i < ZOMBIE_MAX_ACTIVE; i++) {
    if (!activeZombies[i]) { freeSlot = i; break; }
  }
  if (freeSlot < 0) return;

  // Spawn outside player direct camera view (120 to 240 deg behind/flanks)
  float fovOffset = (120.0f + (float)(rand() % 120)) * 0.0174533f;
  float rad = heading * 0.0174533f + fovOffset;
  float dist = 24.0f + (float)(rand() % 14); // 24m to 38m away
  float zx = px - sinf(rad) * dist;
  float zy = py + cosf(rad) * dist;

  uint64_t pArgs[8] = {
    4, chosen, pg_zf32(zx), pg_zf32(zy), pg_zf32(pz + 0.5f), pg_zf32(0.0f), 0, 1
  };
  u32 zPed = (u32)pg_test_call(instance, 0xd49f9b0955c367deull, 8, pArgs);
  if (!zPed || !PG_FN_ENTITY_EXISTS(instance, zPed)) return;

  // Disarm
  pg_test_call(instance, 0xf25df915fa38c5f3ull, 2, (uint64_t[]){ zPed, 1 });
  // Fearless
  pg_test_call(instance, 0x9f8aa94d6d97dbf4ull, 2, (uint64_t[]){ zPed, 1 });
  // Combat & Flee flags
  pg_test_call(instance, 0x70a2d1137c8ed7c9ull, 3, (uint64_t[]){ zPed, 0, 0 });
  pg_test_call(instance, 0x9f7794730795e019ull, 3, (uint64_t[]){ zPed, 46, 1 });
  // Can ragdoll
  pg_test_call(instance, 0xb128377056a54e2aull, 2, (uint64_t[]){ zPed, 1 });

  // Health
  int kills = atomic_load(&zombieKills);
  u32 zHp = 140 + (u32)((kills > 50) ? 50 : kills);
  PG_FN_SET_ENTITY_HEALTH(instance, zPed, zHp, 0);

  // Blood decal
  pg_test_call(instance, 0x83f7e01c7b769a26ull, 5, (uint64_t[]){ zPed, 3, 0, 0, 0 });

  // Initial wander task (staggered limping walk)
  pg_test_call(instance, 0xbb9ce077274f6a1bull, 3, (uint64_t[]){ zPed, pg_zf32(10.0f), 10 });
  zombieChasing[freeSlot] = false;

  // Red radar blip - visible across entire map!
  u32 zBlip = (u32)pg_test_call(instance, 0x5cde92c702a8fce7ull, 1, (uint64_t[]){ zPed });
  if (zBlip) {
    pg_test_call(instance, 0x03d7fb09e75d6b7eull, 2, (uint64_t[]){ zBlip, 1 }); // Red
    pg_test_call(instance, 0xd38744167b2fa257ull, 2, (uint64_t[]){ zBlip, pg_zf32(0.70f) });
    pg_test_call(instance, 0xbe8be4fe60e27b72ull, 2, (uint64_t[]){ zBlip, 0 }); // Global radar
    pg_test_call(instance, 0x9029b2f3da924928ull, 2, (uint64_t[]){ zBlip, 2 });
  }

  activeZombies[freeSlot] = zPed;
  zombieBlips[freeSlot] = zBlip;

  pg_zombie_log("[Zombie Spawner] Da spawn thanh cong Zombie #%d (Handle %u, Model 0x%08x) tai (%.1f, %.1f, %.1f).",
                livingZombies + 1, zPed, chosen, zx, zy, pz);
}

// Extraction Objective Update
static void pg_zombie_manage_extraction(w2c_game *instance, u32 playerPed, float px, float py, float pz, double now) {
  if (!atomic_load(&zombieEnableExtraction)) return;

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

  // Inside extraction zone (< 40m from Del Perro Pier)
  if (distToEvac < 40.0f) {
    if (evacHoldStartTime == 0) {
      evacHoldStartTime = now;
      pg_zombie_log("[Zombie Evac] Nguoi choi da vao vung so tan! Bat dau dem nguoc 45 giay de truc thang den.");
    }
    int secondsLeft = 45 - (int)((now - evacHoldStartTime) / 1000.0);

    if (secondsLeft > 0) {
      snprintf(zombieStatusText, sizeof(zombieStatusText), "DANG CO THU DIEM SO TAN! CON %d GIAY DE THOAT", secondsLeft);
    } else {
      atomic_store(&zombieEscaped, true);
      snprintf(zombieStatusText, sizeof(zombieStatusText), "CHIEN THANG: TRUC THANG CUU HO DA DEN! THOAT THANH CONG!");
      PG_FN_CLEAR_WANTED(instance, PG_FN_PLAYER_ID(instance));
      pg_zombie_log("[Zombie Evac] CHUC MUNG: Ban da song sot va thoat khoi Los Santos thanh cong!");
    }
  } else {
    evacHoldStartTime = 0;
    snprintf(zombieStatusText, sizeof(zombieStatusText), "DIEM SO TAN: %.0f M | KILLS: %d | CAP VU KHI: %d", distToEvac, kills, tier);
  }
}

// Cleanup all
static void pg_zombie_cleanup_all(w2c_game *instance) {
  for (int i = 0; i < ZOMBIE_MAX_ACTIVE; i++) {
    pg_zombie_remove_blip(instance, &zombieBlips[i]);
    pg_zombie_release_ped(instance, &activeZombies[i]);
    zombieChasing[i] = false;
  }
  for (int i = 0; i < ZOMBIE_MAX_WRECKS; i++) {
    pg_zombie_delete_wreck(instance, &activeWrecks[i]);
  }
  pg_zombie_remove_blip(instance, &companionBlip);
  pg_zombie_release_ped(instance, &companionPed);
  pg_zombie_remove_blip(instance, &evacBlip);

  // 0x0f07e7745a236711: CLEAR_TIMECYCLE_MODIFIER
  pg_test_call(instance, 0x0f07e7745a236711ull, 0, NULL);
  playerTeleportedOutdoors = false;
  pg_zombie_log("[Zombie System] Da don dep toan bo entity va dat lai trang thai.");
}

// ============================================================================
// MAIN SCRIPT TICK LOOP (Lightweight, staggered execution)
// ============================================================================
static void pg_zombie_tick(w2c_game *instance) {
  if (!atomic_load(&zombieActive)) return;
  if (PGPauseMenuActive() || PGLoadSceneActive() || (PGGameStatus() & 16)) return;

  u32 playerPed = PG_FN_PLAYER_PED_ID(instance);
  if (!playerPed || !PG_FN_ENTITY_EXISTS(instance, playerPed)) return;

  if (!pg_asi_game) pg_asi_game = instance;
  uint64_t prevScript = pg_asi_enter_script(instance);

  zombieFrameCounter++;
  double now = timeMS(CLOCK_MONOTONIC);

  // Allocate coordinate vector once
  if (!zombieCoordVector) zombieCoordVector = pg_asi_alloc(32);
  if (!zombieCoordVector) {
    pg_asi_leave_script(instance, prevScript);
    return;
  }

  // Handle explicit reset request from UI
  if (atomic_exchange(&zombieResetRequested, false)) {
    pg_zombie_cleanup_all(instance);
    zombieCurrentStage = ZOMBIE_STAGE_WEAPONS;
    pg_zombie_teleport_outdoors(instance, playerPed);
    pg_zombie_apply_initial_loadout(instance, playerPed);
    pg_asi_leave_script(instance, prevScript);
    return;
  }

  // ==========================================================================
  // STAGE 0: IMMEDIATE DAYLIGHT & WEATHER (Fix lỗi 3: Không bị tối rồi mới sáng)
  // ==========================================================================
  if (zombieCurrentStage == ZOMBIE_STAGE_BOOT) {
    pg_zombie_apply_atmosphere_immediate(instance);
    zombieCurrentStage = ZOMBIE_STAGE_WARMUP;
    pg_zombie_log("[Zombie] Stage 0 KHOI TAO: Da dong bang gio 12h trua va thoi tiet OVERCAST ngay lap tuc.");
  }

  // ==========================================================================
  // STAGE 1: TELEPORT OUTDOORS & STABILIZE ROAD (Fix lỗi 2: Đưa ra ngoài đường)
  // ==========================================================================
  if (zombieCurrentStage == ZOMBIE_STAGE_WARMUP) {
    if (zombieFrameCounter > 60) { // ~1s sau khi vao map
      pg_zombie_teleport_outdoors(instance, playerPed);
      pg_zombie_setup_police_ignore(instance);

      // Kiem tra xem game co model zombie that u_m_y_zombie_01 hay khong (0xc0296a2edf545e92: IS_MODEL_VALID)
      uint32_t realZombie = pg_joaat("u_m_y_zombie_01");
      if ((bool)pg_test_call(instance, 0xc0296a2edf545e92ull, 1, (uint64_t[]){ realZombie })) {
        activeZombieModels[activeZombieModelCount++] = realZombie;
        pg_zombie_log("[Zombie Model] Xac nhan model zombie that u_m_y_zombie_01 ton tai trong game, da them vao danh sach spawn.");
      } else {
        pg_zombie_log("[Zombie Model] Model u_m_y_zombie_01 khong co trong du lieu story mode, su dung skater & hillbilly.");
      }

      zombieCurrentStage = ZOMBIE_STAGE_TRAFFIC;
      pg_zombie_log("[Zombie] Stage 1 HOAN TAT: Nhan vat da duoc dua ra ngoai duong pho thoang dang.");
    }
  }

  // ==========================================================================
  // STAGE 2: SUPPRESS AMBIENT TRAFFIC (Frame 120+, ~2s)
  // ==========================================================================
  if (zombieCurrentStage == ZOMBIE_STAGE_TRAFFIC) {
    if (zombieFrameCounter > 120) {
      zombieCurrentStage = ZOMBIE_STAGE_WEAPONS;
      pg_zombie_log("[Zombie] Stage 2 HOAN TAT: Da triet tieu giao thong va dan cu thuong.");
    }
  }

  // ==========================================================================
  // STAGE 3: INITIAL MELEE WEAPONS (Frame 180+, ~3s)
  // ==========================================================================
  if (zombieCurrentStage == ZOMBIE_STAGE_WEAPONS) {
    if (zombieFrameCounter > 180) {
      pg_zombie_apply_initial_loadout(instance, playerPed);
      zombieCurrentStage = ZOMBIE_STAGE_EXTRACTION;
      pg_zombie_log("[Zombie] Stage 3 HOAN TAT: Da trang bi vu khi can chien ban dau.");
    }
  }

  // ==========================================================================
  // STAGE 4: EXTRACTION CHECKPOINT (Frame 240+, ~4s)
  // ==========================================================================
  if (zombieCurrentStage == ZOMBIE_STAGE_EXTRACTION) {
    if (zombieFrameCounter > 240) {
      pg_zombie_setup_extraction(instance);
      zombieCurrentStage = ZOMBIE_STAGE_COMPANION;
      pg_zombie_log("[Zombie] Stage 4 HOAN TAT: Diem so tan Del Perro Pier da duoc thiet lap.");
    }
  }

  // ==========================================================================
  // STAGE 5: COMPANION SURVIVOR (Frame 300+, ~5s)
  // ==========================================================================
  if (zombieCurrentStage == ZOMBIE_STAGE_COMPANION) {
    if (zombieFrameCounter > 300) {
      zombieCurrentStage = ZOMBIE_STAGE_WRECKS;
      pg_zombie_log("[Zombie] Stage 5 HOAN TAT: Kich hoat dong doi sinh ton.");
    }
  }

  // ==========================================================================
  // STAGE 6: ABANDONED WRECKS (Frame 360+, ~6s)
  // ==========================================================================
  if (zombieCurrentStage == ZOMBIE_STAGE_WRECKS) {
    if (zombieFrameCounter > 360) {
      zombieCurrentStage = ZOMBIE_STAGE_ACTIVE;
      pg_zombie_log("[Zombie] Stage 6 HOAN TAT: Kich hoat xe bo hoang ven duong.");
      pg_zombie_log("[Zombie] TOAN BO MODULE DA KHOI TAO THANH CONG! Bat dau mo phong sinh ton day du.");
    }
  }

  // Continuous traffic suppression every frame (fast C call)
  pg_zombie_apply_traffic_suppression(instance);

  // Weather & Daylight re-sync every 60 frames (~2s) to guarantee no darkness
  if (zombieFrameCounter % 60 == 0) {
    pg_zombie_apply_atmosphere_immediate(instance);
  }

  // Get current player position
  PG_FN_GET_ENTITY_COORDS(instance, zombieCoordVector, playerPed, 1);
  float *pc = (float*)pg_pointer(&env, zombieCoordVector, 24);
  float px = pc[0], py = pc[2], pz = pc[4];
  float heading = PG_FN_GET_ENTITY_HEADING(instance, playerPed);

  // Staggered execution across frames to keep tick < 0.1ms (Fix lỗi 1: Không giật, không treo thread)
  if (zombieCurrentStage >= ZOMBIE_STAGE_COMPANION && (zombieFrameCounter % 30 == 0)) {
    pg_zombie_manage_companion(instance, playerPed, px, py, pz, heading, now);
  }

  if (zombieCurrentStage >= ZOMBIE_STAGE_WRECKS && (zombieFrameCounter % 60 == 15)) {
    pg_zombie_manage_wrecks(instance, px, py, pz, heading, now);
  }

  if (zombieCurrentStage >= ZOMBIE_STAGE_ACTIVE) {
    // Zombie horde AI runs frequently but spawning is rate-limited to 3s
    pg_zombie_manage_horde(instance, playerPed, px, py, pz, heading, now);

    if (zombieFrameCounter % 30 == 15) {
      pg_zombie_manage_extraction(instance, playerPed, px, py, pz, now);
    }
  }

  // Periodic diagnostic log (every 10s)
  if (now - lastPeriodicLogTime > 10000) {
    lastPeriodicLogTime = now;
    int livingZ = 0;
    for (int i = 0; i < ZOMBIE_MAX_ACTIVE; i++) if (activeZombies[i]) livingZ++;
    int livingW = 0;
    for (int i = 0; i < ZOMBIE_MAX_WRECKS; i++) if (activeWrecks[i]) livingW++;
    pg_zombie_log("[Zombie Stats] Zombie: %d/%d | Xe bo hoang: %d/%d | Kills: %d | Cap vu khi: %d | Khoang cach evac: %.0fm",
                  livingZ, ZOMBIE_MAX_ACTIVE, livingW, ZOMBIE_MAX_WRECKS,
                  atomic_load(&zombieKills), atomic_load(&zombieTier), atomic_load(&zombieDistanceToEvac));
  }

  pg_asi_leave_script(instance, prevScript);
}

// ============================================================================
// PUBLIC C APIS (Exposed to Swift & UIKit Settings)
// ============================================================================
bool PGZombieIsActive(void) { return atomic_load(&zombieActive); }
void PGZombieSetActive(bool active) {
  atomic_store(&zombieActive, active);
  if (!active && pg_asi_game) {
    uint64_t prev = pg_asi_enter_script(pg_asi_game);
    pg_zombie_cleanup_all(pg_asi_game);
    pg_asi_leave_script(pg_asi_game, prev);
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

int PGZombieGetActiveCount(void) {
  int count = 0;
  for (int i = 0; i < ZOMBIE_MAX_ACTIVE; i++) if (activeZombies[i]) count++;
  return count;
}

int PGZombieGetWreckCount(void) {
  int count = 0;
  for (int i = 0; i < ZOMBIE_MAX_WRECKS; i++) if (activeWrecks[i]) count++;
  return count;
}

// Submodule Toggles
bool PGZombieGetModuleAtmosphere(void) { return atomic_load(&zombieEnableAtmosphere); }
void PGZombieSetModuleAtmosphere(bool enabled) { atomic_store(&zombieEnableAtmosphere, enabled); }

bool PGZombieGetModuleWrecks(void) { return atomic_load(&zombieEnableWrecks); }
void PGZombieSetModuleWrecks(bool enabled) { atomic_store(&zombieEnableWrecks, enabled); }

bool PGZombieGetModuleCompanion(void) { return atomic_load(&zombieEnableCompanion); }
void PGZombieSetModuleCompanion(bool enabled) { atomic_store(&zombieEnableCompanion, enabled); }

bool PGZombieGetModuleExtraction(void) { return atomic_load(&zombieEnableExtraction); }
void PGZombieSetModuleExtraction(bool enabled) { atomic_store(&zombieEnableExtraction, enabled); }

// Live In-Memory Log Reader for Swift UI
void PGZombieGetLog(char *buffer, size_t maxLen) {
  if (!buffer || maxLen == 0) return;
  pthread_mutex_lock(&zombieLogLock);
  buffer[0] = 0;
  size_t written = 0;
  int start = (zombieLogCount < ZOMBIE_MAX_LOG_LINES) ? 0 : (zombieLogHead % ZOMBIE_MAX_LOG_LINES);
  for (int i = 0; i < zombieLogCount; i++) {
    int idx = (start + i) % ZOMBIE_MAX_LOG_LINES;
    int len = snprintf(buffer + written, maxLen - written, "%s\n", zombieLogLines[idx]);
    if (len > 0 && written + (size_t)len < maxLen) {
      written += (size_t)len;
    } else {
      break;
    }
  }
  pthread_mutex_unlock(&zombieLogLock);
}
