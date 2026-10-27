// Native LSPDFR Police System for GTA V iOS Native Port
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdatomic.h>

#define LSPDFR_MAX_SUSPECTS 6

typedef enum { LSPDFR_AGENCY_LSPD = 0, LSPDFR_AGENCY_LSSD = 1, LSPDFR_AGENCY_SAHP = 2, LSPDFR_AGENCY_SWAT = 3, LSPDFR_AGENCY_FIB = 4 } LSPDFRAgency;
typedef enum { LSPDFR_CALLOUT_NONE = 0, LSPDFR_CALLOUT_PURSUIT = 1, LSPDFR_CALLOUT_ROBBERY = 2, LSPDFR_CALLOUT_SHOOTOUT = 3, LSPDFR_CALLOUT_STOLEN_COP = 4 } LSPDFRCalloutType;
typedef enum { LSPDFR_BACKUP_LOCAL = 1, LSPDFR_BACKUP_SWAT = 2, LSPDFR_BACKUP_AIR = 3, LSPDFR_BACKUP_EMS = 4 } LSPDFRBackupType;

void PG_FN_SET_WEATHER(w2c_game*, u64);
void PG_FN_CLEAR_WEATHER_OVERRIDE(w2c_game*);
void PG_FN_SET_CLOCK(w2c_game*, u32, u32, u32);
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
void PG_FN_SET_PED_INTO_VEHICLE(w2c_game*, u32, u32, u32);
void PG_FN_VEHICLE_ENGINE_HEALTH(w2c_game*, u32, f32);
void PG_FN_GIVE_WEAPON(w2c_game*, u32, u32, u32, u32, u32);
u32 PG_FN_ENTITY_EXISTS(w2c_game*, u32);
void PG_FN_SET_ENTITY_HEALTH(w2c_game*, u32, u32, u32);
u32 PG_FN_GET_ENTITY_MODEL(w2c_game*, u32);

static const char *const lspdfrVehicles[] = {
  "police", "police2", "police3", "police4", "sheriff", "sheriff2", "policet", "riot", "fbi", "fbi2", "policeb", "polmav"
};
static const uint32_t lspdfrUniforms[] = { 0x5E3DA4A4, 0x1403DDA5, 0x44664796, 0x8D8F1B10, 0x3C2A8647 };

static _Atomic bool lspdfrOnDuty = false;
static _Atomic int lspdfrAgency = LSPDFR_AGENCY_LSPD;
static _Atomic int lspdfrQueuedVehicle = -1;
static _Atomic int lspdfrQueuedCallout = 0;
static _Atomic int lspdfrQueuedBackup = 0;
static _Atomic bool lspdfrQueuedTrafficStop = false;
static _Atomic bool lspdfrQueuedArrest = false;
static _Atomic bool lspdfrQueuedTransport = false;
static _Atomic bool lspdfrRadarActive = false;
static _Atomic uint32_t lspdfrCustomVehHash = 0;
static uint64_t lspdfrScratchMem = 0;

static int activeCallout = LSPDFR_CALLOUT_NONE;
static u32 suspectPeds[LSPDFR_MAX_SUSPECTS];
static u32 suspectVeh = 0;
static u32 suspectBlips[LSPDFR_MAX_SUSPECTS + 1];
static double calloutStartTime = 0;
static u32 pulledVehicle = 0, pulledDriver = 0;
static char radarInfoText[128] = "RADAR: SAN SANG TUAN TRA";

static uint64_t pg_lspdfr_call(w2c_game *instance, uint64_t hash, const uint64_t *args, uint32_t count) {
  if (!pg_asi_game) pg_asi_game = instance;
  static uint64_t buf;
  if (!buf) buf = PG_FN_OPERATOR_NEW(instance, 8 * 40);
  if (!buf) return 0;
  if (count > 32) count = 32;
  if (count) memcpy(pg_pointer(&env, buf, 8ull * count), args, 8ull * count);
  memset(pg_pointer(&env, buf + 8 * 32, 64), 0, 64);
  if (!pg_asi_native(hash, buf, count, buf + 8 * 32)) return 0;
  uint64_t val = 0; memcpy(&val, pg_pointer(&env, buf + 8 * 32, 8), 8);
  return val;
}

static void pg_lspdfr_maintain_police_immunity(w2c_game *instance, u32 playerPed) {
  u32 player = PG_FN_PLAYER_ID(instance);
  if (player < 32) {
    PG_FN_SET_MAX_WANTED(instance, 0);
    if (PG_FN_GET_WANTED(instance, player) != 0) PG_FN_CLEAR_WANTED(instance, player);
    uint64_t pArgs[2] = { player, 1 }; pg_lspdfr_call(instance, 0x32c62aa929c2da6aull, pArgs, 2);
    uint64_t dArgs[2] = { player, 0 }; pg_lspdfr_call(instance, 0xdb172424876553f4ull, dArgs, 2);
  }
  if (playerPed && PG_FN_ENTITY_EXISTS(instance, playerPed)) {
    uint64_t copArgs[2] = { playerPed, 1 }; pg_lspdfr_call(instance, 0xb736a4e61c3be30full, copArgs, 2);
    uint64_t targetArgs[2] = { playerPed, 0 }; pg_lspdfr_call(instance, 0xbf16658d4a946808ull, targetArgs, 2);
  }
}

static void pg_lspdfr_equip_duty_weapons(w2c_game *instance, u32 playerPed) {
  if (!playerPed || !PG_FN_ENTITY_EXISTS(instance, playerPed)) return;
  PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_stungun"), 100, 0, 0);
  PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_combatpistol"), 250, 0, 1);
  PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_carbinerifle"), 450, 0, 0);
  PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_pumpshotgun"), 120, 0, 0);
  PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_nightstick"), 1, 0, 0);
  PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_flashlight"), 1, 0, 0);
  PG_FN_GIVE_WEAPON(instance, playerPed, pg_joaat("weapon_flare"), 20, 0, 0);
  uint64_t armorArgs[2] = { playerPed, 100 }; pg_lspdfr_call(instance, 0xcea04d83130e2d61ull, armorArgs, 2);
}

static void pg_lspdfr_apply_agency_uniform(w2c_game *instance, u32 playerPed, int agency) {
  if (!playerPed || agency < 0 || agency > 4) return;
  uint32_t model = lspdfrUniforms[agency];
  PG_FN_REQUEST_MODEL(instance, model);
  if (!PG_FN_HAS_MODEL_LOADED(instance, model)) return;
  u32 player = PG_FN_PLAYER_ID(instance);
  if (player < 32) {
    uint64_t mArgs[2] = { player, model }; pg_lspdfr_call(instance, 0x00a1cadd00108836ull, mArgs, 2);
    u32 newPed = PG_FN_PLAYER_PED_ID(instance);
    if (newPed) {
      uint64_t vArgs[1] = { newPed }; pg_lspdfr_call(instance, 0x45eee61580806d63ull, vArgs, 1);
      pg_lspdfr_equip_duty_weapons(instance, newPed);
    }
  }
}

static void pg_lspdfr_spawn_police_vehicle(w2c_game *instance, u32 playerPed, int vehIndex, uint32_t customHash) {
  if (!playerPed || !PG_FN_ENTITY_EXISTS(instance, playerPed)) return;
  uint32_t modelHash = customHash;
  if (!modelHash) {
    if (vehIndex < 0 || vehIndex >= 12) vehIndex = 0;
    modelHash = pg_joaat(lspdfrVehicles[vehIndex]);
  }
  PG_FN_REQUEST_MODEL(instance, modelHash);
  if (!PG_FN_HAS_MODEL_LOADED(instance, modelHash)) return;

  PG_FN_GET_ENTITY_COORDS(instance, lspdfrScratchMem, playerPed, 1);
  float *coords = (float *)pg_pointer(&env, lspdfrScratchMem, 24);
  float heading = PG_FN_GET_ENTITY_HEADING(instance, playerPed);
  float rad = heading * (3.14159265f / 180.0f);
  float *vcoords = (float *)pg_pointer(&env, lspdfrScratchMem + 32, 24);
  vcoords[0] = coords[0] - sinf(rad) * 6.5f;
  vcoords[1] = coords[1] + cosf(rad) * 6.5f;
  vcoords[2] = coords[2] + 0.4f;

  u32 veh = PG_FN_CREATE_VEHICLE(instance, modelHash, lspdfrScratchMem + 32, heading, 1, 0, 1);
  if (veh && PG_FN_ENTITY_EXISTS(instance, veh)) {
    PG_FN_VEHICLE_ENGINE_HEALTH(instance, veh, 1000.0f);
    uint64_t fixArgs[1] = { veh }; pg_lspdfr_call(instance, 0x115722b1ull, fixArgs, 1);
    uint64_t cleanArgs[2] = { veh, 0 }; pg_lspdfr_call(instance, 0x28d1a16aull, cleanArgs, 2);
    uint64_t sirenArgs[2] = { veh, 1 }; pg_lspdfr_call(instance, 0xf492490e49079de9ull, sirenArgs, 2);
    PG_FN_SET_PED_INTO_VEHICLE(instance, playerPed, veh, (u32)-1);
    uint64_t blipArgs[1] = { veh };
    u32 blip = (u32)pg_lspdfr_call(instance, 0x5cde92c702a8fce7ull, blipArgs, 1);
    if (blip) {
      uint64_t colorArgs[2] = { blip, 3 }; pg_lspdfr_call(instance, 0x03d7fb09e75d6b7eull, colorArgs, 2);
    }
  }
}

static void pg_lspdfr_start_callout(w2c_game *instance, u32 playerPed, int type) {
  if (!playerPed || type == LSPDFR_CALLOUT_NONE) return;
  for (int i = 0; i < LSPDFR_MAX_SUSPECTS; i++) suspectPeds[i] = 0;
  suspectVeh = 0;

  PG_FN_GET_ENTITY_COORDS(instance, lspdfrScratchMem, playerPed, 1);
  float *pCoords = (float *)pg_pointer(&env, lspdfrScratchMem, 24);
  float heading = PG_FN_GET_ENTITY_HEADING(instance, playerPed);
  float rad = (heading + 15.0f) * (3.14159265f / 180.0f);
  float cx = pCoords[0] - sinf(rad) * 120.0f;
  float cy = pCoords[1] + cosf(rad) * 120.0f;
  float cz = pCoords[2];
  float *cCoords = (float *)pg_pointer(&env, lspdfrScratchMem + 64, 24);
  cCoords[0] = cx; cCoords[1] = cy; cCoords[2] = cz;

  uint64_t soundArgs[2] = { 0, 0 }; pg_lspdfr_call(instance, 0x67c540aa08e4a6f5ull, soundArgs, 2);

  if (type == LSPDFR_CALLOUT_PURSUIT || type == LSPDFR_CALLOUT_STOLEN_COP) {
    uint32_t carHash = (type == LSPDFR_CALLOUT_STOLEN_COP) ? pg_joaat("police3") : pg_joaat("fugitive");
    uint32_t driverHash = 0x20557434;
    PG_FN_REQUEST_MODEL(instance, carHash);
    PG_FN_REQUEST_MODEL(instance, driverHash);
    if (PG_FN_HAS_MODEL_LOADED(instance, carHash) && PG_FN_HAS_MODEL_LOADED(instance, driverHash)) {
      suspectVeh = PG_FN_CREATE_VEHICLE(instance, carHash, lspdfrScratchMem + 64, heading, 1, 0, 1);
      if (suspectVeh) {
        uint32_t bx, by, bz; memcpy(&bx, &cx, 4); memcpy(&by, &cy, 4); memcpy(&bz, &cz, 4);
        uint64_t pedArgs[8] = { 26, driverHash, bx, by, bz, 0, 1, 1 };
        u32 driver = (u32)pg_lspdfr_call(instance, 0xd49f36e4d04c2b82ull, pedArgs, 8);
        if (driver) {
          PG_FN_SET_PED_INTO_VEHICLE(instance, driver, suspectVeh, (u32)-1);
          suspectPeds[0] = driver;
          uint64_t fleeArgs[5] = { driver, suspectVeh, 0, 45.0f, 0x400C0025 };
          pg_lspdfr_call(instance, 0x3c08aebd8e070f12ull, fleeArgs, 5);
          uint64_t bArgs[1] = { suspectVeh };
          u32 blip = (u32)pg_lspdfr_call(instance, 0x5cde92c702a8fce7ull, bArgs, 1);
          if (blip) {
            uint64_t cArgs[2] = { blip, 1 }; pg_lspdfr_call(instance, 0x03d7fb09e75d6b7eull, cArgs, 2);
            uint64_t rArgs[2] = { blip, 1 }; pg_lspdfr_call(instance, 0x4e929e7a5796de40ull, rArgs, 2);
            suspectBlips[0] = blip;
          }
        }
      }
    }
  } else if (type == LSPDFR_CALLOUT_ROBBERY || type == LSPDFR_CALLOUT_SHOOTOUT) {
    uint32_t gangHash = 0x401BC182;
    PG_FN_REQUEST_MODEL(instance, gangHash);
    if (PG_FN_HAS_MODEL_LOADED(instance, gangHash)) {
      for (int i = 0; i < 2; i++) {
        float ox = cx + (i * 3.5f), oy = cy + (i * 2.0f);
        uint32_t bx, by, bz; memcpy(&bx, &ox, 4); memcpy(&by, &oy, 4); memcpy(&bz, &cz, 4);
        uint64_t pedArgs[8] = { 26, gangHash, bx, by, bz, 0, 1, 1 };
        u32 robber = (u32)pg_lspdfr_call(instance, 0xd49f36e4d04c2b82ull, pedArgs, 8);
        if (robber) {
          suspectPeds[i] = robber;
          PG_FN_GIVE_WEAPON(instance, robber, pg_joaat("weapon_microsmg"), 300, 0, 1);
          uint64_t combatArgs[3] = { robber, playerPed, 0 }; pg_lspdfr_call(instance, 0xf166e48407bac484ull, combatArgs, 3);
          uint64_t bArgs[1] = { robber };
          u32 blip = (u32)pg_lspdfr_call(instance, 0x5cde92c702a8fce7ull, bArgs, 1);
          if (blip) {
            uint64_t cArgs[2] = { blip, 1 }; pg_lspdfr_call(instance, 0x03d7fb09e75d6b7eull, cArgs, 2);
            uint64_t rArgs[2] = { blip, 1 }; pg_lspdfr_call(instance, 0x4e929e7a5796de40ull, rArgs, 2);
            suspectBlips[i] = blip;
          }
        }
      }
    }
  }
  activeCallout = type; calloutStartTime = timeMS(CLOCK_MONOTONIC);
}

static void pg_lspdfr_dispatch_backup(w2c_game *instance, u32 playerPed, int type) {
  if (!playerPed || !PG_FN_ENTITY_EXISTS(instance, playerPed)) return;
  PG_FN_GET_ENTITY_COORDS(instance, lspdfrScratchMem, playerPed, 1);
  float *coords = (float *)pg_pointer(&env, lspdfrScratchMem, 24);
  float heading = PG_FN_GET_ENTITY_HEADING(instance, playerPed);
  float rad = (heading + 180.0f) * (3.14159265f / 180.0f);
  float bx = coords[0] - sinf(rad) * 45.0f, by = coords[1] + cosf(rad) * 45.0f, bz = coords[2] + 0.5f;
  float *bCoords = (float *)pg_pointer(&env, lspdfrScratchMem + 32, 24);
  bCoords[0] = bx; bCoords[1] = by; bCoords[2] = bz;

  uint32_t vehModel = pg_joaat("police"), pedModel = 0x5E3DA4A4;
  if (type == LSPDFR_BACKUP_SWAT) { vehModel = pg_joaat("riot"); pedModel = 0x8D8F1B10; }
  else if (type == LSPDFR_BACKUP_AIR) { vehModel = pg_joaat("polmav"); bz += 35.0f; bCoords[2] = bz; }
  else if (type == LSPDFR_BACKUP_EMS) { vehModel = pg_joaat("ambulance"); pedModel = 0x118D6308; }

  PG_FN_REQUEST_MODEL(instance, vehModel);
  PG_FN_REQUEST_MODEL(instance, pedModel);
  if (PG_FN_HAS_MODEL_LOADED(instance, vehModel) && PG_FN_HAS_MODEL_LOADED(instance, pedModel)) {
    u32 bVeh = PG_FN_CREATE_VEHICLE(instance, vehModel, lspdfrScratchMem + 32, heading, 1, 0, 1);
    if (bVeh) {
      uint64_t sirenArgs[2] = { bVeh, 1 }; pg_lspdfr_call(instance, 0xf492490e49079de9ull, sirenArgs, 2);
      uint32_t px, py, pz_bits; memcpy(&px, &bx, 4); memcpy(&py, &by, 4); memcpy(&pz_bits, &bz, 4);
      uint64_t pArgs[8] = { 6, pedModel, px, py, pz_bits, 0, 1, 1 };
      u32 cop = (u32)pg_lspdfr_call(instance, 0xd49f36e4d04c2b82ull, pArgs, 8);
      if (cop) {
        PG_FN_SET_PED_INTO_VEHICLE(instance, cop, bVeh, (u32)-1);
        if (type == LSPDFR_BACKUP_SWAT) PG_FN_GIVE_WEAPON(instance, cop, pg_joaat("weapon_carbinerifle"), 300, 0, 1);
        else PG_FN_GIVE_WEAPON(instance, cop, pg_joaat("weapon_combatpistol"), 150, 0, 1);
        uint64_t escortArgs[4] = { cop, playerPed, -1, 1 };
        pg_lspdfr_call(instance, 0x3c08aebd8e070f12ull, escortArgs, 4);
      }
    }
  }
}

static void pg_lspdfr_execute_traffic_stop(w2c_game *instance, u32 playerPed) {
  if (!playerPed) return;
  PG_FN_GET_ENTITY_COORDS(instance, lspdfrScratchMem, playerPed, 1);
  float *coords = (float *)pg_pointer(&env, lspdfrScratchMem, 24);
  if (pulledVehicle && PG_FN_ENTITY_EXISTS(instance, pulledVehicle)) {
    if (pulledDriver && PG_FN_ENTITY_EXISTS(instance, pulledDriver)) {
      uint64_t wanderArgs[4] = { pulledDriver, pulledVehicle, 18.0f, 786603 };
      pg_lspdfr_call(instance, 0x502ec17e171b3e12ull, wanderArgs, 4);
    }
    pulledVehicle = pulledDriver = 0;
    snprintf(radarInfoText, sizeof(radarInfoText), "TRAFFIC STOP: DA GIAI TOA XE");
    return;
  }
  uint64_t vehArgs[4] = { coords[0], coords[1], coords[2], 25.0f };
  u32 targetVeh = (u32)pg_lspdfr_call(instance, 0x30ec860b8de9f412ull, vehArgs, 4);
  if (targetVeh && PG_FN_ENTITY_EXISTS(instance, targetVeh)) {
    pulledVehicle = targetVeh;
    uint64_t pedInVehArgs[2] = { targetVeh, (u32)-1 };
    pulledDriver = (u32)pg_lspdfr_call(instance, 0x388fde9def71d753ull, pedInVehArgs, 2);
    if (pulledDriver && PG_FN_ENTITY_EXISTS(instance, pulledDriver)) {
      uint64_t stopArgs[4] = { pulledDriver, targetVeh, 6, 30000 };
      pg_lspdfr_call(instance, 0x502ec17e171b3e12ull, stopArgs, 4);
      uint64_t hazardArgs[2] = { targetVeh, 1 };
      pg_lspdfr_call(instance, 0xc07a82b3d6837a77ull, hazardArgs, 2);
      snprintf(radarInfoText, sizeof(radarInfoText), "TRAFFIC STOP: DA YEU CAU DUNG XE");
    }
  }
}

static void pg_lspdfr_execute_arrest_and_transport(w2c_game *instance, u32 playerPed, bool callTransport) {
  if (!playerPed) return;
  for (int i = 0; i < LSPDFR_MAX_SUSPECTS; i++) {
    u32 suspect = suspectPeds[i];
    if (suspect && PG_FN_ENTITY_EXISTS(instance, suspect)) {
      if (!callTransport) {
        uint64_t handsArgs[5] = { suspect, -1, playerPed, 0, 0 };
        pg_lspdfr_call(instance, 0xc2e775b3213a3184ull, handsArgs, 5);
        snprintf(radarInfoText, sizeof(radarInfoText), "ARREST: DOI TUONG DA DAU HANG");
      } else {
        pg_lspdfr_dispatch_backup(instance, playerPed, LSPDFR_BACKUP_LOCAL);
        snprintf(radarInfoText, sizeof(radarInfoText), "TRANSPORT: DANG AP GIAI VE DON");
      }
      break;
    }
  }
}

static void pg_lspdfr_update_radar(w2c_game *instance, u32 playerPed) {
  if (!lspdfrRadarActive || !playerPed) return;
  PG_FN_GET_ENTITY_COORDS(instance, lspdfrScratchMem, playerPed, 1);
  float *coords = (float *)pg_pointer(&env, lspdfrScratchMem, 24);
  uint64_t vehArgs[4] = { coords[0], coords[1], coords[2], 35.0f };
  u32 targetVeh = (u32)pg_lspdfr_call(instance, 0x30ec860b8de9f412ull, vehArgs, 4);
  if (targetVeh && PG_FN_ENTITY_EXISTS(instance, targetVeh)) {
    uint64_t speedArgs[1] = { targetVeh };
    float speed = (float)pg_lspdfr_call(instance, 0xab54a438726d3f55ull, speedArgs, 1);
    int mph = (int)(speed * 2.236936f);
    snprintf(radarInfoText, sizeof(radarInfoText), "RADAR: XE PHIA TRUOC [%d MPH] - TRANG THAI: HOP LE", mph);
  }
}

void pg_lspdfr_tick(w2c_game *instance) {
  if (PGPauseMenuActive() || PGLoadSceneActive()) return;
  if (PGGameStatus() & (16 | 4 | 2 | 32)) return;

  u32 playerPed = PG_FN_PLAYER_PED_ID(instance);
  if (!playerPed || !PG_FN_ENTITY_EXISTS(instance, playerPed)) return;
  if (!pg_asi_game) pg_asi_game = instance;
  if (!lspdfrScratchMem) lspdfrScratchMem = PG_FN_OPERATOR_NEW(instance, 512);
  if (!lspdfrScratchMem) return;

  if (lspdfrOnDuty) {
    u64 prev = pg_asi_enter_script(instance);
    pg_lspdfr_maintain_police_immunity(instance, playerPed);

    int reqVeh = atomic_exchange(&lspdfrQueuedVehicle, -1);
    uint32_t customHash = atomic_exchange(&lspdfrCustomVehHash, 0);
    if (reqVeh >= 0 || customHash != 0) pg_lspdfr_spawn_police_vehicle(instance, playerPed, reqVeh, customHash);

    int reqCallout = atomic_exchange(&lspdfrQueuedCallout, 0);
    if (reqCallout != 0) pg_lspdfr_start_callout(instance, playerPed, reqCallout);

    int reqBackup = atomic_exchange(&lspdfrQueuedBackup, 0);
    if (reqBackup != 0) pg_lspdfr_dispatch_backup(instance, playerPed, reqBackup);

    if (atomic_exchange(&lspdfrQueuedTrafficStop, false)) pg_lspdfr_execute_traffic_stop(instance, playerPed);
    if (atomic_exchange(&lspdfrQueuedArrest, false)) pg_lspdfr_execute_arrest_and_transport(instance, playerPed, false);
    if (atomic_exchange(&lspdfrQueuedTransport, false)) pg_lspdfr_execute_arrest_and_transport(instance, playerPed, true);

    pg_lspdfr_update_radar(instance, playerPed);

    if (activeCallout != LSPDFR_CALLOUT_NONE) {
      bool allNeutralized = true;
      for (int i = 0; i < LSPDFR_MAX_SUSPECTS; i++) {
        u32 s = suspectPeds[i];
        if (s && PG_FN_ENTITY_EXISTS(instance, s)) {
          uint64_t deadArgs[1] = { s };
          bool isDead = (bool)pg_lspdfr_call(instance, 0x331de89284224890ull, deadArgs, 1);
          if (!isDead) { allNeutralized = false; break; }
        }
      }
      if (allNeutralized && (timeMS(CLOCK_MONOTONIC) - calloutStartTime > 4000)) {
        activeCallout = LSPDFR_CALLOUT_NONE;
        snprintf(radarInfoText, sizeof(radarInfoText), "NHIEM VU HOAN THANH: DOI TUONG DA BI BAT");
      }
    }
    pg_asi_leave_script(instance, prev);
  }
}

bool PGLSPDFRIsOnDuty(void) { return atomic_load(&lspdfrOnDuty); }
void PGLSPDFRSetDuty(bool onDuty) { atomic_store(&lspdfrOnDuty, onDuty); }
void PGLSPDFRSetAgency(int agency) { atomic_store(&lspdfrAgency, agency); }
int PGLSPDFRGetAgency(void) { return atomic_load(&lspdfrAgency); }
void PGLSPDFRSpawnVehicle(int index) { atomic_store(&lspdfrQueuedVehicle, index); }
void PGLSPDFRSpawnCustomVehicle(uint32_t modelHash) { atomic_store(&lspdfrCustomVehHash, modelHash); }
void PGLSPDFRTriggerCallout(int calloutType) { atomic_store(&lspdfrQueuedCallout, calloutType); }
void PGLSPDFRRequestBackup(int backupType) { atomic_store(&lspdfrQueuedBackup, backupType); }
void PGLSPDFRTrafficStop(void) { atomic_store(&lspdfrQueuedTrafficStop, true); }
void PGLSPDFRArrestNearby(void) { atomic_store(&lspdfrQueuedArrest, true); }
void PGLSPDFRCallTransport(void) { atomic_store(&lspdfrQueuedTransport, true); }
void PGLSPDFRToggleRadar(void) { atomic_store(&lspdfrRadarActive, !atomic_load(&lspdfrRadarActive)); }
bool PGLSPDFRIsRadarActive(void) { return atomic_load(&lspdfrRadarActive); }
void PGLSPDFRGetStatus(char *buffer, size_t maxLen) {
  if (!buffer || maxLen == 0) return;
  snprintf(buffer, maxLen, "%s", radarInfoText);
}
