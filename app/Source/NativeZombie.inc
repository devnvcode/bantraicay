// ============================================================================
// NATIVE ZOMBIE APOCALYPSE SURVIVAL SYSTEM - V3 ROCK-SOLID STABILITY
// PlayGTA5Native / Mugoat NewGen for iOS
// Fix tận gốc lỗi "Game đã dừng":
// 1. Khởi tạo tuần tự theo từng giai đoạn (Staged Delayed Init) trễ sau khi map load
// 2. Không quét memory arrays lạ (loại bỏ GET_PED_NEARBY_PEDS/VEHICLES gây OOB trap)
// 3. Mật độ dân & xe = 0 triệt để mỗi frame; dẹp sạch cảnh sát
// 4. Mọi model đều có REQUEST_MODEL + HAS_MODEL_LOADED trước khi dùng
// 5. Mọi entity đều kiểm tra PG_FN_ENTITY_EXISTS trước khi gọi native
// 6. Entity handle truyền vào DELETE/NO_LONGER_NEEDED qua con trỏ bộ nhớ guest an toàn
// 7. Giới hạn cứng iPhone: tối đa 12 zombie, tối đa 5 xe bỏ hoang, tổng <= 18 entity
// 8. Tốc độ sinh rải rác: 1 zombie mỗi 3s, 1 xe mỗi 6s; kiểm tra memory headroom
// 9. Ghi log khởi tạo chi tiết ra pg_log và file zombie_survival.log
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
#define ZOMBIE_MAX_TOTAL_EXTRA_ENTITIES 18

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
  "rebel", "stanier", "voodoo2", "ingot", "tornado3", "rancherxl"
};

// Ped model fallback hashes (standard GTA V story mode models guaranteed in game files)
static const uint32_t fallbackZombieModels[] = {
  0x19DFA98E, // a_m_y_skater_01
  0x3497D515, // a_m_m_hillbilly_01
  0x367E99DD  // s_m_y_clown_01
};

// Survivor companion model (Marine / Military Guard)
static const uint32_t companionModel = 0x8D8F1B10; // s_m_y_marine_01

// Escape destination: Del Perro Pier helipad area
static const float evacX = -1600.0f;
static const float evacY = -1050.0f;
static const float evacZ = 13.0f;

// Module Feature Flags (Can be individually toggled)
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
  ZOMBIE_STAGE_IDLE = 0,
  ZOMBIE_STAGE_WARMUP = 1,       // Frames 0 - 240 (~4s): Wait for map stream
  ZOMBIE_STAGE_TRAFFIC = 2,      // Frame 240 (~4s): Suppress ambient traffic & cops
  ZOMBIE_STAGE_WEAPONS = 3,      // Frame 360 (~6s): Initial melee weapons
  ZOMBIE_STAGE_ATMOSPHERE = 4,   // Frame 480 (~8s): 12h daylight, overcast, cold gray tint
  ZOMBIE_STAGE_EXTRACTION = 5,   // Frame 600 (~10s): Del Perro Pier rescue blip
  ZOMBIE_STAGE_COMPANION = 6,    // Frame 720 (~12s): Spawn companion survivor
  ZOMBIE_STAGE_WRECKS = 7,       // Frame 840 (~14s): Roadside abandoned wrecks
  ZOMBIE_STAGE_ACTIVE = 8        // Frame 960+ (~16s+): Controlled zombie horde spawner
} ZombieInitStage;

static ZombieInitStage zombieCurrentStage = ZOMBIE_STAGE_IDLE;
static int zombieFrameCounter = 0;

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

// Memory buffer slots in WebAssembly guest memory
static uint64_t zombieCoordVector = 0;
static uint64_t zombieEntityPtrBuf = 0;
static uint64_t zombieStrWeather = 0;
static uint64_t zombieStrTimecycle = 0;
static uint64_t zombieStrAudioScene = 0;
static uint64_t zombieStrDamagePack = 0;

static char zombieStatusText[160] = "ZOMBIE SURVIVAL: DANG CHUAN BI...";

// --- Diagnostics & Logging to file and pg_log ---
static void pg_zombie_log(const char *format, ...) {
  char line[384];
  va_list args;
  va_start(args, format);
  vsnprintf(line, sizeof(line), format, args);
  va_end(args);

  // Send to engine/app diagnostic stream
  pg_log(line);

  // Append to persistent log file in game directory
  static char logFilePath[PATH_MAX] = {0};
  if (!logFilePath[0]) {
    const char *mods = pg_mods_folder_path();
    if (mods && mods[0]) {
      snprintf(logFilePath, sizeof(logFilePath), "%s/zombie_survival.log", mods);
    } else {
      snprintf(logFilePath, sizeof(logFilePath), "zombie_survival.log");
    }
  }
  FILE *f = fopen(logFilePath, "a");
  if (f) {
    fprintf(f, "%s\n", line);
    fflush(f);
    fclose(f);
  }
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
  // 0x86a652570e5f25dd: REMOVE_BLIP
  pg_test_call(instance, 0x86a652570e5f25ddull, 1, (uint64_t[]){ b });
}

// --- Model Selection (Real Zombie model vs Fallback Ped) ---
static uint32_t chosenZombieModel = 0;
static bool zombieModelChecked = false;
static uint32_t pg_zombie_get_ped_model(w2c_game *instance) {
  if (!zombieModelChecked) {
    zombieModelChecked = true;
    uint32_t realZombie = pg_joaat("u_m_y_zombie_01"); // 0xac4b4506
    // 0xc0296a2edf545e92: IS_MODEL_VALID
    uint64_t valid = pg_test_call(instance, 0xc0296a2edf545e92ull, 1, (uint64_t[]){ realZombie });
    if (valid && valid != (uint64_t)-1) {
      chosenZombieModel = realZombie;
      pg_zombie_log("[Zombie] Phat hien model zombie goc trong game: u_m_y_zombie_01 (0x%08x)", realZombie);
    } else {
      chosenZombieModel = fallbackZombieModels[0]; // a_m_y_skater_01
      pg_zombie_log("[Zombie] Su dung model dan cu bi nhiem benh kem hieu ung zombie: 0x%08x", chosenZombieModel);
    }
  }
  return chosenZombieModel ? chosenZombieModel : fallbackZombieModels[0];
}

// ============================================================================
// STAGE IMPLEMENTATIONS (Sequential Delayed Init)
// ============================================================================

// Stage 2: Suppress ambient traffic, ambient peds and cops
static void pg_zombie_apply_traffic_suppression(w2c_game *instance) {
  if (!atomic_load(&zombieEnableTraffic)) return;

  // Set densities to 0 every frame
  PG_FN_PED_DENSITY_THIS_FRAME(instance, 0.0f);
  PG_FN_VEHICLE_DENSITY_THIS_FRAME(instance, 0.0f);
  PG_FN_PARKED_DENSITY_THIS_FRAME(instance, 0.0f);
  PG_FN_REDUCE_PED_MODEL_BUDGET(instance, 1);
  PG_FN_REDUCE_VEHICLE_MODEL_BUDGET(instance, 1);

  // 0xb3b3359379fe77d3: SET_RANDOM_VEHICLE_DENSITY_MULTIPLIER_THIS_FRAME (0.0f)
  pg_test_call(instance, 0xb3b3359379fe77d3ull, 1, (uint64_t[]){ pg_zf32(0.0f) });
  // 0x90b6da738a9a25da: SET_AMBIENT_VEHICLE_RANGE_MULTIPLIER_THIS_FRAME (0.0f)
  pg_test_call(instance, 0x90b6da738a9a25daull, 1, (uint64_t[]){ pg_zf32(0.0f) });

  // Police suppression
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

// Stage 3: Initial Loadout Setup (Melee weapons only at start)
static void pg_zombie_apply_initial_loadout(w2c_game *instance, u32 playerPed) {
  if (!atomic_load(&zombieEnableWeapons)) return;

  // 0xf25df915fa38c5f3: REMOVE_ALL_PED_WEAPONS
  pg_test_call(instance, 0xf25df915fa38c5f3ull, 2, (uint64_t[]){ playerPed, 1 });

  // Grant starting melee weapons
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
}

// Stage 4: Atmosphere: Daytime 12h, OVERCAST, subtle cold gray filter
static void pg_zombie_apply_atmosphere(w2c_game *instance) {
  if (!atomic_load(&zombieEnableAtmosphere)) return;

  // Broad daylight at 12:00 PM
  PG_FN_SET_CLOCK(instance, 12, 0, 0);

  // Weather: OVERCAST (bright daylight, desolate clouds, wide clear visibility without dense white fog)
  uint64_t wStr = pg_zstr(&zombieStrWeather, "OVERCAST");
  if (wStr) {
    PG_FN_CLEAR_WEATHER_OVERRIDE(instance);
    PG_FN_SET_WEATHER(instance, wStr);
  }

  // Timecycle: Subtle desaturated cold apocalyptic tone (strength 0.35f)
  uint64_t tcStr = pg_zstr(&zombieStrTimecycle, "prologue_shootout");
  if (tcStr) {
    // 0x2c933abf17a1df41: SET_TIMECYCLE_MODIFIER
    pg_test_call(instance, 0x2c933abf17a1df41ull, 1, (uint64_t[]){ tcStr });
    // 0x82e7ffcd5b2326b3: SET_TIMECYCLE_MODIFIER_STRENGTH
    pg_test_call(instance, 0x82e7ffcd5b2326b3ull, 1, (uint64_t[]){ pg_zf32(0.35f) });
  }

  // Audio: Silence ambient city vehicle traffic hum
  if (atomic_load(&zombieEnableAudio)) {
    uint64_t aStr = pg_zstr(&zombieStrAudioScene, "DLC_AW_Arena_Wars_Disable_Ambient_Traffic");
    if (aStr) {
      // 0x013a80fc08f6e4f2: START_AUDIO_SCENE
      pg_test_call(instance, 0x013a80fc08f6e4f2ull, 1, (uint64_t[]){ aStr });
    }
  }
}

// Stage 5: Extraction Checkpoint Blip at Del Perro Pier
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

// Stage 6: Survivor Companion Management
static void pg_zombie_manage_companion(w2c_game *instance, u32 playerPed, float px, float py, float pz, float heading, double now) {
  if (!atomic_load(&zombieEnableCompanion)) return;

  if (companionPed && PG_FN_ENTITY_EXISTS(instance, companionPed)) {
    // Check if dead
    // 0x3317dedb88c95038: IS_PED_DEAD_OR_DYING
    bool isDead = (bool)pg_test_call(instance, 0x3317dedb88c95038ull, 1, (uint64_t[]){ companionPed });
    if (isDead) {
      pg_zombie_remove_blip(instance, &companionBlip);
      pg_zombie_release_ped(instance, &companionPed);
      companionRespawnAtTime = now + 40000; // Respawn after 40s
      pg_zombie_log("[Zombie] Dong doi da bi zombie can guc! Tiep vien se den sau 40 giay.");
      return;
    }

    // Distance to player
    PG_FN_GET_ENTITY_COORDS(instance, zombieCoordVector, companionPed, 1);
    float *cc = (float*)pg_pointer(&env, zombieCoordVector, 24);
    float dx = px - cc[0], dy = py - cc[2];
    float distSq = dx * dx + dy * dy;

    // Auto-teleport if stuck or fell behind (> 40m)
    if (distSq > 40.0f * 40.0f) {
      float *v = (float*)pg_pointer(&env, zombieCoordVector, 24);
      v[0] = px - 2.0f; v[1] = 0; v[2] = py - 2.0f; v[3] = 0; v[4] = pz + 0.5f; v[5] = 0;
      PG_FN_SET_ENTITY_COORDS_NO_OFFSET(instance, companionPed, zombieCoordVector, 0, 0, 0);
      pg_zombie_log("[Zombie] Dong doi bi ket/xa, da duoc dich chuyen lai gan nguoi choi.");
    }

    // Companion combat / follow tick every 1.5s
    if (now - lastCompanionCheckTime > 1500) {
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
        // 0x6a071245eb0d1882: TASK_GO_TO_ENTITY (follow player)
        uint64_t fArgs[7] = { companionPed, playerPed, (uint64_t)-1, pg_zf32(2.5f), pg_zf32(2.0f), 1073741824, 0 };
        pg_test_call(instance, 0x6a071245eb0d1882ull, 7, fArgs);
      }
    }
    return;
  }

  // Spawn companion if dead and respawn timer reached
  if (now < companionRespawnAtTime) return;
  if (!pg_zombie_has_memory()) return;

  PG_FN_REQUEST_MODEL(instance, companionModel);
  if (!PG_FN_HAS_MODEL_LOADED(instance, companionModel)) return;

  float spawnX = px - sinf(heading * 0.0174533f) * 2.5f;
  float spawnY = py + cosf(heading * 0.0174533f) * 2.5f;
  uint64_t pArgs[8] = {
    4, companionModel, pg_zf32(spawnX), pg_zf32(spawnY), pg_zf32(pz + 0.5f), pg_zf32(heading), 0, 1
  };
  // 0xd49f9b0955c367de: CREATE_PED
  companionPed = (u32)pg_test_call(instance, 0xd49f9b0955c367deull, 8, pArgs);

  if (companionPed && PG_FN_ENTITY_EXISTS(instance, companionPed)) {
    PG_FN_SET_ENTITY_HEALTH(instance, companionPed, 350, 0);
    // 0xcea04d83135264cc: SET_PED_ARMOUR
    pg_test_call(instance, 0xcea04d83135264ccull, 2, (uint64_t[]){ companionPed, 100 });
    // 0x7aefb85c1d49deb6: SET_PED_ACCURACY
    pg_test_call(instance, 0x7aefb85c1d49deb6ull, 2, (uint64_t[]){ companionPed, 85 });

    // Arm companion with Shotgun and Pistol
    PG_FN_GIVE_WEAPON(instance, companionPed, pg_joaat("weapon_pumpshotgun"), 500, 0, 1);
    PG_FN_GIVE_WEAPON(instance, companionPed, pg_joaat("weapon_combatpistol"), 500, 0, 0);

    // Group relationship
    // 0xf162e133b4e7a675: GET_PED_GROUP_INDEX
    uint64_t pGroup = pg_test_call(instance, 0xf162e133b4e7a675ull, 1, (uint64_t[]){ playerPed });
    if (!pGroup || pGroup == (uint64_t)-1) {
      // 0x90370ebe0fee1a3d: CREATE_GROUP
      pGroup = pg_test_call(instance, 0x90370ebe0fee1a3dull, 1, (uint64_t[]){ 0 });
    }
    // 0x9f3480fe65db31b5: SET_PED_AS_GROUP_MEMBER
    pg_test_call(instance, 0x9f3480fe65db31b5ull, 2, (uint64_t[]){ companionPed, pGroup });
    // 0x3dbfc55d5c9bb447: SET_PED_NEVER_LEAVES_GROUP
    pg_test_call(instance, 0x3dbfc55d5c9bb447ull, 2, (uint64_t[]){ companionPed, 1 });

    // Blue Radar Blip
    // 0x5cde92c702a8fce7: ADD_BLIP_FOR_ENTITY
    companionBlip = (u32)pg_test_call(instance, 0x5cde92c702a8fce7ull, 1, (uint64_t[]){ companionPed });
    if (companionBlip) {
      pg_test_call(instance, 0x03d7fb09e75d6b7eull, 2, (uint64_t[]){ companionBlip, 3 }); // Blue
      pg_test_call(instance, 0xd38744167b2fa257ull, 2, (uint64_t[]){ companionBlip, pg_zf32(0.85f) });
      pg_test_call(instance, 0xbe8be4fe60e27b72ull, 2, (uint64_t[]){ companionBlip, 0 }); // Global radar
      pg_test_call(instance, 0x9029b2f3da924928ull, 2, (uint64_t[]){ companionBlip, 2 });
    }
    pg_zombie_log("[Zombie] Dong doi song sot da gia nhap doi hinh (Handle %u).", companionPed);
  }
  PG_FN_RELEASE_MODEL(instance, companionModel);
}

// Stage 7: Abandoned Roadside Wrecks
static void pg_zombie_manage_wrecks(w2c_game *instance, float px, float py, float pz, float heading, double now) {
  if (!atomic_load(&zombieEnableWrecks)) return;

  // Clean up distant wrecks (> 70m) - 1 wreck per check
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
  lastWreckSpawnTime = now;

  int freeSlot = -1;
  int currentWreckCount = 0;
  for (int i = 0; i < ZOMBIE_MAX_WRECKS; i++) {
    if (activeWrecks[i]) currentWreckCount++;
    else if (freeSlot < 0) freeSlot = i;
  }
  if (freeSlot < 0 || currentWreckCount >= ZOMBIE_MAX_WRECKS) return;
  if (!pg_zombie_has_memory()) return;

  static unsigned wreckIndex = 0;
  const char *modelName = zombieWreckModels[wreckIndex % 6];
  wreckIndex++;
  uint32_t modelHash = pg_joaat(modelName);

  PG_FN_REQUEST_MODEL(instance, modelHash);
  if (!PG_FN_HAS_MODEL_LOADED(instance, modelHash)) return;

  float dist = 22.0f + (float)(rand() % 20);
  float angOffset = (float)((rand() % 90) - 45) * 0.0174533f;
  float rad = heading * 0.0174533f + angOffset;

  float *v = (float*)pg_pointer(&env, zombieCoordVector, 24);
  v[0] = px - sinf(rad) * dist; v[1] = 0;
  v[2] = py + cosf(rad) * dist; v[3] = 0;
  v[4] = pz + 0.5f; v[5] = 0;

  float wreckHeading = heading + (float)(rand() % 180);
  u32 veh = PG_FN_CREATE_VEHICLE(instance, modelHash, zombieCoordVector, wreckHeading, 0, 0, 1);
  if (veh && PG_FN_ENTITY_EXISTS(instance, veh)) {
    PG_FN_GROUND_VEHICLE(instance, veh, 5.0f);
    PG_FN_VEHICLE_DIRT(instance, veh, 15.0f);
    PG_FN_VEHICLE_ENGINE_HEALTH(instance, veh, -4000.0f);

    // 0x8aba6af54b942b95: SET_VEHICLE_UNDRIVEABLE
    pg_test_call(instance, 0x8aba6af54b942b95ull, 2, (uint64_t[]){ veh, 1 });
    // 0xb664292eaecf7fa6: SET_VEHICLE_DOORS_LOCKED (2 = locked)
    pg_test_call(instance, 0xb664292eaecf7fa6ull, 2, (uint64_t[]){ veh, 2 });
    // 0xec6a202ee4960385: SET_VEHICLE_TYRE_BURST
    pg_test_call(instance, 0xec6a202ee4960385ull, 4, (uint64_t[]){ veh, 0, 1, pg_zf32(1000.0f) });
    pg_test_call(instance, 0xec6a202ee4960385ull, 4, (uint64_t[]){ veh, 4, 1, pg_zf32(1000.0f) });
    // 0x9e5b5e4d2ccd2259: SMASH_VEHICLE_WINDOW
    pg_test_call(instance, 0x9e5b5e4d2ccd2259ull, 2, (uint64_t[]){ veh, 0 });

    activeWrecks[freeSlot] = veh;
  }
  PG_FN_RELEASE_MODEL(instance, modelHash);
}

// Stage 8: Weapon Progression by Kills
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
      pg_zombie_log("[Zombie] Mo khoa Vu khi Cap 1: Sung luc Combat Pistol (5 kills).");
    } else if (targetTier == 2) {
      PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_pumpshotgun"), 35, 0, 1);
      pg_zombie_log("[Zombie] Mo khoa Vu khi Cap 2: Sung san Pump Shotgun (15 kills).");
    } else if (targetTier == 3) {
      PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_microsmg"), 150, 0, 1);
      pg_zombie_log("[Zombie] Mo khoa Vu khi Cap 3: Sung tieu lien Micro SMG (30 kills).");
    } else if (targetTier == 4) {
      PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_carbinerifle"), 200, 0, 1);
      pg_zombie_log("[Zombie] Mo khoa Vu khi Cap 4: Sung truong Carbine Rifle (50 kills).");
    } else if (targetTier == 5) {
      PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_combatmg"), 350, 0, 1);
      PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_assaultshotgun"), 60, 0, 0);
      pg_zombie_log("[Zombie] Mo khoa Vu khi Cap 5: Sung may nang Combat MG (75 kills).");
    }
  }
}

// Stage 8: Zombie Horde Management & AI
static void pg_zombie_manage_horde(w2c_game *instance, u32 playerPed, float px, float py, float pz, float heading, double now) {
  if (!atomic_load(&zombieEnableZombies)) return;

  uint32_t zModel = pg_zombie_get_ped_model(instance);
  // Keep model requested so it streams safely
  PG_FN_REQUEST_MODEL(instance, zModel);

  int livingZombies = 0;

  // Process all active zombies
  for (int i = 0; i < ZOMBIE_MAX_ACTIVE; i++) {
    u32 z = activeZombies[i];
    if (!z) continue;

    if (!PG_FN_ENTITY_EXISTS(instance, z)) {
      pg_zombie_remove_blip(instance, &zombieBlips[i]);
      activeZombies[i] = 0;
      zombieChasing[i] = false;
      continue;
    }

    // 0x3317dedb88c95038: IS_PED_DEAD_OR_DYING
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

    // AI State Machine:
    // When distance <= 25m or damaged by weapon -> Alert and Charge!
    // 0x2d343d2219cd027a: HAS_PED_BEEN_DAMAGED_BY_WEAPON
    bool damaged = (bool)pg_test_call(instance, 0x2d343d2219cd027aull, 3, (uint64_t[]){ z, 0, 2 });
    if ((dist <= 25.0f || damaged) && !zombieChasing[i]) {
      zombieChasing[i] = true;
      u32 target = playerPed;
      if (companionPed && PG_FN_ENTITY_EXISTS(instance, companionPed) && (rand() % 3 == 0)) {
        target = companionPed;
      }
      // 0xf166e48407bac484: TASK_COMBAT_PED
      pg_test_call(instance, 0xf166e48407bac484ull, 4, (uint64_t[]){ z, target, 0, 16 });
      // 0x085bf80fa50a39d1: SET_PED_MOVE_RATE_OVERRIDE (sprint 1.30f)
      pg_test_call(instance, 0x085bf80fa50a39d1ull, 2, (uint64_t[]){ z, pg_zf32(1.30f) });
    }

    // Guttural sound when nearby (4m - 10m) - Restrained (at most once every 4.5s)
    if (dist >= 4.0f && dist <= 10.0f && (now - lastGroanSoundTime > 4500) && atomic_load(&zombieEnableAudio)) {
      lastGroanSoundTime = now;
      // 0xbc9ae166038a5cec: PLAY_PAIN (6 = groan/moan)
      pg_test_call(instance, 0xbc9ae166038a5cecull, 3, (uint64_t[]){ z, 6, 0 });
    }

    // Real bite damage when close (< 1.6m) - at most once every 1.2s
    if (dist < 1.6f && (now - lastBiteDamageTime > 1200)) {
      lastBiteDamageTime = now;
      // 0xeef059fad016d209: GET_ENTITY_HEALTH
      u32 pHealth = (u32)pg_test_call(instance, 0xeef059fad016d209ull, 1, (uint64_t[]){ playerPed });
      if (pHealth > 100) {
        u32 newHealth = (pHealth > 115) ? (pHealth - 15) : 100;
        PG_FN_SET_ENTITY_HEALTH(instance, playerPed, newHealth, 0);

        if (atomic_load(&zombieEnableAudio)) {
          // Play hit sound and bite sound
          pg_test_call(instance, 0xbc9ae166038a5cecull, 3, (uint64_t[]){ playerPed, 8, 0 });
          pg_test_call(instance, 0xbc9ae166038a5cecull, 3, (uint64_t[]){ z, 7, 0 });
        }
      }
    }
  }

  // Rate-limit Spawning: At most 1 zombie per 3.0s, hard capped at ZOMBIE_MAX_ACTIVE (12)
  if (now - lastZombieSpawnTime < 3000) return;
  lastZombieSpawnTime = now;

  if (livingZombies >= ZOMBIE_MAX_ACTIVE) return;
  if (!pg_zombie_has_memory()) return;
  if (!PG_FN_HAS_MODEL_LOADED(instance, zModel)) return;

  int freeSlot = -1;
  for (int i = 0; i < ZOMBIE_MAX_ACTIVE; i++) {
    if (!activeZombies[i]) { freeSlot = i; break; }
  }
  if (freeSlot < 0) return;

  // Spawn outside player camera FOV (behind or rear flanks: 120 - 240 deg offset)
  float fovOffset = (120.0f + (float)(rand() % 120)) * 0.0174533f;
  float rad = heading * 0.0174533f + fovOffset;
  float dist = 24.0f + (float)(rand() % 14); // 24m to 38m away
  float zx = px - sinf(rad) * dist;
  float zy = py + cosf(rad) * dist;

  uint64_t pArgs[8] = {
    4, zModel, pg_zf32(zx), pg_zf32(zy), pg_zf32(pz + 0.5f), pg_zf32(0.0f), 0, 1
  };
  // 0xd49f9b0955c367de: CREATE_PED
  u32 zPed = (u32)pg_test_call(instance, 0xd49f9b0955c367deull, 8, pArgs);
  if (!zPed || !PG_FN_ENTITY_EXISTS(instance, zPed)) return;

  // 1. Disarm (0xf25df915fa38c5f3)
  pg_test_call(instance, 0xf25df915fa38c5f3ull, 2, (uint64_t[]){ zPed, 1 });

  // 2. Fearless (0x9f8aa94d6d97dbf4)
  pg_test_call(instance, 0x9f8aa94d6d97dbf4ull, 2, (uint64_t[]){ zPed, 1 });

  // 3. Flee / Combat attributes (never flee, always fight)
  pg_test_call(instance, 0x70a2d1137c8ed7c9ull, 3, (uint64_t[]){ zPed, 0, 0 });
  pg_test_call(instance, 0x9f7794730795e019ull, 3, (uint64_t[]){ zPed, 46, 1 });

  // 4. Can ragdoll when hit (0xb128377056a54e2a)
  pg_test_call(instance, 0xb128377056a54e2aull, 2, (uint64_t[]){ zPed, 1 });

  // 5. Health: scale slightly with kills (140 base + up to 80 bonus)
  int kills = atomic_load(&zombieKills);
  u32 zHp = 140 + (u32)((kills > 50) ? 50 : kills);
  PG_FN_SET_ENTITY_HEALTH(instance, zPed, zHp, 0);

  // 6. Blood / Bloody wound decals (0x83f7e01c7b769a26: APPLY_PED_BLOOD)
  pg_test_call(instance, 0x83f7e01c7b769a26ull, 5, (uint64_t[]){ zPed, 3, 0, 0, 0 });

  // 7. Initial behavior: Wander aimlessly (0xbb9ce077274f6a1b: TASK_WANDER_STANDARD)
  pg_test_call(instance, 0xbb9ce077274f6a1bull, 3, (uint64_t[]){ zPed, pg_zf32(10.0f), 10 });
  zombieChasing[freeSlot] = false;

  // 8. Red Radar Blip - SET_BLIP_AS_SHORT_RANGE = 0 (global radar visibility)
  u32 zBlip = (u32)pg_test_call(instance, 0x5cde92c702a8fce7ull, 1, (uint64_t[]){ zPed });
  if (zBlip) {
    pg_test_call(instance, 0x03d7fb09e75d6b7eull, 2, (uint64_t[]){ zBlip, 1 }); // 1 = Red
    pg_test_call(instance, 0xd38744167b2fa257ull, 2, (uint64_t[]){ zBlip, pg_zf32(0.70f) });
    pg_test_call(instance, 0xbe8be4fe60e27b72ull, 2, (uint64_t[]){ zBlip, 0 }); // Global radar
    pg_test_call(instance, 0x9029b2f3da924928ull, 2, (uint64_t[]){ zBlip, 2 }); // Map & minimap
  }

  activeZombies[freeSlot] = zPed;
  zombieBlips[freeSlot] = zBlip;
}

// Stage 8: Extraction & City Escape Objective Update
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

  // Inside extraction zone (< 40m from Del Perro Pier rescue point)
  if (distToEvac < 40.0f) {
    if (evacHoldStartTime == 0) {
      evacHoldStartTime = now;
      pg_zombie_log("[Zombie] Nguoi choi da vao vung so tan! Bat dau dem nguoc 45 giay.");
    }
    int secondsLeft = 45 - (int)((now - evacHoldStartTime) / 1000.0);

    if (secondsLeft > 0) {
      snprintf(zombieStatusText, sizeof(zombieStatusText), "DANG CO THU DIEM SO TAN! CON %d GIAY DE THOAT", secondsLeft);
    } else {
      atomic_store(&zombieEscaped, true);
      snprintf(zombieStatusText, sizeof(zombieStatusText), "CHIEN THANG: TRUC THANG CUU HO DA DEN! THOAT THANH CONG!");
      PG_FN_CLEAR_WANTED(instance, PG_FN_PLAYER_ID(instance));
      pg_zombie_log("[Zombie] CHUC MUNG: Ban da song sot va thoat khoi Los Santos thanh cong!");
    }
  } else {
    evacHoldStartTime = 0;
    snprintf(zombieStatusText, sizeof(zombieStatusText), "DIEM SO TAN: %.0f M | KILLS: %d | CAP VU KHI: %d", distToEvac, kills, tier);
  }
}

// Clean up all entities when reset or turned off
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
  pg_zombie_log("[Zombie] Da don dep toan bo entity va dat lai trang thai.");
}

// ============================================================================
// MAIN SCRIPT TICK LOOP
// ============================================================================
static void pg_zombie_tick(w2c_game *instance) {
  if (!atomic_load(&zombieActive)) return;
  if (PGPauseMenuActive() || PGLoadSceneActive() || (PGGameStatus() & 16)) return;

  u32 playerPed = PG_FN_PLAYER_PED_ID(instance);
  if (!playerPed || !PG_FN_ENTITY_EXISTS(instance, playerPed)) return;

  // Staged delayed initialization counter
  zombieFrameCounter++;
  double now = timeMS(CLOCK_MONOTONIC);

  // Allocate persistent coordinate vector once
  if (!zombieCoordVector) zombieCoordVector = pg_asi_alloc(32);
  if (!zombieCoordVector) return;

  // Handle explicit reset request from UI
  if (atomic_exchange(&zombieResetRequested, false)) {
    pg_zombie_cleanup_all(instance);
    zombieCurrentStage = ZOMBIE_STAGE_WEAPONS; // Re-grant loadout
    pg_zombie_apply_initial_loadout(instance, playerPed);
    return;
  }

  // --- STAGE 1: Warmup phase (Frames 0 - 240, ~4s after map ready) ---
  if (zombieCurrentStage == ZOMBIE_STAGE_IDLE) {
    if (zombieFrameCounter > 240) {
      zombieCurrentStage = ZOMBIE_STAGE_TRAFFIC;
      pg_zombie_log("[Zombie] Stage 1 HOAN TAT: Map va streaming da on dinh.");
    } else {
      return;
    }
  }

  // --- STAGE 2: Suppress ambient traffic & cops (Frame 240+, ~4s) ---
  if (zombieCurrentStage == ZOMBIE_STAGE_TRAFFIC) {
    pg_zombie_apply_traffic_suppression(instance);
    zombieCurrentStage = ZOMBIE_STAGE_WEAPONS;
    pg_zombie_log("[Zombie] Stage 2 HOAN TAT: Triet tieu mat do xe co va dan cu thuong.");
  }

  // --- STAGE 3: Initial melee loadout (Frame 360+, ~6s) ---
  if (zombieCurrentStage == ZOMBIE_STAGE_WEAPONS) {
    if (zombieFrameCounter > 360) {
      pg_zombie_apply_initial_loadout(instance, playerPed);
      zombieCurrentStage = ZOMBIE_STAGE_ATMOSPHERE;
      pg_zombie_log("[Zombie] Stage 3 HOAN TAT: Trang bi sinh ton can chien ban dau.");
    }
  }

  // --- STAGE 4: Atmosphere & Weather (Frame 480+, ~8s) ---
  if (zombieCurrentStage == ZOMBIE_STAGE_ATMOSPHERE) {
    if (zombieFrameCounter > 480) {
      pg_zombie_apply_atmosphere(instance);
      lastAtmosphereSyncTime = now;
      zombieCurrentStage = ZOMBIE_STAGE_EXTRACTION;
      pg_zombie_log("[Zombie] Stage 4 HOAN TAT: Khong khi hoang tan 12h trua, troi overcast am u.");
    }
  }

  // --- STAGE 5: Extraction Checkpoint Blip (Frame 600+, ~10s) ---
  if (zombieCurrentStage == ZOMBIE_STAGE_EXTRACTION) {
    if (zombieFrameCounter > 600) {
      pg_zombie_setup_extraction(instance);
      zombieCurrentStage = ZOMBIE_STAGE_COMPANION;
      pg_zombie_log("[Zombie] Stage 5 HOAN TAT: Diem so tan Del Perro Pier da duoc thiet lap.");
    }
  }

  // --- STAGE 6: Companion Survivor (Frame 720+, ~12s) ---
  if (zombieCurrentStage == ZOMBIE_STAGE_COMPANION) {
    if (zombieFrameCounter > 720) {
      zombieCurrentStage = ZOMBIE_STAGE_WRECKS;
      pg_zombie_log("[Zombie] Stage 6 HOAN TAT: Kich hoat he thong dong doi song sot.");
    }
  }

  // --- STAGE 7: Roadside Abandoned Wrecks (Frame 840+, ~14s) ---
  if (zombieCurrentStage == ZOMBIE_STAGE_WRECKS) {
    if (zombieFrameCounter > 840) {
      zombieCurrentStage = ZOMBIE_STAGE_ACTIVE;
      pg_zombie_log("[Zombie] Stage 7 HOAN TAT: Kich hoat he thong xe bo hoang ven duong.");
      pg_zombie_log("[Zombie] TOAN BO MODULE KHOI TAO THANH CONG! Bat dau mo phong sinh ton.");
    }
  }

  // Continuous traffic suppression every 2 frames
  if (zombieFrameCounter % 2 == 0) {
    pg_zombie_apply_traffic_suppression(instance);
  }

  // Atmosphere re-sync every 6s
  if (now - lastAtmosphereSyncTime > 6000) {
    lastAtmosphereSyncTime = now;
    pg_zombie_apply_atmosphere(instance);
  }

  // Player position & heading
  PG_FN_GET_ENTITY_COORDS(instance, zombieCoordVector, playerPed, 1);
  float *pc = (float*)pg_pointer(&env, zombieCoordVector, 24);
  float px = pc[0], py = pc[2], pz = pc[4];
  float heading = PG_FN_GET_ENTITY_HEADING(instance, playerPed);

  // Companion management
  if (zombieCurrentStage >= ZOMBIE_STAGE_COMPANION) {
    pg_zombie_manage_companion(instance, playerPed, px, py, pz, heading, now);
  }

  // Wreck management
  if (zombieCurrentStage >= ZOMBIE_STAGE_WRECKS) {
    pg_zombie_manage_wrecks(instance, px, py, pz, heading, now);
  }

  // Active full zombie horde & escape simulation
  if (zombieCurrentStage >= ZOMBIE_STAGE_ACTIVE) {
    pg_zombie_manage_horde(instance, playerPed, px, py, pz, heading, now);
    pg_zombie_manage_extraction(instance, playerPed, px, py, pz, now);
  }

  // Periodic diagnostic logging (every 10s)
  if (now - lastPeriodicLogTime > 10000) {
    lastPeriodicLogTime = now;
    int livingZ = 0;
    for (int i = 0; i < ZOMBIE_MAX_ACTIVE; i++) if (activeZombies[i]) livingZ++;
    int livingW = 0;
    for (int i = 0; i < ZOMBIE_MAX_WRECKS; i++) if (activeWrecks[i]) livingW++;
    pg_zombie_log("[Zombie Stats] Zombies song: %d/%d | Xe bo hoang: %d/%d | Kills: %d | Cap vu khi: %d | Khoang cach evac: %.0fm",
                  livingZ, ZOMBIE_MAX_ACTIVE, livingW, ZOMBIE_MAX_WRECKS, atomic_load(&zombieKills), atomic_load(&zombieTier), atomic_load(&zombieDistanceToEvac));
  }
}

// ============================================================================
// PUBLIC C APIS (Exposed to Swift & UIKit Settings)
// ============================================================================
bool PGZombieIsActive(void) { return atomic_load(&zombieActive); }
void PGZombieSetActive(bool active) {
  atomic_store(&zombieActive, active);
  if (!active && pg_asi_game) {
    pg_zombie_cleanup_all(pg_asi_game);
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
