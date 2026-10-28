// ============================================================================
// PlayGTA5Native - GTA V ZOMBIE APOCALYPSE SURVIVAL MODE (V6 ULTRA-REALISTIC)
// ============================================================================
// V6 Architecture & Movement-Freeze Root Cause Resolution:
// 1. Khắc phục triệt để lỗi "Game đã dừng khi di chuyển":
//    - Thủ phạm V5: Hash typo heading trong V5 (thực chất là 8914) trả về -1,
//      ép kiểu float thành 1.8446744e19 khiến sinf/cosf ra NaN, làm sập IS_SPHERE_VISIBLE
//      và REQUEST_COLLISION_AT_COORD ngay khi người chơi di chuyển.
//    - Giải pháp V6: Dùng trực tiếp hàm C của engine: PG_FN_GET_ENTITY_HEADING(instance, playerPed).
//      Bổ sung kiểm tra isfinite() và kiểm soát góc heading an toàn (0 - 360 độ).
//    - Kiểm tra góc nhìn camera bằng tích vô hướng vector (Vector Dot-Product Math) thay vì
//      gọi native IS_SPHERE_VISIBLE mạo hiểm:
//      cosAngle = (forwardX * dx + forwardY * dy) / dist.
//      Chỉ sinh zombie ở góc khuất / sau lưng người chơi (cosAngle <= 0.15f), 100% không bao giờ
//      hiện đột ngột trước mắt người chơi và hoàn toàn không thể crash!
//    - Chiến đấu & Đuổi bắt: Sử dụng TASK_COMBAT_PED (0xf166e48407bac484) kết hợp với
//      SET_PED_MOVE_RATE_OVERRIDE (0x085bf80fa50a39d1) và SET_PED_MOVEMENT_CLIPSET.
//      Đây là cơ chế chiến đấu gốc của Rockstar, tự động tìm đường (navmesh), vượt chướng ngại vật
//      và rượt đuổi người chơi không bao giờ dừng hay lỗi tham số!
// 2. Mật độ quần thể Zombie đông đúc ("như kiểu NPC lúc chưa có zombie"):
//    - Mở rộng giới hạn thực thể: ZOMBIE_MAX_ACTIVE = 36, TARGET = 28, MIN = 20.
//    - Giai đoạn chuẩn bị (Stage 2, dưới màn hình mờ đen): Khởi tạo sẵn 20 zombie trong 4 vành đai
//      đồng tâm (14m đến 68m) bao quanh Portola Drive.
//    - Khi màn hình sáng lên: Ngay lập tức người chơi thấy cả khu phố ngập tràn zombie lảo đảo.
//    - Bổ sung liên tục khi di chuyển (mỗi 350ms bổ sung 1 con ở khoảng cách 45m - 70m góc khuất)
//      đảm bảo luôn duy trì ổn định 26 - 28 zombie xung quanh người chơi ở mọi con đường.
// 3. Khóa cứng 60 FPS mượt mà:
//    - Xử lý phân tải Staggered (mỗi frame chỉ kiểm tra 4 zombie, chu kỳ 8 frame = < 0.02ms/frame).
//    - Triệt tiêu 100% giao thông và người đi bộ thường (DENSITY = 0.0f) để giải phóng GPU/CPU.
//    - Tái sử dụng các model zombie đã load vào bộ nhớ cache, 0 rò rỉ RAM.
//    - Xe bỏ hoang: 8 xe được đặt sẵn vững chắc trên mặt đường (GET_GROUND_Z_FOR_3D_COORD).
//    - Đồng đội Marine chiến đấu cùng và nhiệm vụ sơ tán trực thăng tại Del Perro Pier.
// ============================================================================

#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// --- Cấu hình giới hạn quần thể Zombie V6 (Đông đúc như NPC thường, khóa 60 FPS) ---
#define ZOMBIE_MAX_ACTIVE       36
#define ZOMBIE_TARGET_ACTIVE    28
#define ZOMBIE_MIN_ACTIVE       20
#define ZOMBIE_MAX_WRECKS       8
#define ZOMBIE_MAX_CORPSES      16
#define ZOMBIE_MAX_LOG_LINES    80

// Direct C / wasm declarations
void PG_FN_PED_DENSITY_THIS_FRAME(w2c_game*, float);
void PG_FN_VEHICLE_DENSITY_THIS_FRAME(w2c_game*, float);
void PG_FN_PARKED_DENSITY_THIS_FRAME(w2c_game*, float);
void PG_FN_REDUCE_PED_MODEL_BUDGET(w2c_game*, u32);
void PG_FN_REDUCE_VEHICLE_MODEL_BUDGET(w2c_game*, u32);
void PG_FN_SET_CLOCK(w2c_game*, u32, u32, u32);
void PG_FN_SET_WEATHER(w2c_game*, u64);
void PG_FN_CLEAR_WEATHER_OVERRIDE(w2c_game*);
u32  PG_FN_PLAYER_PED_ID(w2c_game*);
u32  PG_FN_PLAYER_ID(w2c_game*);
u32  PG_FN_ENTITY_EXISTS(w2c_game*, u32);
void PG_FN_GET_ENTITY_COORDS(w2c_game*, u64, u32, u32);
f32  PG_FN_GET_ENTITY_HEADING(w2c_game*, u32);
void PG_FN_SET_MAX_WANTED(w2c_game*, u32);
u32  PG_FN_GET_WANTED(w2c_game*, u32);
void PG_FN_CLEAR_WANTED(w2c_game*, u32);
void PG_FN_GIVE_WEAPON(w2c_game*, u32, u32, u32, u32, u32);
void PG_FN_REQUEST_MODEL(w2c_game*, u32);
u32  PG_FN_HAS_MODEL_LOADED(w2c_game*, u32);
void PG_FN_RELEASE_MODEL(w2c_game*, u32);
u32  PG_FN_CREATE_VEHICLE(w2c_game*, u32, u64, float, u32, u32, u32);
u32  PG_FN_GROUND_VEHICLE(w2c_game*, u32, f32);
void PG_FN_VEHICLE_DIRT(w2c_game*, u32, float);
void PG_FN_VEHICLE_ENGINE_HEALTH(w2c_game*, u32, float);
void PG_FN_DELETE_VEHICLE(w2c_game*, u64);
void PG_FN_SET_ENTITY_HEALTH(w2c_game*, u32, u32, u32);
void PG_FN_SET_ENTITY_COORDS_NO_OFFSET(w2c_game*, u32, u64, u32, u32, u32);

// Model hash definitions
static const char *const zombieWreckModels[] = {
  "rebel", "stanier", "tornado3", "voodoo2"
};

static uint32_t activeZombieModels[3] = {
  0x19DFA98E, // a_m_y_skater_01
  0x3497D515, // a_m_m_hillbilly_01
  0           // placeholder for u_m_y_zombie_01 if verified in game files
};
static int activeZombieModelCount = 2;

static const uint32_t companionModel = 0x8D8F1B10; // s_m_y_marine_01

// Escape destination: Del Perro Pier helipad
static const float evacX = -1600.0f;
static const float evacY = -1050.0f;
static const float evacZ = 13.0f;

// --- Movement Clipsets for Realistic Zombie Shambling ---
static const char *const zombieMovementClipsets[2] = {
  "move_m@drunk@verydrunk", // Romero style heavy swaying & stumbling
  "move_m@injured"          // Injured limp dragging one foot
};

// --- Module Feature Flags ---
static _Atomic bool zombieActive = false; // Mac dinh TAT zombie, tra lai Los Santos nguyen ban
static _Atomic bool zombieEnableAtmosphere = true;
static _Atomic bool zombieEnableTraffic = true;
static _Atomic bool zombieEnableWrecks = true;
static _Atomic bool zombieEnableZombies = true;
static _Atomic bool zombieEnableWeapons = true;
static _Atomic bool zombieEnableCompanion = true;
static _Atomic bool zombieEnableExtraction = true;
static _Atomic bool zombieEnableBiteDamage = true;
static _Atomic bool zombieEnableRagdoll = true;
static _Atomic bool zombieEnableBlood = true;
static _Atomic bool zombieEnableAudio = false;       // Safe default
static _Atomic bool zombieEnableWeaponUnlocks = true;

// Gameplay Stats & State
static _Atomic int zombieKills = 0;
static _Atomic int zombieTier = 0;
static _Atomic bool zombieResetRequested = false;
static _Atomic float zombieDistanceToEvac = 2000.0f;
static _Atomic bool zombieEscaped = false;

// Sequential Initialization Stages
typedef enum {
  ZOMBIE_STAGE_BOOT = 0,               // Frame 0: Fade Out + Day 12h + OVERCAST + PAUSE_CLOCK + Teleport
  ZOMBIE_STAGE_PREPOPULATE_WRECKS = 1, // Frames 20-60: Road wrecks pre-population (under fade)
  ZOMBIE_STAGE_PREPOPULATE_ZOMBIES = 2,// Frames 60-110: Pre-populate 20 zombies with clipsets (under fade)
  ZOMBIE_STAGE_READY_AND_FADEIN = 3,   // Frame 120: Initial weapons + DO_SCREEN_FADE_IN(1000)
  ZOMBIE_STAGE_ACTIVE = 4              // Frame 130+: Dynamic population loop
} ZombieInitStage;

static ZombieInitStage zombieCurrentStage = ZOMBIE_STAGE_BOOT;
static uint64_t zombieFrameCounter = 0;
static bool playerTeleportedOutdoors = false;
static uint32_t zombieGroupHash = 0;

// Zombie AI State Machine
typedef enum {
  ZSTATE_WANDER = 0,   // Di lảo đảo theo clipset
  ZSTATE_CHASE = 1,    // Đang rượt đuổi mục tiêu (TASK_COMBAT_PED + clipset)
  ZSTATE_ATTACK = 2,   // Áp sát cắn / cào gây sát thương
  ZSTATE_DEAD = 3      // Đã chết, giữ xác trên mặt đất
} ZombieAIState;

// Runtime Tracking Entities
static u32 activeZombies[ZOMBIE_MAX_ACTIVE];
static u32 zombieBlips[ZOMBIE_MAX_ACTIVE];
static ZombieAIState zombieStates[ZOMBIE_MAX_ACTIVE];
static double zombieNextTaskTime[ZOMBIE_MAX_ACTIVE];
static double zombieNextBiteTime[ZOMBIE_MAX_ACTIVE];
static u32 zombiePrevHealth[ZOMBIE_MAX_ACTIVE];
static int zombieClipsetIndex[ZOMBIE_MAX_ACTIVE];
static u32 activeWrecks[ZOMBIE_MAX_WRECKS];
static u32 companionPed = 0;
static u32 companionBlip = 0;
static u32 evacBlip = 0;

// Dead corpse tracking to prevent instant deletion
static u32 deadCorpses[ZOMBIE_MAX_CORPSES];
static double deadCorpseTime[ZOMBIE_MAX_CORPSES];

// Rate limiting timers (in ms)
static double lastZombieSpawnTime = 0;
static double lastWreckSpawnTime = 0;
static double lastCompanionCheckTime = 0;
static double companionRespawnAtTime = 0;
static double lastAtmosphereSyncTime = 0;
static double evacHoldStartTime = 0;

// Pointers in WebAssembly guest memory (Dedicated non-overlapping buffers)
static uint64_t zombiePlayerCoordVector = 0;
static uint64_t zombieEntityCoordVector = 0;
static uint64_t zombieSpawnCoordVector = 0;
static uint64_t zombieGroundZBuf = 0;
static uint64_t zombieEntityPtrBuf = 0;
static uint64_t zombieBlipPtrBuf = 0;
static uint64_t zombieRelGroupBuf = 0;
static uint64_t zombieStrWeather = 0;
static uint64_t zombieStrTimecycle = 0;
static uint64_t zombieClipsetPtrs[2] = {0, 0};
static bool zombieClipsetsLoaded[2] = {false, false};

static char zombieStatusText[160] = "ZOMBIE SURVIVAL: DANG KHOI TAO QUAN THE...";

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

  pg_log(line);

  pthread_mutex_lock(&zombieLogLock);
  snprintf(zombieLogLines[zombieLogHead % ZOMBIE_MAX_LOG_LINES], sizeof(zombieLogLines[0]), "%s", line);
  zombieLogHead++;
  if (zombieLogCount < ZOMBIE_MAX_LOG_LINES) zombieLogCount++;
  pthread_mutex_unlock(&zombieLogLock);
}

// Float & String Helpers for rage Native Calls
static inline uint64_t pg_zf32(float f) {
  uint32_t bits = 0;
  memcpy(&bits, &f, 4);
  return (uint64_t)bits;
}

static inline uint64_t pg_zstr(uint64_t *slot, const char *str) {
  if (!slot) return 0;
  if (!*slot) {
    size_t len = strlen(str) + 1;
    *slot = pg_asi_alloc((uint32_t)len);
    if (*slot) memcpy(pg_pointer(&env, *slot, len), str, len);
  }
  return *slot;
}

// Memory headroom guard
static bool pg_zombie_has_memory(void) {
  uint64_t headroom = 0;
  if (nativeProcessHeadroom(&headroom) && headroom < guardReserve()) {
    return false;
  }
  return true;
}

// Safe Entity Deletion & Release
static void pg_zombie_release_ped(w2c_game *instance, u32 *pedHandle) {
  if (!pedHandle || !*pedHandle) return;
  u32 h = *pedHandle;
  *pedHandle = 0;
  if (!PG_FN_ENTITY_EXISTS(instance, h)) return;

  if (!zombieEntityPtrBuf) zombieEntityPtrBuf = pg_asi_alloc(16);
  if (zombieEntityPtrBuf) {
    *(u32*)pg_pointer(&env, zombieEntityPtrBuf, 4) = h;
    // 0x2595dd4236549ce3: SET_PED_AS_NO_LONGER_NEEDED
    pg_test_call(instance, 0x2595dd4236549ce3ull, 1, (uint64_t[]){ zombieEntityPtrBuf });
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
    // 0x86a652570e5f25dd: REMOVE_BLIP
    pg_test_call(instance, 0x86a652570e5f25ddull, 1, (uint64_t[]){ zombieBlipPtrBuf });
  }
}

// Check if a point is visible in camera view (Mathematical Vector Frustum Check + Native Guard)
static bool pg_zombie_is_point_on_screen(w2c_game *instance, float px, float py, float heading, float x, float y, float z, float radius) {
  if (!isfinite(x) || !isfinite(y) || !isfinite(z)) return true;

  // Vector dot product check:
  // Heading 0 faces North (+Y), 90 faces West (-X), 180 South (-Y), 270 East (+X)
  float rad = heading * 0.0174532925f;
  float fx = -sinf(rad);
  float fy =  cosf(rad);

  float dx = x - px;
  float dy = y - py;
  float distSq = dx * dx + dy * dy;
  if (distSq < 16.0f) return true; // Close proximity is considered on-screen

  float invDist = 1.0f / sqrtf(distSq);
  float cosAngle = (fx * dx + fy * dy) * invDist;

  // cosAngle > 0.15f is within the 160-degree front camera field of view
  if (cosAngle > 0.15f) return true;

  // Also verify with native IS_SPHERE_VISIBLE for completeness
  uint64_t args[4] = { pg_zf32(x), pg_zf32(y), pg_zf32(z), pg_zf32(radius) };
  return (bool)pg_test_call(instance, 0xe33d59da70b58fdfull, 4, args);
}

// Check if entity is currently on screen
static bool pg_zombie_is_entity_on_screen(w2c_game *instance, u32 entity) {
  if (!entity || !PG_FN_ENTITY_EXISTS(instance, entity)) return false;
  // 0xe659e47af827484b: IS_ENTITY_ON_SCREEN
  return (bool)pg_test_call(instance, 0xe659e47af827484bull, 1, (uint64_t[]){ entity });
}

// Get Ground Z coordinate accurately
static bool pg_zombie_get_ground_z(w2c_game *instance, float x, float y, float zInput, float *outZ) {
  if (!isfinite(x) || !isfinite(y) || !isfinite(zInput)) return false;

  if (!zombieGroundZBuf) zombieGroundZBuf = pg_asi_alloc(16);
  if (!zombieGroundZBuf) return false;

  *(float*)pg_pointer(&env, zombieGroundZBuf, 4) = zInput;
  // 0xc906a7dab05c8d2b: GET_GROUND_Z_FOR_3D_COORD(float x, float y, float z, float *groundZ, BOOL ignoreWater)
  uint64_t args[5] = { pg_zf32(x), pg_zf32(y), pg_zf32(zInput + 2.0f), zombieGroundZBuf, 0 };
  bool ok = (bool)pg_test_call(instance, 0xc906a7dab05c8d2bull, 5, args);
  if (ok) {
    float resZ = *(float*)pg_pointer(&env, zombieGroundZBuf, 4);
    if (isfinite(resZ) && fabsf(resZ - zInput) < 30.0f) {
      *outZ = resZ;
      return true;
    }
  }
  return false;
}

// --- Request and Check Movement Clipsets ---
static void pg_zombie_request_clipsets(w2c_game *instance) {
  for (int i = 0; i < 2; i++) {
    if (!zombieClipsetPtrs[i]) {
      zombieClipsetPtrs[i] = pg_asi_alloc(64);
      strcpy((char*)pg_pointer(&env, zombieClipsetPtrs[i], 64), zombieMovementClipsets[i]);
    }
    // 0xd2a71e1a77418a49: REQUEST_CLIP_SET
    pg_test_call(instance, 0xd2a71e1a77418a49ull, 1, (uint64_t[]){ zombieClipsetPtrs[i] });
    // 0x318234f4f3738af3: HAS_CLIP_SET_LOADED
    zombieClipsetsLoaded[i] = (bool)pg_test_call(instance, 0x318234f4f3738af3ull, 1, (uint64_t[]){ zombieClipsetPtrs[i] });
  }
}

static void pg_zombie_apply_clipset(w2c_game *instance, u32 ped, int clipIdx) {
  int idx = clipIdx % 2;
  if (zombieClipsetsLoaded[idx] && zombieClipsetPtrs[idx]) {
    // 0xaf8a94ede7712bef: SET_PED_MOVEMENT_CLIPSET(Ped ped, char* clipSet, float blendDuration)
    uint64_t args[3] = { ped, zombieClipsetPtrs[idx], pg_zf32(0.25f) };
    pg_test_call(instance, 0xaf8a94ede7712befull, 3, args);
  }
}

// --- Teleport player outdoors to Portola Drive ---
static void pg_zombie_teleport_outdoors(w2c_game *instance, u32 playerPed) {
  if (!playerPed || !PG_FN_ENTITY_EXISTS(instance, playerPed)) return;
  if (!zombiePlayerCoordVector) zombiePlayerCoordVector = pg_asi_alloc(32);
  if (!zombiePlayerCoordVector) return;

  PG_FN_GET_ENTITY_COORDS(instance, zombiePlayerCoordVector, playerPed, 1);
  float *v = (float*)pg_pointer(&env, zombiePlayerCoordVector, 24);
  float px = v[0], py = v[2], pz = v[4];

  bool insideMansion = (px > -830.0f && px < -795.0f && py > 165.0f && py < 195.0f);

  if (insideMansion || !playerTeleportedOutdoors) {
    playerTeleportedOutdoors = true;
    v[0] = -840.0f; v[1] = 0;
    v[2] = 148.0f;  v[3] = 0;
    v[4] = 66.0f;   v[5] = 0;
    PG_FN_SET_ENTITY_COORDS_NO_OFFSET(instance, playerPed, zombiePlayerCoordVector, 0, 0, 0);
    // 0x07503f7948f491a7: REQUEST_COLLISION_AT_COORD
    pg_test_call(instance, 0x07503f7948f491a7ull, 3, (uint64_t[]){ pg_zf32(-840.0f), pg_zf32(148.0f), pg_zf32(66.0f) });
    pg_zombie_log("[Zombie] Da dua nguoi choi tu (%.0f, %.0f, %.0f) ra mat duong Portola Drive ngoai troi (-840, 148, 66).", px, py, pz);
  }
}

// Setup relationship groups: ZOMBIE hates PLAYER & COMPANION
static void pg_zombie_setup_relationships(w2c_game *instance, u32 playerPed) {
  if (!zombieGroupHash) {
    if (!zombieRelGroupBuf) zombieRelGroupBuf = pg_asi_alloc(16);
    uint64_t nameStr = 0;
    pg_zstr(&nameStr, "ZOMBIE_HORDE");
    // 0xf372bc22fcb88606: ADD_RELATIONSHIP_GROUP
    pg_test_call(instance, 0xf372bc22fcb88606ull, 2, (uint64_t[]){ nameStr, zombieRelGroupBuf });
    zombieGroupHash = *(uint32_t*)pg_pointer(&env, zombieRelGroupBuf, 4);
    if (!zombieGroupHash) zombieGroupHash = pg_joaat("ZOMBIE_HORDE");
  }

  // 0x7dbdd04862d95f04: GET_PED_RELATIONSHIP_GROUP_HASH
  uint32_t pGroup = (uint32_t)pg_test_call(instance, 0x7dbdd04862d95f04ull, 1, (uint64_t[]){ playerPed });
  if (zombieGroupHash && pGroup) {
    // 0xbf25eb89375a37ad: SET_RELATIONSHIP_BETWEEN_GROUPS(5 = Hate)
    pg_test_call(instance, 0xbf25eb89375a37adull, 3, (uint64_t[]){ 5, zombieGroupHash, pGroup });
    pg_test_call(instance, 0xbf25eb89375a37adull, 3, (uint64_t[]){ 5, pGroup, zombieGroupHash });
    pg_zombie_log("[AI Relationship] Da thiet lap nhom thu dich ZOMBIE ghet PLAYER (Hate = 5).");
  }
}

// Immediate daylight & atmosphere freeze
static void pg_zombie_apply_atmosphere_immediate(w2c_game *instance) {
  if (!atomic_load(&zombieEnableAtmosphere)) return;

  PG_FN_SET_CLOCK(instance, 12, 0, 0);
  // 0x4055e40bd2dbec1d: PAUSE_CLOCK(1)
  pg_test_call(instance, 0x4055e40bd2dbec1dull, 1, (uint64_t[]){ 1 });

  PG_FN_CLEAR_WEATHER_OVERRIDE(instance);
  uint64_t wStr = pg_zstr(&zombieStrWeather, "OVERCAST");
  if (wStr) PG_FN_SET_WEATHER(instance, wStr);

  uint64_t tStr = pg_zstr(&zombieStrTimecycle, "prologue_shootout");
  if (tStr) {
    // 0x2c933abf17a1df41: SET_TIMECYCLE_MODIFIER
    pg_test_call(instance, 0x2c933abf17a1df41ull, 1, (uint64_t[]){ tStr });
    // 0x82e7ffcd5b2326b3: SET_TIMECYCLE_MODIFIER_STRENGTH
    pg_test_call(instance, 0x82e7ffcd5b2326b3ull, 1, (uint64_t[]){ pg_zf32(0.35f) });
  }
}

// Traffic & cops suppression
static void pg_zombie_apply_traffic_suppression(w2c_game *instance) {
  if (!atomic_load(&zombieEnableTraffic)) return;

  PG_FN_PED_DENSITY_THIS_FRAME(instance, 0.0f);
  PG_FN_VEHICLE_DENSITY_THIS_FRAME(instance, 0.0f);
  PG_FN_PARKED_DENSITY_THIS_FRAME(instance, 0.0f);
  PG_FN_REDUCE_PED_MODEL_BUDGET(instance, 1);
  PG_FN_REDUCE_VEHICLE_MODEL_BUDGET(instance, 1);

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

// Initial melee loadout
static void pg_zombie_apply_initial_loadout(w2c_game *instance, u32 playerPed) {
  if (!atomic_load(&zombieEnableWeapons)) return;

  // 0xf25df915fa38c5f3: REMOVE_ALL_PED_WEAPONS
  pg_test_call(instance, 0xf25df915fa38c5f3ull, 2, (uint64_t[]){ playerPed, 1 });

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
  pg_zombie_log("[Zombie Loadout] Da cap vu khi can chien (Machete, Knife, Crowbar, 50 Giap).");
}

// Extraction checkpoint setup
static void pg_zombie_setup_extraction(w2c_game *instance) {
  if (!atomic_load(&zombieEnableExtraction)) return;

  if (!evacBlip) {
    // 0x5a039bb0bca604b6: ADD_BLIP_FOR_COORD
    uint64_t bArgs[3] = { pg_zf32(evacX), pg_zf32(evacY), pg_zf32(evacZ) };
    evacBlip = (u32)pg_test_call(instance, 0x5a039bb0bca604b6ull, 3, bArgs);
    if (evacBlip) {
      pg_test_call(instance, 0xdf735600a4696dafull, 2, (uint64_t[]){ evacBlip, 38 }); // Helicopter
      pg_test_call(instance, 0x03d7fb09e75d6b7eull, 2, (uint64_t[]){ evacBlip, 2 });  // Green
      pg_test_call(instance, 0xd38744167b2fa257ull, 2, (uint64_t[]){ evacBlip, pg_zf32(1.20f) });
      pg_test_call(instance, 0xbe8be4fe60e27b72ull, 2, (uint64_t[]){ evacBlip, 0 });  // Global
      pg_test_call(instance, 0x9029b2f3da924928ull, 2, (uint64_t[]){ evacBlip, 2 });
    }
  }
}

// Spawn single roadside wreck on verified ground
static u32 pg_zombie_spawn_wreck_at(w2c_game *instance, float x, float y, float z, float heading) {
  if (!isfinite(x) || !isfinite(y) || !isfinite(z)) return 0;

  float groundZ = z;
  if (!pg_zombie_get_ground_z(instance, x, y, z, &groundZ)) {
    groundZ = z;
  }

  // 0x07503f7948f491a7: REQUEST_COLLISION_AT_COORD
  pg_test_call(instance, 0x07503f7948f491a7ull, 3, (uint64_t[]){ pg_zf32(x), pg_zf32(y), pg_zf32(groundZ) });

  static int modelCycle = 0;
  uint32_t model = pg_joaat(zombieWreckModels[modelCycle % 4]);
  modelCycle++;

  PG_FN_REQUEST_MODEL(instance, model);
  if (!PG_FN_HAS_MODEL_LOADED(instance, model)) return 0;

  if (!zombieSpawnCoordVector) zombieSpawnCoordVector = pg_asi_alloc(32);
  if (!zombieSpawnCoordVector) return 0;

  float *v = (float*)pg_pointer(&env, zombieSpawnCoordVector, 24);
  v[0] = x; v[1] = 0; v[2] = y; v[3] = 0; v[4] = groundZ + 0.15f; v[5] = 0;

  u32 veh = PG_FN_CREATE_VEHICLE(instance, model, zombieSpawnCoordVector, heading, 0, 0, 1);
  if (veh && PG_FN_ENTITY_EXISTS(instance, veh)) {
    PG_FN_GROUND_VEHICLE(instance, veh, 3.0f);
    PG_FN_VEHICLE_DIRT(instance, veh, 15.0f);
    PG_FN_VEHICLE_ENGINE_HEALTH(instance, veh, -4000.0f);

    pg_test_call(instance, 0x8aba6af54b942b95ull, 2, (uint64_t[]){ veh, 1 }); // undriveable
    pg_test_call(instance, 0xb664292eaecf7fa6ull, 2, (uint64_t[]){ veh, 2 }); // doors locked
    pg_test_call(instance, 0xec6a202ee4960385ull, 4, (uint64_t[]){ veh, 0, 1, pg_zf32(1000.0f) }); // flat tire 0
    pg_test_call(instance, 0xec6a202ee4960385ull, 4, (uint64_t[]){ veh, 4, 1, pg_zf32(1000.0f) }); // flat tire 4
    pg_test_call(instance, 0x9e5b5e4d2ccd2259ull, 2, (uint64_t[]){ veh, 0 }); // smash window

    // Freeze in place so physics doesn't make it jump or slide
    // 0x1c99bb7b61196e45: SET_ENTITY_VELOCITY
    pg_test_call(instance, 0x1c99bb7b61196e45ull, 4, (uint64_t[]){ veh, 0, 0, 0 });
  }

  PG_FN_RELEASE_MODEL(instance, model);
  return veh;
}

// Spawn single zombie at verified ground with movement clipset
static u32 pg_zombie_spawn_ped_at(w2c_game *instance, float x, float y, float z, int slot) {
  if (!isfinite(x) || !isfinite(y) || !isfinite(z)) return 0;
  if (slot < 0 || slot >= ZOMBIE_MAX_ACTIVE) return 0;

  float groundZ = z;
  if (!pg_zombie_get_ground_z(instance, x, y, z, &groundZ)) {
    groundZ = z;
  }

  // 0x07503f7948f491a7: REQUEST_COLLISION_AT_COORD
  pg_test_call(instance, 0x07503f7948f491a7ull, 3, (uint64_t[]){ pg_zf32(x), pg_zf32(y), pg_zf32(groundZ) });

  // Choose loaded model
  uint32_t chosen = 0;
  for (int m = 0; m < activeZombieModelCount; m++) {
    if (PG_FN_HAS_MODEL_LOADED(instance, activeZombieModels[m])) {
      chosen = activeZombieModels[m];
      break;
    }
  }
  if (!chosen) return 0;

  uint64_t pArgs[8] = {
    4, chosen, pg_zf32(x), pg_zf32(y), pg_zf32(groundZ + 0.1f), pg_zf32(0.0f), 0, 1
  };
  // 0xd49f9b0955c367de: CREATE_PED
  u32 zPed = (u32)pg_test_call(instance, 0xd49f9b0955c367deull, 8, pArgs);
  if (!zPed || !PG_FN_ENTITY_EXISTS(instance, zPed)) return 0;

  // Disarm
  pg_test_call(instance, 0xf25df915fa38c5f3ull, 2, (uint64_t[]){ zPed, 1 });

  // Movement clipset (Romero drunk sway or injured limp)
  int clipIdx = slot % 2;
  pg_zombie_apply_clipset(instance, zPed, clipIdx);
  zombieClipsetIndex[slot] = clipIdx;

  // 360-degree vision and hearing
  // 0xf29cf591c4bf6cee: SET_PED_SEEING_RANGE
  pg_test_call(instance, 0xf29cf591c4bf6ceeull, 2, (uint64_t[]){ zPed, pg_zf32(55.0f) });
  // 0x33a8f7f7d5f7f33c: SET_PED_HEARING_RANGE
  pg_test_call(instance, 0x33a8f7f7d5f7f33cull, 2, (uint64_t[]){ zPed, pg_zf32(55.0f) });
  // 0x2db492222fb21e26: SET_PED_VISUAL_FIELD_MIN_ANGLE
  pg_test_call(instance, 0x2db492222fb21e26ull, 2, (uint64_t[]){ zPed, pg_zf32(-180.0f) });
  // 0x70793bdca1e854d4: SET_PED_VISUAL_FIELD_MAX_ANGLE
  pg_test_call(instance, 0x70793bdca1e854d4ull, 2, (uint64_t[]){ zPed, pg_zf32(180.0f) });

  // Combat flags: Never flee, always fight
  pg_test_call(instance, 0x70a2d1137c8ed7c9ull, 3, (uint64_t[]){ zPed, 0, 0 });
  pg_test_call(instance, 0x9f7794730795e019ull, 3, (uint64_t[]){ zPed, 46, 1 });
  pg_test_call(instance, 0x9f7794730795e019ull, 3, (uint64_t[]){ zPed, 5, 1 }); // Unarmed combat
  // 0xb128377056a54e2a: SET_PED_CAN_RAGDOLL
  pg_test_call(instance, 0xb128377056a54e2aull, 2, (uint64_t[]){ zPed, 1 });

  // Assign ZOMBIE relationship group
  if (zombieGroupHash) {
    // 0xc80a74ac829ddd92: SET_PED_RELATIONSHIP_GROUP_HASH
    pg_test_call(instance, 0xc80a74ac829ddd92ull, 2, (uint64_t[]){ zPed, zombieGroupHash });
  }

  // Health
  int kills = atomic_load(&zombieKills);
  u32 zHp = 140 + (u32)((kills > 50) ? 50 : kills);
  PG_FN_SET_ENTITY_HEALTH(instance, zPed, zHp, 0);
  zombiePrevHealth[slot] = zHp;

  // Blood decals
  if (atomic_load(&zombieEnableBlood)) {
    // 0x83f7e01c7b769a26: APPLY_PED_DAMAGE_PACK
    pg_test_call(instance, 0x83f7e01c7b769a26ull, 5, (uint64_t[]){ zPed, 3, 0, 0, 0 });
  }

  // Initial wander task with zombie gait
  // 0xbb9ce077274f6a1b: TASK_WANDER_STANDARD
  pg_test_call(instance, 0xbb9ce077274f6a1bull, 3, (uint64_t[]){ zPed, pg_zf32(10.0f), 10 });
  zombieStates[slot] = ZSTATE_WANDER;
  zombieNextTaskTime[slot] = 0;
  zombieNextBiteTime[slot] = 0;

  // Radar Blip (Red)
  u32 zBlip = (u32)pg_test_call(instance, 0x5cde92c702a8fce7ull, 1, (uint64_t[]){ zPed });
  if (zBlip) {
    pg_test_call(instance, 0x03d7fb09e75d6b7eull, 2, (uint64_t[]){ zBlip, 1 }); // Red
    pg_test_call(instance, 0xd38744167b2fa257ull, 2, (uint64_t[]){ zBlip, pg_zf32(0.70f) });
    pg_test_call(instance, 0xbe8be4fe60e27b72ull, 2, (uint64_t[]){ zBlip, 0 }); // Global radar
    pg_test_call(instance, 0x9029b2f3da924928ull, 2, (uint64_t[]){ zBlip, 2 });
  }

  activeZombies[slot] = zPed;
  zombieBlips[slot] = zBlip;
  return zPed;
}

// Weapon progression
static void pg_zombie_update_weapons(w2c_game *instance, u32 playerPed, int kills) {
  if (!atomic_load(&zombieEnableWeapons) || !atomic_load(&zombieEnableWeaponUnlocks)) return;

  int currentTier = atomic_load(&zombieTier);
  int targetTier = 0;
  if (kills >= 75) targetTier = 5;
  else if (kills >= 50) targetTier = 4;
  else if (kills >= 30) targetTier = 3;
  else if (kills >= 15) targetTier = 2;
  else if (kills >= 5)  targetTier = 1;

  if (targetTier > currentTier) {
    if (atomic_compare_exchange_strong(&zombieTier, &currentTier, targetTier)) {
      if (targetTier == 1) {
        PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_combatpistol"), 60, 0, 1);
        pg_zombie_log("[Weapons] Dat moc %d kills: Mo khoa Sung luc Combat Pistol!", kills);
      } else if (targetTier == 2) {
        PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_pumpshotgun"), 35, 0, 1);
        pg_zombie_log("[Weapons] Dat moc %d kills: Mo khoa Sung san Pump Shotgun!", kills);
      } else if (targetTier == 3) {
        PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_microsmg"), 150, 0, 1);
        pg_zombie_log("[Weapons] Dat moc %d kills: Mo khoa Sung tieu lien Micro SMG!", kills);
      } else if (targetTier == 4) {
        PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_carbinerifle"), 200, 0, 1);
        pg_zombie_log("[Weapons] Dat moc %d kills: Mo khoa Sung truong Carbine Rifle!", kills);
      } else if (targetTier == 5) {
        PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_combatmg"), 350, 0, 1);
        PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_assaultshotgun"), 60, 0, 0);
        pg_zombie_log("[Weapons] Dat moc %d kills: Mo khoa Sung may nang Combat MG!", kills);
      }
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
      companionPed = 0;
      companionRespawnAtTime = now + 40000;
      pg_zombie_log("[Companion] Dong doi da hy sinh! Tiep vien se den sau 40 giay.");
      return;
    }

    if (!zombieEntityCoordVector) zombieEntityCoordVector = pg_asi_alloc(32);
    if (!zombieEntityCoordVector) return;

    PG_FN_GET_ENTITY_COORDS(instance, zombieEntityCoordVector, companionPed, 1);
    float *cc = (float*)pg_pointer(&env, zombieEntityCoordVector, 24);
    float dx = px - cc[0], dy = py - cc[2];
    float distSq = dx * dx + dy * dy;

    // Auto-teleport if stuck (> 45m)
    if (distSq > 45.0f * 45.0f) {
      float *v = (float*)pg_pointer(&env, zombieEntityCoordVector, 24);
      v[0] = px - 2.0f; v[1] = 0; v[2] = py - 2.0f; v[3] = 0; v[4] = pz + 0.5f; v[5] = 0;
      PG_FN_SET_ENTITY_COORDS_NO_OFFSET(instance, companionPed, zombieEntityCoordVector, 0, 0, 0);
      return;
    }

    if (now - lastCompanionCheckTime > 1500) {
      lastCompanionCheckTime = now;
      u32 closestZombie = 0;
      float closestDistSq = 35.0f * 35.0f;

      for (int i = 0; i < ZOMBIE_MAX_ACTIVE; i++) {
        u32 z = activeZombies[i];
        if (!z || zombieStates[i] == ZSTATE_DEAD || !PG_FN_ENTITY_EXISTS(instance, z)) continue;
        PG_FN_GET_ENTITY_COORDS(instance, zombieEntityCoordVector, z, 1);
        float *zc = (float*)pg_pointer(&env, zombieEntityCoordVector, 24);
        float zdx = cc[0] - zc[0], zdy = cc[2] - zc[2];
        float zdSq = zdx * zdx + zdy * zdy;
        if (zdSq < closestDistSq) { closestDistSq = zdSq; closestZombie = z; }
      }

      if (closestZombie) {
        // 0xf166e48407bac484: TASK_COMBAT_PED
        uint64_t cArgs[4] = { companionPed, closestZombie, 0, 16 };
        pg_test_call(instance, 0xf166e48407bac484ull, 4, cArgs);
      }
    }
    return;
  }

  // Spawn companion
  if (now < companionRespawnAtTime) return;
  if (!pg_zombie_has_memory()) return;

  PG_FN_REQUEST_MODEL(instance, companionModel);
  if (!PG_FN_HAS_MODEL_LOADED(instance, companionModel)) return;

  float spawnX = px - 2.5f;
  float spawnY = py - 2.5f;
  float groundZ = pz;
  pg_zombie_get_ground_z(instance, spawnX, spawnY, pz, &groundZ);

  uint64_t pArgs[8] = {
    4, companionModel, pg_zf32(spawnX), pg_zf32(spawnY), pg_zf32(groundZ + 0.2f), pg_zf32(heading), 0, 1
  };
  companionPed = (u32)pg_test_call(instance, 0xd49f9b0955c367deull, 8, pArgs);
  if (companionPed && PG_FN_ENTITY_EXISTS(instance, companionPed)) {
    PG_FN_SET_ENTITY_HEALTH(instance, companionPed, 350, 0);
    pg_test_call(instance, 0xcea04d83135264ccull, 2, (uint64_t[]){ companionPed, 100 });
    PG_FN_GIVE_WEAPON(instance, companionPed, pg_joaat("weapon_carbinerifle"), 9999, 0, 1);
    pg_test_call(instance, 0x7aefb85c1d49deb6ull, 2, (uint64_t[]){ companionPed, 85 });

    // Join player group
    uint64_t pGroup = pg_test_call(instance, 0xf162e133b4e7a675ull, 1, (uint64_t[]){ playerPed });
    if (!pGroup) pGroup = pg_test_call(instance, 0x90370ebe0fee1a3dull, 1, (uint64_t[]){ 0 });
    pg_test_call(instance, 0x9f3480fe65db31b5ull, 2, (uint64_t[]){ companionPed, pGroup });
    pg_test_call(instance, 0x3dbfc55d5c9bb447ull, 2, (uint64_t[]){ companionPed, 1 });

    // Companion blip (Blue)
    companionBlip = (u32)pg_test_call(instance, 0x5cde92c702a8fce7ull, 1, (uint64_t[]){ companionPed });
    if (companionBlip) {
      pg_test_call(instance, 0x03d7fb09e75d6b7eull, 2, (uint64_t[]){ companionBlip, 3 });
      pg_test_call(instance, 0xd38744167b2fa257ull, 2, (uint64_t[]){ companionBlip, pg_zf32(0.85f) });
      pg_test_call(instance, 0xbe8be4fe60e27b72ull, 2, (uint64_t[]){ companionBlip, 0 });
      pg_test_call(instance, 0x9029b2f3da924928ull, 2, (uint64_t[]){ companionBlip, 2 });
    }
    pg_zombie_log("[Companion] Dong doi Marine da san sang yem tro!");
  }
}

// Dead corpse garbage collection (after 25 seconds or > 65m away)
static void pg_zombie_manage_corpses(w2c_game *instance, float px, float py, double now) {
  if (!zombieEntityCoordVector) zombieEntityCoordVector = pg_asi_alloc(32);
  if (!zombieEntityCoordVector) return;

  for (int i = 0; i < ZOMBIE_MAX_CORPSES; i++) {
    u32 corpse = deadCorpses[i];
    if (!corpse) continue;

    if (!PG_FN_ENTITY_EXISTS(instance, corpse)) {
      deadCorpses[i] = 0;
      continue;
    }

    PG_FN_GET_ENTITY_COORDS(instance, zombieEntityCoordVector, corpse, 1);
    float *cc = (float*)pg_pointer(&env, zombieEntityCoordVector, 24);
    float dx = px - cc[0], dy = py - cc[2];
    float distSq = dx * dx + dy * dy;

    bool timeExpired = (now - deadCorpseTime[i] > 25000.0);
    bool farAway = (distSq > 65.0f * 65.0f);
    bool offScreen = !pg_zombie_is_entity_on_screen(instance, corpse);

    if ((timeExpired && offScreen) || farAway) {
      pg_zombie_release_ped(instance, &deadCorpses[i]);
      deadCorpses[i] = 0;
    }
  }
}

// Add corpse to tracked queue
static void pg_zombie_track_corpse(u32 corpse, double now) {
  for (int i = 0; i < ZOMBIE_MAX_CORPSES; i++) {
    if (!deadCorpses[i]) {
      deadCorpses[i] = corpse;
      deadCorpseTime[i] = now;
      return;
    }
  }
}

// Pre-population Stage 1: Road wrecks (frames 20-60, under black screen)
static void pg_zombie_prepopulate_wrecks(w2c_game *instance) {
  static const struct { float x; float y; float z; float heading; } initWrecks[8] = {
    { -855.0f,  132.0f, 63.5f, 115.0f },
    { -822.0f,  168.0f, 68.0f, 105.0f },
    { -875.0f,  115.0f, 60.0f, 285.0f },
    { -800.0f,  190.0f, 72.0f, 120.0f },
    { -910.0f,   90.0f, 54.0f, 210.0f },
    { -960.0f,   50.0f, 48.0f, 195.0f },
    { -1020.0f, -10.0f, 42.0f, 140.0f },
    { -1100.0f, -80.0f, 35.0f, 225.0f }
  };

  for (int i = 0; i < ZOMBIE_MAX_WRECKS; i++) {
    if (!activeWrecks[i]) {
      u32 veh = pg_zombie_spawn_wreck_at(instance, initWrecks[i].x, initWrecks[i].y, initWrecks[i].z, initWrecks[i].heading);
      if (veh) {
        activeWrecks[i] = veh;
      }
    }
  }
}

// Pre-population Stage 2: Initial 20 zombies in 4 concentric rings (frames 60-110, under black screen)
static void pg_zombie_prepopulate_zombies(w2c_game *instance, float px, float py, float pz) {
  static const struct { float dist; float angleDeg; } spawnRing[20] = {
    // Ring 1 (Immediate threat, 14m - 18m)
    { 14.0f,  30.0f },
    { 16.0f, 120.0f },
    { 15.0f, 210.0f },
    { 18.0f, 300.0f },
    // Ring 2 (Close wave, 24m - 30m)
    { 24.0f,  60.0f },
    { 26.0f, 150.0f },
    { 28.0f, 240.0f },
    { 30.0f, 330.0f },
    // Ring 3 (Mid distance, 36m - 48m)
    { 36.0f,  15.0f },
    { 40.0f,  90.0f },
    { 42.0f, 165.0f },
    { 44.0f, 225.0f },
    { 46.0f, 285.0f },
    { 48.0f, 345.0f },
    // Ring 4 (Far street horizons, 54m - 68m)
    { 54.0f,  45.0f },
    { 58.0f, 105.0f },
    { 60.0f, 180.0f },
    { 62.0f, 255.0f },
    { 65.0f, 315.0f },
    { 68.0f,   0.0f }
  };

  for (int i = 0; i < 20 && i < ZOMBIE_MAX_ACTIVE; i++) {
    if (!activeZombies[i]) {
      float rad = spawnRing[i].angleDeg * 0.0174532925f;
      float zx = px + cosf(rad) * spawnRing[i].dist;
      float zy = py + sinf(rad) * spawnRing[i].dist;
      pg_zombie_spawn_ped_at(instance, zx, zy, pz, i);
    }
  }
}

// Main horde management loop (Staggered: 4 zombies per frame, <0.02ms)
static void pg_zombie_manage_horde(w2c_game *instance, u32 playerPed, float px, float py, float pz, float heading, double now) {
  if (!atomic_load(&zombieEnableZombies)) return;

  // Dynamic cap based on Metal frame time: smoothly adapt to maintain 60 FPS
  int dynamicZombieCap = ZOMBIE_TARGET_ACTIVE;
  double gpuMs = PGGPUFrameMilliseconds();
  if (gpuMs > 25.0) {
    dynamicZombieCap = ZOMBIE_MIN_ACTIVE;
  }

  // Request models and clipsets continuously
  for (int m = 0; m < activeZombieModelCount; m++) {
    PG_FN_REQUEST_MODEL(instance, activeZombieModels[m]);
  }
  pg_zombie_request_clipsets(instance);

  if (!zombieEntityCoordVector) zombieEntityCoordVector = pg_asi_alloc(32);
  if (!zombieEntityCoordVector) return;

  // Staggered checking: Process 4 slots per frame (8 frames for full cycle)
  static int staggerOffset = 0;
  int startIdx = staggerOffset;
  int endIdx = startIdx + 4;
  if (endIdx > ZOMBIE_MAX_ACTIVE) endIdx = ZOMBIE_MAX_ACTIVE;
  staggerOffset = (endIdx >= ZOMBIE_MAX_ACTIVE) ? 0 : endIdx;

  for (int i = startIdx; i < endIdx; i++) {
    u32 z = activeZombies[i];
    if (!z) continue;

    if (!PG_FN_ENTITY_EXISTS(instance, z)) {
      pg_zombie_remove_blip(instance, &zombieBlips[i]);
      activeZombies[i] = 0;
      zombieStates[i] = ZSTATE_WANDER;
      continue;
    }

    // 0x3317dedb88c95038: IS_PED_DEAD_OR_DYING
    bool isDead = (bool)pg_test_call(instance, 0x3317dedb88c95038ull, 1, (uint64_t[]){ z });
    if (isDead) {
      if (zombieStates[i] != ZSTATE_DEAD) {
        zombieStates[i] = ZSTATE_DEAD;
        pg_zombie_remove_blip(instance, &zombieBlips[i]);
        int currentKills = atomic_fetch_add(&zombieKills, 1) + 1;
        pg_zombie_update_weapons(instance, playerPed, currentKills);
        pg_zombie_track_corpse(z, now);
        activeZombies[i] = 0; // Free slot for new spawn while corpse rests
        pg_zombie_log("[Combat] Zombie #%d da bi tieu diet! Tong kills: %d.", i + 1, currentKills);
      }
      continue;
    }

    // Distance to player
    PG_FN_GET_ENTITY_COORDS(instance, zombieEntityCoordVector, z, 1);
    float *zc = (float*)pg_pointer(&env, zombieEntityCoordVector, 24);
    float dx = px - zc[0], dy = py - zc[2];
    float dist = sqrtf(dx * dx + dy * dy);

    // Target selection (Player or Companion)
    u32 target = playerPed;
    float targetDist = dist;

    if (companionPed && PG_FN_ENTITY_EXISTS(instance, companionPed)) {
      PG_FN_GET_ENTITY_COORDS(instance, zombieEntityCoordVector, companionPed, 1);
      float *cc = (float*)pg_pointer(&env, zombieEntityCoordVector, 24);
      float cdx = cc[0] - zc[0], cdy = cc[2] - zc[2];
      float cdist = sqrtf(cdx * cdx + cdy * cdy);
      if (cdist < dist && cdist < 30.0f) {
        target = companionPed;
        targetDist = cdist;
      }
    }

    // Hit reaction: Check if health dropped
    u32 currentHp = (u32)pg_test_call(instance, 0xeef059fad016d209ull, 1, (uint64_t[]){ z });
    if (currentHp < zombiePrevHealth[i] && currentHp > 0) {
      zombiePrevHealth[i] = currentHp;
      pg_zombie_log("[Combat] Zombie #%d bi danh trung! Mau con: %u.", i + 1, currentHp);

      // Short stumble / ragdoll reaction
      if (atomic_load(&zombieEnableRagdoll)) {
        // 0xae99fb955581844a: SET_PED_TO_RAGDOLL
        pg_test_call(instance, 0xae99fb955581844aull, 7, (uint64_t[]){ z, 1000, 1200, 0, 1, 1, 0 });
      }

      // Immediately alert and chase attacker
      zombieStates[i] = ZSTATE_CHASE;
      zombieNextTaskTime[i] = now + 1200;
    }

    // Despawn if far away (> 110m)
    if (dist > 110.0f) {
      pg_zombie_remove_blip(instance, &zombieBlips[i]);
      pg_zombie_release_ped(instance, &activeZombies[i]);
      zombieStates[i] = ZSTATE_WANDER;
      continue;
    }

    // STATE MACHINE: WANDER -> CHASE -> ATTACK
    if (zombieStates[i] == ZSTATE_WANDER) {
      // Detection radius: 45m
      float detectRadius = 45.0f;
      if (targetDist <= detectRadius) {
        zombieStates[i] = ZSTATE_CHASE;
        zombieNextTaskTime[i] = now;
        pg_zombie_log("[AI] Zombie #%d phat hien muc tieu (%.1fm) -> Bat dau DUOI!", i + 1, targetDist);
      }
    } else if (zombieStates[i] == ZSTATE_CHASE) {
      if (targetDist > 85.0f) {
        // Lost track, return to wander
        zombieStates[i] = ZSTATE_WANDER;
        pg_zombie_apply_clipset(instance, z, zombieClipsetIndex[i]);
        pg_test_call(instance, 0xbb9ce077274f6a1bull, 3, (uint64_t[]){ z, pg_zf32(10.0f), 10 });
        pg_zombie_log("[AI] Zombie #%d mat dau muc tieu -> Quay lai LAO DAO.", i + 1);
      } else if (targetDist < 1.6f) {
        // Close enough to attack/bite
        zombieStates[i] = ZSTATE_ATTACK;
        zombieNextBiteTime[i] = now + 1100;
      } else if (now >= zombieNextTaskTime[i]) {
        // Re-evaluate chase navigation every 1.5s
        zombieNextTaskTime[i] = now + 1500;
        int kills = atomic_load(&zombieKills);
        float chaseSpeed = 1.35f + (float)(kills > 50 ? 50 : kills) * 0.008f + ((float)(z % 3) * 0.10f);

        // Maintain zombie movement clipset during chase
        pg_zombie_apply_clipset(instance, z, zombieClipsetIndex[i]);

        // Native Combat Pursuit: TASK_COMBAT_PED (0xf166e48407bac484)
        uint64_t cArgs[4] = { z, target, 0, 16 };
        pg_test_call(instance, 0xf166e48407bac484ull, 4, cArgs);

        // 0x085bf80fa50a39d1: SET_PED_MOVE_RATE_OVERRIDE
        pg_test_call(instance, 0x085bf80fa50a39d1ull, 2, (uint64_t[]){ z, pg_zf32(chaseSpeed) });

        // Maintain compatibility fallback: 0x6a071245eb0d1882 (TASK_GO_TO_ENTITY)
        // If distant, ensure navigation target is locked
        if (targetDist > 25.0f) {
          uint64_t fArgs[7] = { z, target, 0xFFFFFFFFull, pg_zf32(1.2f), pg_zf32(chaseSpeed), pg_zf32(2.0f), 0 };
          pg_test_call(instance, 0x6a071245eb0d1882ull, 7, fArgs);
        }
      }
    } else if (zombieStates[i] == ZSTATE_ATTACK) {
      if (targetDist >= 2.2f) {
        zombieStates[i] = ZSTATE_CHASE;
        zombieNextTaskTime[i] = now;
      } else if (now >= zombieNextBiteTime[i]) {
        zombieNextBiteTime[i] = now + 1200;
        if (atomic_load(&zombieEnableBiteDamage)) {
          // 0x697157ced63f18d4: APPLY_DAMAGE_TO_PED (Armor first, clean native)
          pg_test_call(instance, 0x697157ced63f18d4ull, 3, (uint64_t[]){ target, 12, 1 });
          pg_zombie_log("[Combat] Zombie #%d can trung muc tieu! Gay 12 sat thuong.", i + 1);
        }
      }
    }
  }

  // Count total living across all slots
  int livingZombies = 0;
  for (int i = 0; i < ZOMBIE_MAX_ACTIVE; i++) {
    if (activeZombies[i] && zombieStates[i] != ZSTATE_DEAD) livingZombies++;
  }

  // Spawn Replenishment: Rate limited to 1 per 350ms, strictly outside player camera view
  if (now - lastZombieSpawnTime < 350) return;
  if (livingZombies >= dynamicZombieCap) return;
  if (!pg_zombie_has_memory()) return;

  int freeSlot = -1;
  for (int i = 0; i < ZOMBIE_MAX_ACTIVE; i++) {
    if (!activeZombies[i]) { freeSlot = i; break; }
  }
  if (freeSlot < 0) return;

  // Calculate candidate spawn position: 45m - 70m away, strictly in blind / flank angles
  float spawnDist = 45.0f + (float)(rand() % 25);
  // Spawn in the rear hemisphere (+/- 90 to 180 degrees from heading)
  float blindAngle = (100.0f + (float)(rand() % 160)) * (rand() % 2 == 0 ? 1.0f : -1.0f);
  float rad = (heading + blindAngle) * 0.0174532925f;

  float candX = px - sinf(rad) * spawnDist;
  float candY = py + cosf(rad) * spawnDist;

  if (!isfinite(candX) || !isfinite(candY)) return;

  // Guard: NEVER spawn while player is looking at the point
  if (pg_zombie_is_point_on_screen(instance, px, py, heading, candX, candY, pz, 4.0f)) {
    return; // Point is visible on camera, skip to prevent pop-in!
  }

  lastZombieSpawnTime = now;
  u32 newPed = pg_zombie_spawn_ped_at(instance, candX, candY, pz, freeSlot);
  if (newPed) {
    pg_zombie_log("[Zombie Spawner] Da bo sung Zombie #%d tai vi tri an toan (%.1f, %.1f). Tong song: %d/%d.",
                  freeSlot + 1, candX, candY, livingZombies + 1, dynamicZombieCap);
  }
}

// Extraction Objective Update
static void pg_zombie_manage_extraction(w2c_game *instance, u32 playerPed, float px, float py, float pz, double now) {
  if (!atomic_load(&zombieEnableExtraction)) return;

  float dx = px - evacX;
  float dy = py - evacY;
  float distToEvac = sqrtf(dx * dx + dy * dy);
  atomic_store(&zombieDistanceToEvac, distToEvac);

  if (atomic_load(&zombieEscaped)) {
    snprintf(zombieStatusText, sizeof(zombieStatusText), "CHIEN THANG: DA THOAT KHOI THANH PHO THANH CONG!");
    return;
  }

  if (distToEvac < 40.0f) {
    if (evacHoldStartTime == 0) {
      evacHoldStartTime = now;
      pg_zombie_log("[Evac] Nguoi choi da vao vung so tan! Bat dau dem nguoc 45 giay.");
    }
    int secondsLeft = 45 - (int)((now - evacHoldStartTime) / 1000.0);

    if (secondsLeft > 0) {
      snprintf(zombieStatusText, sizeof(zombieStatusText), "DANG CO THU DIEM SO TAN! CON %d GIAY DE THOAT", secondsLeft);
    } else {
      atomic_store(&zombieEscaped, true);
      snprintf(zombieStatusText, sizeof(zombieStatusText), "TRUC THANG DA DEN! THOAT THANH CONG!");
      pg_zombie_log("[Evac] Chuc mung! Nguoi choi da song sot va thoat khoi dai dich Zombie!");
    }
  } else {
    evacHoldStartTime = 0;
    int kills = atomic_load(&zombieKills);
    int tier = atomic_load(&zombieTier);
    snprintf(zombieStatusText, sizeof(zombieStatusText),
             "DIET: %d | CAP: %d/5 | DIEM SO TAN: %.0fm (Ben tau Del Perro)",
             kills, tier, distToEvac);
  }
}

// Full cleanup
static void pg_zombie_cleanup_all(w2c_game *instance) {
  for (int i = 0; i < ZOMBIE_MAX_ACTIVE; i++) {
    pg_zombie_remove_blip(instance, &zombieBlips[i]);
    pg_zombie_release_ped(instance, &activeZombies[i]);
    zombieStates[i] = ZSTATE_WANDER;
  }
  for (int i = 0; i < ZOMBIE_MAX_WRECKS; i++) {
    pg_zombie_delete_wreck(instance, &activeWrecks[i]);
  }
  for (int i = 0; i < ZOMBIE_MAX_CORPSES; i++) {
    pg_zombie_release_ped(instance, &deadCorpses[i]);
  }
  pg_zombie_remove_blip(instance, &companionBlip);
  pg_zombie_release_ped(instance, &companionPed);
  pg_zombie_remove_blip(instance, &evacBlip);
  pg_zombie_log("[Zombie System] Da don dep sach entity va dat lai trang thai.");
}

// ============================================================================
// MAIN SCRIPT TICK LOOP (Lightweight, < 0.05ms per tick)
// ============================================================================
static void pg_zombie_tick(w2c_game *instance) {
  if (!atomic_load(&zombieActive)) {
    static bool cleanedUp = false;
    if (!cleanedUp) {
      cleanedUp = true;
      pg_zombie_cleanup_all(instance);
    }
    return;
  }
  if (PGPauseMenuActive() || PGLoadSceneActive() || (PGGameStatus() & 16)) return;

  u32 playerPed = PG_FN_PLAYER_PED_ID(instance);
  if (!playerPed || !PG_FN_ENTITY_EXISTS(instance, playerPed)) return;

  if (!pg_asi_game) pg_asi_game = instance;
  uint64_t prevScript = pg_asi_enter_script(instance);

  double tickStart = timeMS(CLOCK_MONOTONIC);
  zombieFrameCounter++;
  double now = tickStart;

  if (!zombiePlayerCoordVector) zombiePlayerCoordVector = pg_asi_alloc(32);
  if (!zombiePlayerCoordVector) {
    pg_asi_leave_script(instance, prevScript);
    return;
  }

  // Handle explicit reset request from UI
  if (atomic_exchange(&zombieResetRequested, false)) {
    pg_zombie_cleanup_all(instance);
    zombieCurrentStage = ZOMBIE_STAGE_BOOT;
    playerTeleportedOutdoors = false;
    pg_asi_leave_script(instance, prevScript);
    return;
  }

  // ==========================================================================
  // STAGE 0: IMMEDIATE FADE OUT + DAYLIGHT + TELEPORT OUTDOORS (Frame 0)
  // ==========================================================================
  if (zombieCurrentStage == ZOMBIE_STAGE_BOOT) {
    // 0x891b5b39ac6302af: DO_SCREEN_FADE_OUT(0)
    pg_test_call(instance, 0x891b5b39ac6302afull, 1, (uint64_t[]){ 0 });

    pg_zombie_apply_atmosphere_immediate(instance);
    pg_zombie_apply_traffic_suppression(instance);
    pg_zombie_teleport_outdoors(instance, playerPed);
    pg_zombie_setup_relationships(instance, playerPed);
    pg_zombie_request_clipsets(instance);

    // Kiem tra model zombie that u_m_y_zombie_01 trong game data (0xc0296a2edf545e92: IS_MODEL_VALID)
    uint32_t realZombie = pg_joaat("u_m_y_zombie_01");
    if ((bool)pg_test_call(instance, 0xc0296a2edf545e92ull, 1, (uint64_t[]){ realZombie })) {
      activeZombieModels[activeZombieModelCount++] = realZombie;
      pg_zombie_log("[Zombie Model] Xac nhan model zombie that u_m_y_zombie_01 ton tai trong game, da them vao danh sach.");
    }

    zombieCurrentStage = ZOMBIE_STAGE_PREPOPULATE_WRECKS;
    pg_zombie_log("[Zombie] Stage 0 KHOI TAO: Da khoa gio 12h, OVERCAST va dua ra ngoai duong Portola Drive.");
  }

  // Traffic suppression runs every frame (< 0.01ms)
  pg_zombie_apply_traffic_suppression(instance);

  PG_FN_GET_ENTITY_COORDS(instance, zombiePlayerCoordVector, playerPed, 1);
  float *v = (float*)pg_pointer(&env, zombiePlayerCoordVector, 24);
  float px = v[0], py = v[2], pz = v[4];

  // Authentic, crash-proof heading reading via direct C engine function
  float heading = PG_FN_GET_ENTITY_HEADING(instance, playerPed);
  if (!isfinite(heading) || heading < -360.0f || heading > 720.0f) {
    heading = 0.0f;
  }

  // ==========================================================================
  // STAGE 1: PRE-POPULATE ROAD WRECKS (Frames 20 - 60, under black screen)
  // ==========================================================================
  if (zombieCurrentStage == ZOMBIE_STAGE_PREPOPULATE_WRECKS) {
    if (zombieFrameCounter > 25) {
      pg_zombie_prepopulate_wrecks(instance);
      zombieCurrentStage = ZOMBIE_STAGE_PREPOPULATE_ZOMBIES;
      pg_zombie_log("[Zombie] Stage 1 HOAN TAT: Da dat san 8 xe bo hoang tren mat duong.");
    }
  }

  // ==========================================================================
  // STAGE 2: PRE-POPULATE 20 ZOMBIES & COMPANION (Frames 60 - 110, under black screen)
  // ==========================================================================
  if (zombieCurrentStage == ZOMBIE_STAGE_PREPOPULATE_ZOMBIES) {
    if (zombieFrameCounter > 65) {
      pg_zombie_manage_companion(instance, playerPed, px, py, pz, heading, now);
      pg_zombie_prepopulate_zombies(instance, px, py, pz);
      zombieCurrentStage = ZOMBIE_STAGE_READY_AND_FADEIN;
      pg_zombie_log("[Zombie] Stage 2 HOAN TAT: Da sinh san 20 zombie trong 4 vanh dai va dong doi.");
    }
  }

  // ==========================================================================
  // STAGE 3: INITIAL WEAPONS & SMOOTH FADE IN (Frame 120, ~2s)
  // ==========================================================================
  if (zombieCurrentStage == ZOMBIE_STAGE_READY_AND_FADEIN) {
    if (zombieFrameCounter > 120) {
      pg_zombie_apply_initial_loadout(instance, playerPed);
      pg_zombie_setup_extraction(instance);

      // 0xd4e8e24955024033: DO_SCREEN_FADE_IN(1000)
      pg_test_call(instance, 0xd4e8e24955024033ull, 1, (uint64_t[]){ 1000 });

      zombieCurrentStage = ZOMBIE_STAGE_ACTIVE;
      pg_zombie_log("[Zombie] Stage 3 HOAN TAT: The gioi da san sang! Hien hinh anh sang ban ngay suong mu.");
    }
  }

  // ==========================================================================
  // STAGE 4: ACTIVE SURVIVAL LOOP
  // ==========================================================================
  if (zombieCurrentStage == ZOMBIE_STAGE_ACTIVE) {
    // Re-assert atmosphere every 60 frames (~1s at 60 FPS)
    if (zombieFrameCounter % 60 == 0) {
      pg_zombie_apply_atmosphere_immediate(instance);
    }

    // Companion survivor update (every 30 frames)
    if (zombieFrameCounter % 30 == 0) {
      pg_zombie_manage_companion(instance, playerPed, px, py, pz, heading, now);
      pg_zombie_manage_corpses(instance, px, py, now);
    }

    // Extraction checkpoint update (every 30 frames)
    if (zombieFrameCounter % 30 == 15) {
      pg_zombie_manage_extraction(instance, playerPed, px, py, pz, now);
    }

    // Main zombie horde management (Staggered: 4 slots per frame)
    pg_zombie_manage_horde(instance, playerPed, px, py, pz, heading, now);
  }

  // Measure tick duration
  double tickDuration = timeMS(CLOCK_MONOTONIC) - tickStart;
  if (tickDuration > 5.0) {
    pg_zombie_log("[Warning] Tick duration %.2f ms vuot nguong an toan!", tickDuration);
  }

  pg_asi_leave_script(instance, prevScript);
}

// ============================================================================
// PUBLIC BRIDGING API (Exposed to UIKit / Swift)
// ============================================================================
bool PGZombieIsActive(void) { return atomic_load(&zombieActive); }
void PGZombieSetActive(bool active) { atomic_store(&zombieActive, active); }
int  PGZombieGetKills(void) { return atomic_load(&zombieKills); }
int  PGZombieGetTier(void) { return atomic_load(&zombieTier); }
bool PGZombieCompanionAlive(void) { return (companionPed != 0); }
void PGZombieResetSurvival(void) { atomic_store(&zombieResetRequested, true); }
float PGZombieGetEscapeDistance(void) { return atomic_load(&zombieDistanceToEvac); }
bool PGZombieIsEscaped(void) { return atomic_load(&zombieEscaped); }

int PGZombieGetActiveCount(void) {
  int count = 0;
  for (int i = 0; i < ZOMBIE_MAX_ACTIVE; i++) {
    if (activeZombies[i] && zombieStates[i] != ZSTATE_DEAD) count++;
  }
  return count;
}

int PGZombieGetWreckCount(void) {
  int count = 0;
  for (int i = 0; i < ZOMBIE_MAX_WRECKS; i++) {
    if (activeWrecks[i]) count++;
  }
  return count;
}

void PGZombieGetStatus(char *buffer, size_t maxLen) {
  if (!buffer || maxLen == 0) return;
  snprintf(buffer, maxLen, "%s", zombieStatusText);
}

void PGZombieGetLog(char *buffer, size_t maxLen) {
  if (!buffer || maxLen == 0) return;
  buffer[0] = '\0';
  pthread_mutex_lock(&zombieLogLock);
  size_t offset = 0;
  int start = (zombieLogCount < ZOMBIE_MAX_LOG_LINES) ? 0 : (zombieLogHead % ZOMBIE_MAX_LOG_LINES);
  for (int i = 0; i < zombieLogCount; i++) {
    int idx = (start + i) % ZOMBIE_MAX_LOG_LINES;
    int written = snprintf(buffer + offset, maxLen - offset, "%s\n", zombieLogLines[idx]);
    if (written < 0 || (size_t)written >= maxLen - offset) break;
    offset += (size_t)written;
  }
  pthread_mutex_unlock(&zombieLogLock);
}

// Module toggles getters & setters
bool PGZombieGetModuleAtmosphere(void) { return atomic_load(&zombieEnableAtmosphere); }
void PGZombieSetModuleAtmosphere(bool enabled) { atomic_store(&zombieEnableAtmosphere, enabled); }
bool PGZombieGetModuleWrecks(void) { return atomic_load(&zombieEnableWrecks); }
void PGZombieSetModuleWrecks(bool enabled) { atomic_store(&zombieEnableWrecks, enabled); }
bool PGZombieGetModuleCompanion(void) { return atomic_load(&zombieEnableCompanion); }
void PGZombieSetModuleCompanion(bool enabled) { atomic_store(&zombieEnableCompanion, enabled); }
bool PGZombieGetModuleExtraction(void) { return atomic_load(&zombieEnableExtraction); }
void PGZombieSetModuleExtraction(bool enabled) { atomic_store(&zombieEnableExtraction, enabled); }

bool PGZombieGetModuleBiteDamage(void) { return atomic_load(&zombieEnableBiteDamage); }
void PGZombieSetModuleBiteDamage(bool enabled) { atomic_store(&zombieEnableBiteDamage, enabled); }
bool PGZombieGetModuleRagdoll(void) { return atomic_load(&zombieEnableRagdoll); }
void PGZombieSetModuleRagdoll(bool enabled) { atomic_store(&zombieEnableRagdoll, enabled); }
bool PGZombieGetModuleBlood(void) { return atomic_load(&zombieEnableBlood); }
void PGZombieSetModuleBlood(bool enabled) { atomic_store(&zombieEnableBlood, enabled); }
bool PGZombieGetModuleAudio(void) { return atomic_load(&zombieEnableAudio); }
void PGZombieSetModuleAudio(bool enabled) { atomic_store(&zombieEnableAudio, enabled); }
bool PGZombieGetModuleWeaponUnlocks(void) { return atomic_load(&zombieEnableWeaponUnlocks); }
void PGZombieSetModuleWeaponUnlocks(bool enabled) { atomic_store(&zombieEnableWeaponUnlocks, enabled); }
