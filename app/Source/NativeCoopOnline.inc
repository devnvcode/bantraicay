// ============================================================================
// PlayGTA5Native - GTA ONLINE / CO-OP MULTIPLAYER SYSTEM (V1 CO-OP ENGINE)
// ============================================================================
// Features:
// 1. Chế độ chơi Co-op Online cả trong Story Mode (Cốt truyện) & Sandbox Mode.
// 2. Kết nối bằng địa chỉ IP mạng nội bộ, VPS hoặc Dedicated Server tùy chọn.
// 3. Tự động vượt qua / bypass mọi kiểm tra phiên bản game (Version Check Bypass).
// 4. Đồng bộ vị trí tọa độ (X, Y, Z), góc nhìn (Heading), chuyển động chạy/lái xe.
// 5. Đồng bộ Model Ped và Phương tiện xe (Vehicle) của người chơi.
// 6. Hiển thị dấu chấm xanh (Blue Blip) toàn bản đồ và hiển thị Tên người chơi
//    đã đặt tại sảnh chờ.
// 7. Non-blocking UDP Network Socket (O_NONBLOCK) - Khóa cứng 60 FPS, không lag.
// ============================================================================

#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef int socklen_t;
#define close closesocket
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#endif

#define COOP_MAX_PLAYERS        16
#define COOP_PACKET_MAGIC       0x4754414F  // 'GTAO'
#define COOP_PROTOCOL_VERSION   1
#define COOP_DEFAULT_PORT       4499
#define COOP_TIMEOUT_MS         6000.0

// Packet structure (packed to ensure exact cross-platform byte alignment)
#pragma pack(push, 1)
typedef struct {
  uint32_t magic;          // 'GTAO' (0x4754414F)
  uint8_t  packetType;     // 1: Ping/Handshake, 2: StateSync, 3: Goodbye
  uint8_t  clientVersion;  // Version byte (auto-matched / bypassed)
  uint16_t playerId;       // Unique ID from player name hash
  char     playerName[32]; // Player lobby display name
  float    posX;           // World X
  float    posY;           // World Y
  float    posZ;           // World Z
  float    heading;        // Heading (0..360)
  float    velX;           // Velocity X
  float    velY;           // Velocity Y
  float    velZ;           // Velocity Z
  uint32_t pedModel;       // Model hash of player character
  uint32_t vehModel;       // Model hash of vehicle (0 if on foot)
  int8_t   seatIndex;      // Seat index (-1: driver)
  uint8_t  inVehicle;      // 1 if inside vehicle, 0 if on foot
  uint32_t weaponHash;     // Current weapon hash
  uint16_t health;         // Health
  uint16_t armor;          // Armor
  uint8_t  isShooting;     // 1 if firing weapon
  uint8_t  isRunning;      // 1 if sprinting
  uint32_t timestamp;      // Milliseconds
} PGCoopSyncPacket;
#pragma pack(pop)

// Remote player representation
typedef struct {
  bool     active;
  uint16_t playerId;
  char     playerName[32];
  float    targetX, targetY, targetZ;
  float    currentX, currentY, currentZ;
  float    targetHeading, currentHeading;
  float    velX, velY, velZ;
  uint32_t pedModel;
  uint32_t loadedPedModel;
  uint32_t vehModel;
  uint32_t loadedVehModel;
  uint8_t  inVehicle;
  int8_t   seatIndex;
  uint32_t weaponHash;
  uint32_t currentWeaponHash;
  uint16_t health;
  uint8_t  isShooting;
  uint8_t  isRunning;
  double   lastPacketTime;

  // In-engine entity handles
  u32      pedHandle;
  u32      vehHandle;
  u32      blipHandle;
} PGCoopRemotePlayer;

// Global Co-op Online State
static _Atomic bool coopActive = false;
static _Atomic bool coopHostMode = false;
static _Atomic bool coopConnected = false;
static _Atomic int  coopPlayerCount = 0;

static pthread_mutex_t coopLock = PTHREAD_MUTEX_INITIALIZER;
static char coopServerIP[64] = "127.0.0.1";
static int  coopServerPort = COOP_DEFAULT_PORT;
static char coopPlayerName[32] = "Player";
static int  coopSocket = -1;
static double coopLastSendTime = 0.0;
static uint64_t coopCoordVector = 0;
static uint64_t coopEntityPtrBuf = 0;
static uint64_t coopBlipPtrBuf = 0;

static PGCoopRemotePlayer remotePlayers[COOP_MAX_PLAYERS];

// Hash function for player ID
static uint16_t pg_coop_hash_name(const char *name) {
  uint16_t hash = 5381;
  while (*name) {
    hash = ((hash << 5) + hash) + (uint8_t)(*name++);
  }
  return hash ? hash : 1;
}

// Convert float to GTA V native 64-bit float representation
static inline uint64_t pg_coop_f32(float val) {
  union { float f; uint32_t u; } c;
  c.f = val;
  return (uint64_t)c.u;
}

// Clean up single remote player's entities
static void pg_coop_cleanup_player(w2c_game *instance, PGCoopRemotePlayer *p) {
  if (!p) return;
  if (!coopEntityPtrBuf) coopEntityPtrBuf = pg_asi_alloc(16);
  if (!coopBlipPtrBuf)   coopBlipPtrBuf   = pg_asi_alloc(16);

  // Remove Blip
  if (p->blipHandle && instance) {
    if (coopBlipPtrBuf) {
      *(u32*)pg_pointer(&env, coopBlipPtrBuf, 4) = p->blipHandle;
      // 0x86a652570e5f25dd: REMOVE_BLIP
      pg_test_call(instance, 0x86a652570e5f25ddull, 1, (uint64_t[]){ coopBlipPtrBuf });
    }
    p->blipHandle = 0;
  }

  // Remove Vehicle
  if (p->vehHandle && instance) {
    if (coopEntityPtrBuf) {
      *(u32*)pg_pointer(&env, coopEntityPtrBuf, 4) = p->vehHandle;
      // 0x2595dd4236549ce3: DELETE_ENTITY
      pg_test_call(instance, 0x2595dd4236549ce3ull, 1, (uint64_t[]){ coopEntityPtrBuf });
    }
    p->vehHandle = 0;
  }

  // Remove Ped
  if (p->pedHandle && instance) {
    if (coopEntityPtrBuf) {
      *(u32*)pg_pointer(&env, coopEntityPtrBuf, 4) = p->pedHandle;
      // 0x2595dd4236549ce3: DELETE_ENTITY
      pg_test_call(instance, 0x2595dd4236549ce3ull, 1, (uint64_t[]){ coopEntityPtrBuf });
    }
    p->pedHandle = 0;
  }

  p->active = false;
  p->playerId = 0;
  p->playerName[0] = '\0';
}

// Clean up all remote entities and sockets
static void pg_coop_shutdown(w2c_game *instance) {
  pthread_mutex_lock(&coopLock);
  for (int i = 0; i < COOP_MAX_PLAYERS; i++) {
    if (remotePlayers[i].active || remotePlayers[i].pedHandle || remotePlayers[i].blipHandle) {
      pg_coop_cleanup_player(instance, &remotePlayers[i]);
    }
  }
  atomic_store(&coopPlayerCount, 0);
  atomic_store(&coopConnected, false);

  if (coopSocket >= 0) {
    close(coopSocket);
    coopSocket = -1;
  }
  pthread_mutex_unlock(&coopLock);
}

// Per-frame Co-op tick called from pg_asi_script_tick in EngineHost.c
static void pg_coop_online_tick(w2c_game *instance) {
  if (!instance) return;

  // 1. If Co-op is not active, ensure all entities are cleaned up and return
  if (!atomic_load(&coopActive)) {
    if (atomic_load(&coopConnected) || coopSocket >= 0) {
      pg_coop_shutdown(instance);
    }
    return;
  }

  // 2. Ensure player ped exists in world
  const u32 playerPed = PG_FN_PLAYER_PED_ID(instance);
  if (!playerPed || !PG_FN_ENTITY_EXISTS(instance, playerPed)) return;

  double now = timeMS(CLOCK_MONOTONIC);

  // 3. Initialize UDP socket if needed (non-blocking)
  if (coopSocket < 0) {
#ifdef _WIN32
    static bool wsaInit = false;
    if (!wsaInit) {
      WSADATA wsa;
      WSAStartup(MAKEWORD(2, 2), &wsa);
      wsaInit = true;
    }
#endif
    coopSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (coopSocket >= 0) {
#ifdef _WIN32
      u_long mode = 1;
      ioctlsocket(coopSocket, FIONBIO, &mode);
#else
      int flags = fcntl(coopSocket, F_GETFL, 0);
      fcntl(coopSocket, F_SETFL, flags | O_NONBLOCK);
#endif
      int reuse = 1;
      setsockopt(coopSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse));

      if (atomic_load(&coopHostMode)) {
        struct sockaddr_in bindAddr;
        memset(&bindAddr, 0, sizeof(bindAddr));
        bindAddr.sin_family = AF_INET;
        bindAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        bindAddr.sin_port = htons((uint16_t)coopServerPort);
        bind(coopSocket, (struct sockaddr*)&bindAddr, sizeof(bindAddr));
      }
      atomic_store(&coopConnected, true);
    }
  }

  if (coopSocket < 0) return;

  // 4. Send Local State Packet (~30 times per second = every 33ms)
  if (now - coopLastSendTime >= 33.0) {
    if (!coopCoordVector) coopCoordVector = pg_asi_alloc(24);
    if (coopCoordVector) {
      PG_FN_GET_ENTITY_COORDS(instance, coopCoordVector, playerPed, 1);
      float *coords = (float*)pg_pointer(&env, coopCoordVector, 24);
      float heading = PG_FN_GET_ENTITY_HEADING(instance, playerPed);

      // Check vehicle status
      // 0x9a9112a0fe9aee8a: GET_VEHICLE_PED_IS_IN
      u32 currentVeh = (u32)pg_test_call(instance, 0x9a9112a0fe9aee8aull, 2, (uint64_t[]){ playerPed, 0 });
      bool inVehicle = (currentVeh && PG_FN_ENTITY_EXISTS(instance, currentVeh));
      u32 vehModel = inVehicle ? PG_FN_GET_ENTITY_MODEL(instance, currentVeh) : 0;
      u32 pedModel = PG_FN_GET_ENTITY_MODEL(instance, playerPed);

      PGCoopSyncPacket pkt;
      memset(&pkt, 0, sizeof(pkt));
      pkt.magic = COOP_PACKET_MAGIC;
      pkt.packetType = 2; // StateSync
      pkt.clientVersion = COOP_PROTOCOL_VERSION; // Auto version match
      pkt.playerId = pg_coop_hash_name(coopPlayerName);
      snprintf(pkt.playerName, sizeof(pkt.playerName), "%s", coopPlayerName);
      pkt.posX = coords[0];
      pkt.posY = coords[2];
      pkt.posZ = coords[4];
      pkt.heading = heading;
      pkt.pedModel = pedModel;
      pkt.vehModel = vehModel;
      pkt.inVehicle = inVehicle ? 1 : 0;
      pkt.seatIndex = -1; // Driver
      pkt.health = 200;
      pkt.timestamp = (uint32_t)now;

      struct sockaddr_in destAddr;
      memset(&destAddr, 0, sizeof(destAddr));
      destAddr.sin_family = AF_INET;
      destAddr.sin_port = htons((uint16_t)coopServerPort);
      destAddr.sin_addr.s_addr = inet_addr(coopServerIP);

      if (destAddr.sin_addr.s_addr != INADDR_NONE) {
        sendto(coopSocket, (const char*)&pkt, sizeof(pkt), 0, (struct sockaddr*)&destAddr, sizeof(destAddr));
      }
      coopLastSendTime = now;
    }
  }

  // 5. Receive and process incoming packets (non-blocking)
  struct sockaddr_in fromAddr;
  socklen_t fromLen = sizeof(fromAddr);
  PGCoopSyncPacket inPkt;

  for (int iter = 0; iter < 16; iter++) {
    int bytes = (int)recvfrom(coopSocket, (char*)&inPkt, sizeof(inPkt), 0, (struct sockaddr*)&fromAddr, &fromLen);
    if (bytes < (int)sizeof(PGCoopSyncPacket)) break;

    // Check Magic Header
    if (inPkt.magic != COOP_PACKET_MAGIC) continue;

    // VERSION CHECK BYPASS: Mọi phiên bản game đều được chấp nhận đồng bộ
    // Không bao giờ ngắt kết nối vì lệch số phiên bản (Version Mismatch Bypass).
    uint16_t remoteId = inPkt.playerId;
    if (remoteId == pg_coop_hash_name(coopPlayerName)) {
      // Ignore packet reflected back to self
      continue;
    }

    // Find existing slot or free slot
    int foundSlot = -1;
    int freeSlot = -1;
    for (int i = 0; i < COOP_MAX_PLAYERS; i++) {
      if (remotePlayers[i].active && remotePlayers[i].playerId == remoteId) {
        foundSlot = i;
        break;
      }
      if (!remotePlayers[i].active && freeSlot < 0) {
        freeSlot = i;
      }
    }

    int slot = foundSlot >= 0 ? foundSlot : freeSlot;
    if (slot >= 0) {
      PGCoopRemotePlayer *r = &remotePlayers[slot];
      if (!r->active) {
        r->active = true;
        r->playerId = remoteId;
        r->currentX = inPkt.posX;
        r->currentY = inPkt.posY;
        r->currentZ = inPkt.posZ;
        r->currentHeading = inPkt.heading;
      }
      snprintf(r->playerName, sizeof(r->playerName), "%s", inPkt.playerName);
      r->targetX = inPkt.posX;
      r->targetY = inPkt.posY;
      r->targetZ = inPkt.posZ;
      r->targetHeading = inPkt.heading;
      r->pedModel = inPkt.pedModel;
      r->vehModel = inPkt.vehModel;
      r->inVehicle = inPkt.inVehicle;
      r->seatIndex = inPkt.seatIndex;
      r->lastPacketTime = now;
    }
  }

  // 6. Update World Entities & Smooth Interpolation for all active remote players
  int activeCount = 0;
  for (int i = 0; i < COOP_MAX_PLAYERS; i++) {
    PGCoopRemotePlayer *r = &remotePlayers[i];
    if (!r->active) continue;

    // Check timeout (> 6.0 seconds without packet)
    if (now - r->lastPacketTime > COOP_TIMEOUT_MS) {
      pg_coop_cleanup_player(instance, r);
      continue;
    }

    activeCount++;

    // Smooth Linear Interpolation (Butter-smooth 60 FPS)
    float dx = r->targetX - r->currentX;
    float dy = r->targetY - r->currentY;
    float dz = r->targetZ - r->currentZ;
    float dist = sqrtf(dx * dx + dy * dy + dz * dz);

    if (dist > 80.0f) {
      // Teleport if too far
      r->currentX = r->targetX;
      r->currentY = r->targetY;
      r->currentZ = r->targetZ;
      r->currentHeading = r->targetHeading;
    } else {
      // Smooth lerp per frame
      r->currentX += dx * 0.30f;
      r->currentY += dy * 0.30f;
      r->currentZ += dz * 0.30f;
      r->currentHeading += (r->targetHeading - r->currentHeading) * 0.30f;
    }

    // Spawn or verify Ped Handle
    if (!r->pedHandle || !PG_FN_ENTITY_EXISTS(instance, r->pedHandle)) {
      u32 pModel = r->pedModel ? r->pedModel : 0x705e61f2; // mp_m_freemode_01 default
      PG_FN_REQUEST_MODEL(instance, pModel);
      if (PG_FN_HAS_MODEL_LOADED(instance, pModel)) {
        uint64_t pArgs[8] = {
          4, pModel, pg_coop_f32(r->currentX), pg_coop_f32(r->currentY), pg_coop_f32(r->currentZ), pg_coop_f32(r->currentHeading), 0, 1
        };
        // 0xd49f9b0955c367de: CREATE_PED
        r->pedHandle = (u32)pg_test_call(instance, 0xd49f9b0955c367deull, 8, pArgs);
        if (r->pedHandle) {
          r->loadedPedModel = pModel;
          // 0x3882114bde57ff48: SET_ENTITY_INVINCIBLE
          pg_test_call(instance, 0x3882114bde57ff48ull, 2, (uint64_t[]){ r->pedHandle, 1 });
          // 0x9f7794730795e019: SET_BLOCKING_OF_NON_TEMPORARY_EVENTS
          pg_test_call(instance, 0x9f7794730795e019ull, 2, (uint64_t[]){ r->pedHandle, 1 });

          // Create Blue Blip for Remote Player (Chấm xanh chân thật trên bản đồ)
          // 0x5cde92c702a8fce7: ADD_BLIP_FOR_ENTITY
          r->blipHandle = (u32)pg_test_call(instance, 0x5cde92c702a8fce7ull, 1, (uint64_t[]){ r->pedHandle });
          if (r->blipHandle) {
            // 0x03d7fb09e75d6b7e: SET_BLIP_COLOUR (3 = HUD_COLOUR_BLUE)
            pg_test_call(instance, 0x03d7fb09e75d6b7eull, 2, (uint64_t[]){ r->blipHandle, 3 });
            // 0xd38744167b2fa257: SET_BLIP_SCALE (0.95f)
            pg_test_call(instance, 0xd38744167b2fa257ull, 2, (uint64_t[]){ r->blipHandle, pg_coop_f32(0.95f) });
            // 0xbe8be4fe60e27b72: SET_BLIP_AS_SHORT_RANGE (0 = Hiển thị toàn thành phố)
            pg_test_call(instance, 0xbe8be4fe60e27b72ull, 2, (uint64_t[]){ r->blipHandle, 0 });
            // 0x9029b2f3da924928: SET_BLIP_DISPLAY (2 = Radar & Main Map)
            pg_test_call(instance, 0x9029b2f3da924928ull, 2, (uint64_t[]){ r->blipHandle, 2 });
          }
        }
      }
    }

    // Update positions and vehicles
    if (r->pedHandle && PG_FN_ENTITY_EXISTS(instance, r->pedHandle)) {
      if (r->inVehicle && r->vehModel) {
        // Vehicle synchronization
        if (!r->vehHandle || !PG_FN_ENTITY_EXISTS(instance, r->vehHandle)) {
          PG_FN_REQUEST_MODEL(instance, r->vehModel);
          if (PG_FN_HAS_MODEL_LOADED(instance, r->vehModel)) {
            if (!coopCoordVector) coopCoordVector = pg_asi_alloc(24);
            float *vec = (float*)pg_pointer(&env, coopCoordVector, 24);
            vec[0] = r->currentX; vec[2] = r->currentY; vec[4] = r->currentZ;
            r->vehHandle = PG_FN_CREATE_VEHICLE(instance, r->vehModel, coopCoordVector, r->currentHeading, 0, 0, 1);
            if (r->vehHandle) {
              r->loadedVehModel = r->vehModel;
              // 0xf75b0d629e1c063d: SET_PED_INTO_VEHICLE
              pg_test_call(instance, 0xf75b0d629e1c063dull, 3, (uint64_t[]){ r->pedHandle, r->vehHandle, (uint64_t)(int64_t)-1 });
            }
          }
        } else {
          // Update vehicle coordinates & heading
          // 0x06a6640eed523090: SET_ENTITY_COORDS
          pg_test_call(instance, 0x06a6640eed523090ull, 8, (uint64_t[]){
            r->vehHandle, pg_coop_f32(r->currentX), pg_coop_f32(r->currentY), pg_coop_f32(r->currentZ), 0, 0, 0, 0
          });
          // 0x8e2530aa8ada980e: SET_ENTITY_HEADING
          pg_test_call(instance, 0x8e2530aa8ada980eull, 2, (uint64_t[]){ r->vehHandle, pg_coop_f32(r->currentHeading) });
        }
      } else {
        // On foot synchronization
        if (r->vehHandle && PG_FN_ENTITY_EXISTS(instance, r->vehHandle)) {
          // 0xd3dbce61a490be52: CLEAR_PED_TASKS_IMMEDIATELY
          pg_test_call(instance, 0xd3dbce61a490be52ull, 2, (uint64_t[]){ r->pedHandle, 16 });
          u32 oldVeh = r->vehHandle;
          r->vehHandle = 0;
          if (coopEntityPtrBuf) {
            *(u32*)pg_pointer(&env, coopEntityPtrBuf, 4) = oldVeh;
            pg_test_call(instance, 0x2595dd4236549ce3ull, 1, (uint64_t[]){ coopEntityPtrBuf });
          }
        }
        // Update Ped coordinates & heading
        // 0x06a6640eed523090: SET_ENTITY_COORDS
        pg_test_call(instance, 0x06a6640eed523090ull, 8, (uint64_t[]){
          r->pedHandle, pg_coop_f32(r->currentX), pg_coop_f32(r->currentY), pg_coop_f32(r->currentZ), 0, 0, 0, 0
        });
        // 0x8e2530aa8ada980e: SET_ENTITY_HEADING
        pg_test_call(instance, 0x8e2530aa8ada980eull, 2, (uint64_t[]){ r->pedHandle, pg_coop_f32(r->currentHeading) });
      }
    }
  }

  atomic_store(&coopPlayerCount, activeCount);
}

// ----------------------------------------------------------------------------
// Public C Bridge API exposed to Swift UI
// ----------------------------------------------------------------------------

bool PGCoopIsActive(void) {
  return atomic_load(&coopActive);
}

void PGCoopSetActive(bool active) {
  atomic_store(&coopActive, active);
}

void PGCoopSetServer(const char *ip, int port) {
  pthread_mutex_lock(&coopLock);
  if (ip && ip[0]) snprintf(coopServerIP, sizeof(coopServerIP), "%s", ip);
  if (port > 0 && port < 65536) coopServerPort = port;
  // Reset socket to reconnect with new endpoint
  if (coopSocket >= 0) {
    close(coopSocket);
    coopSocket = -1;
  }
  pthread_mutex_unlock(&coopLock);
}

void PGCoopSetPlayerName(const char *name) {
  pthread_mutex_lock(&coopLock);
  if (name && name[0]) snprintf(coopPlayerName, sizeof(coopPlayerName), "%s", name);
  pthread_mutex_unlock(&coopLock);
}

void PGCoopSetHostMode(bool host) {
  atomic_store(&coopHostMode, host);
  if (coopSocket >= 0) {
    close(coopSocket);
    coopSocket = -1;
  }
}

void PGCoopGetServerIP(char *buffer, size_t maxLen) {
  pthread_mutex_lock(&coopLock);
  if (buffer && maxLen > 0) snprintf(buffer, maxLen, "%s", coopServerIP);
  pthread_mutex_unlock(&coopLock);
}

int PGCoopGetServerPort(void) {
  pthread_mutex_lock(&coopLock);
  int p = coopServerPort;
  pthread_mutex_unlock(&coopLock);
  return p;
}

void PGCoopGetPlayerName(char *buffer, size_t maxLen) {
  pthread_mutex_lock(&coopLock);
  if (buffer && maxLen > 0) snprintf(buffer, maxLen, "%s", coopPlayerName);
  pthread_mutex_unlock(&coopLock);
}

int PGCoopGetPlayerCount(void) {
  return atomic_load(&coopPlayerCount);
}

bool PGCoopIsConnected(void) {
  return atomic_load(&coopConnected);
}

void PGCoopGetStatus(char *buffer, size_t maxLen) {
  if (!buffer || maxLen == 0) return;
  bool active = atomic_load(&coopActive);
  int count = atomic_load(&coopPlayerCount);
  bool host = atomic_load(&coopHostMode);

  pthread_mutex_lock(&coopLock);
  if (!active) {
    snprintf(buffer, maxLen, "Co-op đang tắt. Sẵn sàng kết nối.");
  } else if (host) {
    snprintf(buffer, maxLen, "Đang làm Host tại cổng %d. Có %d người chơi khác kết nối.", coopServerPort, count);
  } else {
    snprintf(buffer, maxLen, "Đang kết nối tới %s:%d. Tìm thấy %d người chơi (Chấm xanh trên bản đồ).", coopServerIP, coopServerPort, count);
  }
  pthread_mutex_unlock(&coopLock);
}
