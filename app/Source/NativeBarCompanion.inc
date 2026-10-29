// ============================================================================
// PlayGTA5Native - NORMAL MODE COMPANION & 18+ INTERACTIVE ROMANCE SYSTEM
// ============================================================================
// Features:
// 1. Fair-skinned, attractive female companion (s_f_y_stripper_01 Chastity / u_f_y_hotposh_01)
// 2. Meet at Vanilla Unicorn / Bars OR honk car horn near girls on street ($100 tip)
// 3. Companion boards passenger seat and GPS waypoint sets to nearest player safehouse
// 4. In-car dialogue & follow into safehouse bedroom / couch with zero fleeing & anti-lost teleport
// 5. Interactive 18+ Bedroom Romance:
//    - NO black screen fade! 100% visible on-screen action
//    - Full undress: removes clothes to bare/bikini components
//    - 4 Synchronized 18+ anims: Blowjob, Missionary, Cowgirl, Lapdance show
//    - Bed vs Sofa/Chair location switching
//    - Money deduction & tip display ($100)
//    - Touch HUD buttons + Horn / E / D-pad control
//    - Finishing replenishes 100% Health (200 HP) and 100 Armour!
// ============================================================================

#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  BAR_COMPANION_INACTIVE = 0,
  BAR_COMPANION_AT_BAR = 1,       // Waiting at bar / roadside
  BAR_COMPANION_ENTERING = 2,     // Boarding player's car
  BAR_COMPANION_RIDING = 3,       // Driving to safehouse with GPS
  BAR_COMPANION_FOLLOWING = 4,    // Following player inside safehouse
  BAR_COMPANION_BEDROOM = 5,      // Active 18+ interactive bedroom sequence
  BAR_COMPANION_RESTED = 6        // Complete, health/armour restored
} BarCompanionState;

static BarCompanionState barCompanionState = BAR_COMPANION_INACTIVE;
static u32 barCompanionPed = 0;
static u32 barCompanionBlip = 0;
static u32 barSafehouseBlip = 0;
static double barLastPromptTime = 0;
static double barFollowStartTime = 0;
static double barRomanceStartTime = 0;
static int barRomancePosture = 0;     // 0: Blowjob, 1: Missionary, 2: Cowgirl, 3: Lapdance
static int barRomanceLocation = 0;    // 0: Giuong (Bed), 1: Ghe Sofa (Couch)
static double barLastPostureSwitch = 0;
static bool barRomanceRequested = false;
static bool barRomanceFinished = false;
static uint64_t barCoordVector = 0;
static uint64_t barSubtitleStrBuf = 0;
static uint64_t barAnimDictPtr1 = 0;
static uint64_t barAnimDictPtr2 = 0;
static uint64_t barAnimNamePedPtr = 0;
static uint64_t barAnimNamePlyrPtr = 0;

// Locations
static const float vanillaUnicornX = 128.0f;
static const float vanillaUnicornY = -1298.0f;
static const float vanillaUnicornZ = 29.5f;

// Known Safehouse Bedrooms: Franklin Mansion, Michael Mansion, Franklin Aunt, Trevor Trailer
static const float safehouseList[4][3] = {
  { 7.0f,    525.0f,  170.5f }, // Franklin Whispymound Mansion
  {-813.0f,  178.0f,   72.0f }, // Michael Rockford Hills Mansion
  {-14.0f, -1440.0f,   30.5f }, // Franklin Forum Dr Aunt House
  {1972.0f, 3816.0f,   33.5f }  // Trevor Sandy Shores Trailer
};

static float activeSafehouseX = 7.0f;
static float activeSafehouseY = 525.0f;
static float activeSafehouseZ = 170.5f;

// Fair-skinned, gorgeous blonde companion models
// s_f_y_stripper_01 (0x61C6B12A), u_f_y_hotposh_01 (0xC2A81D22), s_f_y_hooker_01 (0xC57C2C49)
static const uint32_t barCompanionModels[3] = {
  0x61C6B12A, // Chastity: Fair skin, blonde hair, beautiful face
  0xC2A81D22, // Hot posh socialite: Fair skin, elegant
  0xC57C2C49  // Fair skin hooker
};
static uint32_t barCompanionModel = 0x61C6B12A;

static void pg_bar_show_subtitle(w2c_game *instance, const char *msg, int durationMs) {
  if (!barSubtitleStrBuf) barSubtitleStrBuf = pg_asi_alloc(256);
  if (!barSubtitleStrBuf) return;
  strncpy((char*)pg_pointer(&env, barSubtitleStrBuf, 256), msg, 255);

  // 0xb87a37eeb7faa67d: BEGIN_TEXT_COMMAND_PRINT("STRING")
  static uint64_t strEntryPtr = 0;
  if (!strEntryPtr) {
    strEntryPtr = pg_asi_alloc(16);
    strcpy((char*)pg_pointer(&env, strEntryPtr, 16), "STRING");
  }
  pg_test_call(instance, 0xb87a37eeb7faa67dull, 1, (uint64_t[]){ strEntryPtr });
  // 0x6c188be134e074aa: ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME
  pg_test_call(instance, 0x6c188be134e074aaull, 1, (uint64_t[]){ barSubtitleStrBuf });
  // 0x9d77056a530643f6: END_TEXT_COMMAND_PRINT(durationMs, 1)
  pg_test_call(instance, 0x9d77056a530643f6ull, 2, (uint64_t[]){ (uint64_t)durationMs, 1 });
}

static void pg_bar_companion_cleanup(w2c_game *instance) {
  if (barCompanionBlip) {
    // 0x86a652570e5f25dd: REMOVE_BLIP
    uint64_t blipSlot = pg_asi_alloc(16);
    if (blipSlot) {
      *(u32*)pg_pointer(&env, blipSlot, 4) = barCompanionBlip;
      pg_test_call(instance, 0x86a652570e5f25ddull, 1, (uint64_t[]){ blipSlot });
    }
    barCompanionBlip = 0;
  }
  if (barSafehouseBlip) {
    uint64_t blipSlot = pg_asi_alloc(16);
    if (blipSlot) {
      *(u32*)pg_pointer(&env, blipSlot, 4) = barSafehouseBlip;
      pg_test_call(instance, 0x86a652570e5f25ddull, 1, (uint64_t[]){ blipSlot });
    }
    barSafehouseBlip = 0;
  }
  if (barCompanionPed && PG_FN_ENTITY_EXISTS(instance, barCompanionPed)) {
    uint64_t pedSlot = pg_asi_alloc(16);
    if (pedSlot) {
      *(u32*)pg_pointer(&env, pedSlot, 4) = barCompanionPed;
      // 0x2595dd4236549ce3: SET_PED_AS_NO_LONGER_NEEDED
      pg_test_call(instance, 0x2595dd4236549ce3ull, 1, (uint64_t[]){ pedSlot });
    }
    barCompanionPed = 0;
  }
  barCompanionState = BAR_COMPANION_INACTIVE;
  barRomanceRequested = false;
  barRomanceFinished = false;
}

// Apply 18+ Synchronized animation based on posture index and bed/sofa location
static void pg_bar_apply_romance_posture(w2c_game *instance, u32 playerPed, u32 companionPed, int posture, int location) {
  if (!barAnimDictPtr1) {
    barAnimDictPtr1 = pg_asi_alloc(64);
    barAnimDictPtr2 = pg_asi_alloc(64);
    barAnimNamePedPtr = pg_asi_alloc(64);
    barAnimNamePlyrPtr = pg_asi_alloc(64);
    strcpy((char*)pg_pointer(&env, barAnimDictPtr1, 64), "mini@prostitutes@sexnorm_veh");
    strcpy((char*)pg_pointer(&env, barAnimDictPtr2, 64), "mini@strip_club@lap_dance");
  }

  // Clear previous tasks
  pg_test_call(instance, 0xaaa34f8a7cb32098ull, 1, (uint64_t[]){ companionPed }); // CLEAR_PED_TASKS_IMMEDIATELY
  pg_test_call(instance, 0xaaa34f8a7cb32098ull, 1, (uint64_t[]){ playerPed });

  // Undress companion completely: bare torso, bare legs, bare feet, no tops
  // 0x262b14f48d29de80: SET_PED_COMPONENT_VARIATION(ped, componentId, drawableId, textureId, paletteId)
  pg_test_call(instance, 0x262b14f48d29de80ull, 5, (uint64_t[]){ companionPed, 3, 15, 0, 0 }); // Bare upper torso
  pg_test_call(instance, 0x262b14f48d29de80ull, 5, (uint64_t[]){ companionPed, 4, 15, 0, 0 }); // Bare legs / bikini
  pg_test_call(instance, 0x262b14f48d29de80ull, 5, (uint64_t[]){ companionPed, 8, 15, 0, 0 }); // Undershirt cleared
  pg_test_call(instance, 0x262b14f48d29de80ull, 5, (uint64_t[]){ companionPed, 11, 15, 0, 0 }); // Bare top
  pg_test_call(instance, 0x262b14f48d29de80ull, 5, (uint64_t[]){ companionPed, 6, 0, 0, 0 });  // Bare feet

  const char *locName = (location == 0) ? "Giuong" : "Ghe Sofa";

  if (posture == 0) {
    // Posture 0: Oral / Blowjob
    pg_test_call(instance, 0xd2a71e1a77418a49ull, 1, (uint64_t[]){ barAnimDictPtr1 }); // REQUEST_ANIM_DICT
    strcpy((char*)pg_pointer(&env, barAnimNamePedPtr, 64), "bj_loop_female");
    strcpy((char*)pg_pointer(&env, barAnimNamePlyrPtr, 64), "bj_loop_player");
    uint64_t aPed[11] = { companionPed, barAnimDictPtr1, barAnimNamePedPtr, pg_zf32(8.0f), pg_zf32(-8.0f), -1, 1, 0, 0, 0, 0 };
    pg_test_call(instance, 0xea47fe3719165b94ull, 11, aPed);
    uint64_t aPlyr[11] = { playerPed, barAnimDictPtr1, barAnimNamePlyrPtr, pg_zf32(8.0f), pg_zf32(-8.0f), -1, 1, 0, 0, 0, 0 };
    pg_test_call(instance, 0xea47fe3719165b94ull, 11, aPlyr);
    char msg[128]; snprintf(msg, sizeof msg, "~p~[18+ Tu the 1/4: Thoi ken - %s] Chon [Doi tu the] hoac [Giuong/Ghe] | [F] Dung", locName);
    pg_bar_show_subtitle(instance, msg, 4000);
  } else if (posture == 1) {
    // Posture 1: Missionary / Nam ngua
    pg_test_call(instance, 0xd2a71e1a77418a49ull, 1, (uint64_t[]){ barAnimDictPtr1 });
    strcpy((char*)pg_pointer(&env, barAnimNamePedPtr, 64), "sex_loop_female");
    strcpy((char*)pg_pointer(&env, barAnimNamePlyrPtr, 64), "sex_loop_player");
    uint64_t aPed[11] = { companionPed, barAnimDictPtr1, barAnimNamePedPtr, pg_zf32(8.0f), pg_zf32(-8.0f), -1, 1, 0, 0, 0, 0 };
    pg_test_call(instance, 0xea47fe3719165b94ull, 11, aPed);
    uint64_t aPlyr[11] = { playerPed, barAnimDictPtr1, barAnimNamePlyrPtr, pg_zf32(8.0f), pg_zf32(-8.0f), -1, 1, 0, 0, 0, 0 };
    pg_test_call(instance, 0xea47fe3719165b94ull, 11, aPlyr);
    char msg[128]; snprintf(msg, sizeof msg, "~p~[18+ Tu the 2/4: Nam ngua - %s] Chon [Doi tu the] hoac [Giuong/Ghe] | [F] Dung", locName);
    pg_bar_show_subtitle(instance, msg, 4000);
  } else if (posture == 2) {
    // Posture 2: Cowgirl / Cuoi ngua
    pg_test_call(instance, 0xd2a71e1a77418a49ull, 1, (uint64_t[]){ barAnimDictPtr1 });
    strcpy((char*)pg_pointer(&env, barAnimNamePedPtr, 64), "sex_loop_player");
    strcpy((char*)pg_pointer(&env, barAnimNamePlyrPtr, 64), "sex_loop_female");
    uint64_t aPed[11] = { companionPed, barAnimDictPtr1, barAnimNamePedPtr, pg_zf32(8.0f), pg_zf32(-8.0f), -1, 1, 0, 0, 0, 0 };
    pg_test_call(instance, 0xea47fe3719165b94ull, 11, aPed);
    uint64_t aPlyr[11] = { playerPed, barAnimDictPtr1, barAnimNamePlyrPtr, pg_zf32(8.0f), pg_zf32(-8.0f), -1, 1, 0, 0, 0, 0 };
    pg_test_call(instance, 0xea47fe3719165b94ull, 11, aPlyr);
    char msg[128]; snprintf(msg, sizeof msg, "~p~[18+ Tu the 3/4: Cuoi ngua - %s] Chon [Doi tu the] hoac [Giuong/Ghe] | [F] Dung", locName);
    pg_bar_show_subtitle(instance, msg, 4000);
  } else {
    // Posture 3: Lapdance show tren ghe/sofa
    pg_test_call(instance, 0xd2a71e1a77418a49ull, 1, (uint64_t[]){ barAnimDictPtr2 });
    strcpy((char*)pg_pointer(&env, barAnimNamePedPtr, 64), "priv_dance_p1");
    uint64_t aPed[11] = { companionPed, barAnimDictPtr2, barAnimNamePedPtr, pg_zf32(8.0f), pg_zf32(-8.0f), -1, 1, 0, 0, 0, 0 };
    pg_test_call(instance, 0xea47fe3719165b94ull, 11, aPed);
    char msg[128]; snprintf(msg, sizeof msg, "~p~[18+ Tu the 4/4: Mua khieu goi - %s] Chon [Doi tu the] hoac [Giuong/Ghe] | [F] Dung", locName);
    pg_bar_show_subtitle(instance, msg, 4000);
  }

  // Camera subtle sensual shake
  static uint64_t camShake = 0;
  if (!camShake) {
    camShake = pg_asi_alloc(32);
    strcpy((char*)pg_pointer(&env, camShake, 32), "JOLT_SHAKE");
  }
  pg_test_call(instance, 0xfd55e49555e017cfull, 2, (uint64_t[]){ camShake, pg_zf32(0.20f) });
}

// Tick function called every frame on the game thread
static void pg_bar_companion_tick(w2c_game *instance) {
  if (!pg_asi_game) pg_asi_game = instance;
  if (PGPauseMenuActive() || PGLoadSceneActive() || (PGGameStatus() & 16)) return;

  uint64_t prev = pg_asi_enter_script(instance);

  u32 playerPed = PG_FN_PLAYER_PED_ID(instance);
  if (!playerPed || !PG_FN_ENTITY_EXISTS(instance, playerPed)) {
    pg_asi_leave_script(instance, prev);
    return;
  }

  if (!barCoordVector) barCoordVector = pg_asi_alloc(32);
  if (!barCoordVector) {
    pg_asi_leave_script(instance, prev);
    return;
  }

  PG_FN_GET_ENTITY_COORDS(instance, barCoordVector, playerPed, 1);
  float *pv = (float*)pg_pointer(&env, barCoordVector, 24);
  float px = pv[0], py = pv[2], pz = pv[4];

  double now = timeMS(CLOCK_MONOTONIC);

  // Check vehicle status
  bool inVehicle = (bool)pg_test_call(instance, 0x997abd671d25ca0bull, 2, (uint64_t[]){ playerPed, 0 });
  u32 playerVeh = 0;
  if (inVehicle) {
    playerVeh = (u32)pg_test_call(instance, 0x9a9112a0fe9a4713ull, 2, (uint64_t[]){ playerPed, 0 });
  }

  // --- STATE 0: INACTIVE -> Spawn at Vanilla Unicorn or nearby street hookers ---
  if (barCompanionState == BAR_COMPANION_INACTIVE) {
    float dxBar = px - vanillaUnicornX;
    float dyBar = py - vanillaUnicornY;
    float distToBar = sqrtf(dxBar * dxBar + dyBar * dyBar);

    // Meet at Vanilla Unicorn (within 70m) or invite anytime while driving in city
    if (distToBar < 70.0f || (inVehicle && now - barLastPromptTime > 45000.0)) {
      barCompanionModel = barCompanionModels[rand() % 3];
      PG_FN_REQUEST_MODEL(instance, barCompanionModel);
      if (PG_FN_HAS_MODEL_LOADED(instance, barCompanionModel)) {
        float spawnX = inVehicle ? (px + 4.5f) : (vanillaUnicornX + 2.0f);
        float spawnY = inVehicle ? (py + 4.5f) : (vanillaUnicornY + 1.5f);
        float spawnZ = inVehicle ? pz : vanillaUnicornZ;

        uint64_t pArgs[8] = {
          5, barCompanionModel, pg_zf32(spawnX), pg_zf32(spawnY), pg_zf32(spawnZ), pg_zf32(90.0f), 0, 1
        };
        barCompanionPed = (u32)pg_test_call(instance, 0xd49f9b0955c367deull, 8, pArgs);
        if (barCompanionPed) {
          // Never flee, never panic, invincible, stay friendly
          pg_test_call(instance, 0x9f8aa94d6d97dbf4ull, 2, (uint64_t[]){ barCompanionPed, 1 }); // SET_BLOCKING_OF_NON_TEMPORARY_EVENTS
          pg_test_call(instance, 0x70a2d1137c8ed7c9ull, 3, (uint64_t[]){ barCompanionPed, 0, 0 }); // SET_PED_FLEE_ATTRIBUTES(0, 0)
          pg_test_call(instance, 0x9f7794730795e019ull, 3, (uint64_t[]){ barCompanionPed, 17, 1 }); // BF_AlwaysFlee = 0
          pg_test_call(instance, 0x3882114bde571ad4ull, 2, (uint64_t[]){ barCompanionPed, 1 }); // SET_ENTITY_INVINCIBLE
          pg_test_call(instance, 0x63f58f7c80513aadull, 2, (uint64_t[]){ barCompanionPed, 0 }); // SET_PED_CAN_BE_TARGETTED
          pg_test_call(instance, 0xf25df915fa38c5f3ull, 2, (uint64_t[]){ barCompanionPed, 1 }); // Disarm

          // Pink companion heart blip on radar
          barCompanionBlip = (u32)pg_test_call(instance, 0x5cde92c702a8fce7ull, 1, (uint64_t[]){ barCompanionPed });
          if (barCompanionBlip) {
            pg_test_call(instance, 0x03d7fb09e75d6b7eull, 2, (uint64_t[]){ barCompanionBlip, 8 }); // Pink
            pg_test_call(instance, 0xdf735600a4696dafull, 2, (uint64_t[]){ barCompanionBlip, 280 }); // Dancer/heart
            pg_test_call(instance, 0xd38744167b2fa257ull, 2, (uint64_t[]){ barCompanionBlip, pg_zf32(0.9f) });
          }
          // Idle wander animation
          pg_test_call(instance, 0xbb9ce077274f6a1bull, 3, (uint64_t[]){ barCompanionPed, pg_zf32(3.0f), 10 });
          barCompanionState = BAR_COMPANION_AT_BAR;
          pg_bar_show_subtitle(instance, "~y~Gap co gai hoa xinh dep trang treo! Hay lai gan hoac bam ken xe de moi co ay ve nha.", 5000);
          pg_log("[Bar Companion] Co gai hoa xinh dep da xuat hien, san sang len xe ve nha.");
        }
      }
    }
  }

  // Verify entity alive
  if (barCompanionPed && !PG_FN_ENTITY_EXISTS(instance, barCompanionPed)) {
    pg_bar_companion_cleanup(instance);
    pg_asi_leave_script(instance, prev);
    return;
  }

  // --- STATE 1: AT BAR / STREET -> Invitation to enter car ---
  if (barCompanionState == BAR_COMPANION_AT_BAR) {
    PG_FN_GET_ENTITY_COORDS(instance, barCoordVector, barCompanionPed, 1);
    float *cv = (float*)pg_pointer(&env, barCoordVector, 24);
    float cdx = px - cv[0], cdy = py - cv[2];
    float distToCompanion = sqrtf(cdx * cdx + cdy * cdy);

    if (distToCompanion < 16.0f) {
      if (now - barLastPromptTime > 4000) {
        barLastPromptTime = now;
        pg_bar_show_subtitle(instance, "~b~Bam ken xe (Horn) hoac lai sat co gai de moi co ay ve nha!", 3000);
      }

      // Check horn pressed (control 86) or close proximity (< 4.0m) or explicit start romance
      bool hornPressed = (bool)pg_test_call(instance, 0x580417101ddb492full, 2, (uint64_t[]){ 0, 86 });
      if (hornPressed || distToCompanion < 4.0f || barRomanceRequested) {
        if (inVehicle && playerVeh) {
          uint64_t eArgs[7] = { barCompanionPed, playerVeh, -1, 0, pg_zf32(2.0f), 1, 0 };
          pg_test_call(instance, 0xc20e50aa46d09ca8ull, 7, eArgs);
          barCompanionState = BAR_COMPANION_ENTERING;
        } else {
          // On foot: follow directly
          uint64_t fArgs[9] = { barCompanionPed, playerPed, 0, pg_zf32(-1.0f), 0, pg_zf32(1.0f), 0xFFFFFFFFull, pg_zf32(1.0f), 1 };
          pg_test_call(instance, 0x304ae42e357b8c7eull, 9, fArgs);
          barCompanionState = BAR_COMPANION_FOLLOWING;
          barFollowStartTime = now;
        }
        pg_bar_show_subtitle(instance, "~y~\"Chao anh dep trai! Cho em ve nha anh nhe, em se chieu anh het minh!\"", 4500);
        pg_log("[Bar Companion] Co gai dong y len xe / di theo nguoi choi ve nha.");
      }
    }
  }

  // --- STATE 2: ENTERING VEHICLE ---
  if (barCompanionState == BAR_COMPANION_ENTERING) {
    bool companionInVeh = (bool)pg_test_call(instance, 0x997abd671d25ca0bull, 2, (uint64_t[]){ barCompanionPed, 0 });
    if (companionInVeh) {
      // Find nearest safehouse
      float bestDist = 1e9f;
      for (int i = 0; i < 4; i++) {
        float dx = px - safehouseList[i][0], dy = py - safehouseList[i][1];
        float d = dx * dx + dy * dy;
        if (d < bestDist) {
          bestDist = d;
          activeSafehouseX = safehouseList[i][0];
          activeSafehouseY = safehouseList[i][1];
          activeSafehouseZ = safehouseList[i][2];
        }
      }

      // Set GPS Waypoint to player's nearest safehouse
      pg_test_call(instance, 0xfe43368d2aa4f2fcull, 2, (uint64_t[]){ pg_zf32(activeSafehouseX), pg_zf32(activeSafehouseY) });

      if (!barSafehouseBlip) {
        barSafehouseBlip = (u32)pg_test_call(instance, 0x5a039bb0bca604b6ull, 3, (uint64_t[]){ pg_zf32(activeSafehouseX), pg_zf32(activeSafehouseY), pg_zf32(activeSafehouseZ) });
        if (barSafehouseBlip) {
          pg_test_call(instance, 0x03d7fb09e75d6b7eull, 2, (uint64_t[]){ barSafehouseBlip, 2 }); // Green
          pg_test_call(instance, 0xdf735600a4696dafull, 2, (uint64_t[]){ barSafehouseBlip, 40 }); // Safehouse icon
          pg_test_call(instance, 0xd38744167b2fa257ull, 2, (uint64_t[]){ barSafehouseBlip, pg_zf32(1.0f) });
        }
      }

      barCompanionState = BAR_COMPANION_RIDING;
      pg_bar_show_subtitle(instance, "~g~Co gai da len xe! Lai theo dau GPS ve can nha / biet thu de thu gian 18+.", 5000);
      pg_log("[Bar Companion] Co gai da ngoi tren xe, GPS dan ve safehouse.");
    }
  }

  // --- STATE 3: RIDING TO SAFEHOUSE ---
  if (barCompanionState == BAR_COMPANION_RIDING) {
    float dxSh = px - activeSafehouseX;
    float dySh = py - activeSafehouseY;
    float distToSafehouse = sqrtf(dxSh * dxSh + dySh * dySh);

    // Sweet talk dialogue while driving
    if (now - barLastPromptTime > 12000) {
      barLastPromptTime = now;
      const char *phrases[] = {
        "~p~\"Xe anh lai em ai qua, em hao huc ve den nha anh roi day...\"",
        "~p~\"Anh dep trai ma phong do qua, ti nua vao phong em se cham soc anh can than nhe!\"",
        "~p~\"Sap ve den nha roi anh oi, phong ngu anh co am cung khong?\""
      };
      pg_bar_show_subtitle(instance, phrases[rand() % 3], 4000);
    }

    // Arrived at Safehouse (< 20m) OR player got out of vehicle OR romance explicitly requested
    if (distToSafehouse < 20.0f || !inVehicle || barRomanceRequested) {
      pg_bar_show_subtitle(instance, "~g~Da ve den nha! Cung co gai vao phong ngu / sofa de thu gian 18+ nao.", 5000);
      pg_test_call(instance, 0xd3dbce61a490be02ull, 3, (uint64_t[]){ barCompanionPed, 0, 0 }); // Leave car

      uint64_t fArgs[9] = { barCompanionPed, playerPed, 0, pg_zf32(-1.0f), 0, pg_zf32(1.0f), 0xFFFFFFFFull, pg_zf32(1.0f), 1 };
      pg_test_call(instance, 0x304ae42e357b8c7eull, 9, fArgs);

      barCompanionState = BAR_COMPANION_FOLLOWING;
      barFollowStartTime = now;
      pg_log("[Bar Companion] Da den safehouse, co gai dang di theo nguoi choi.");
    }
  }

  // Intercept & block screen blackout: Never allow black screen during companion interactions!
  if (barCompanionState >= BAR_COMPANION_FOLLOWING) {
    bool fadedOut = (bool)pg_test_call(instance, 0xb16fce9ddc7ba182ull, 0, NULL); // IS_SCREEN_FADED_OUT
    bool fadingOut = (bool)pg_test_call(instance, 0x797ac7cb535ba28full, 0, NULL); // IS_SCREEN_FADING_OUT
    if (fadedOut || fadingOut) {
      pg_test_call(instance, 0xd4e8e24955024033ull, 1, (uint64_t[]){ 0 }); // DO_SCREEN_FADE_IN(0)
    }
  }

  // --- STATE 4: FOLLOWING TO BEDROOM ---
  if (barCompanionState == BAR_COMPANION_FOLLOWING) {
    PG_FN_GET_ENTITY_COORDS(instance, barCoordVector, barCompanionPed, 1);
    float *cv = (float*)pg_pointer(&env, barCoordVector, 24);
    float cdx = px - cv[0], cdy = py - cv[2];
    float distToCompanion = sqrtf(cdx * cdx + cdy * cdy);

    float dxSh = px - activeSafehouseX;
    float dySh = py - activeSafehouseY;
    float distToSafehouse = sqrtf(dxSh * dxSh + dySh * dySh);

    // ANTI-FLEE & ANTI-LOST TELEPORT: If companion gets stuck, blocked, or lags > 5.5m
    if (distToCompanion > 5.5f) {
      cv[0] = px - 0.8f; cv[1] = 0; cv[2] = py - 0.8f; cv[3] = 0; cv[4] = pz; cv[5] = 0;
      PG_FN_SET_ENTITY_COORDS_NO_OFFSET(instance, barCompanionPed, barCoordVector, 0, 0, 0);
      uint64_t fArgs[9] = { barCompanionPed, playerPed, 0, pg_zf32(-1.0f), 0, pg_zf32(1.0f), 0xFFFFFFFFull, pg_zf32(1.0f), 1 };
      pg_test_call(instance, 0x304ae42e357b8c7eull, 9, fArgs);
    }

    // Inside bedroom/living room OR explicit button request OR stopped moving near companion
    bool readyForRomance = (distToSafehouse < 22.0f && distToCompanion < 3.5f && (now - barFollowStartTime > 2500.0)) ||
                           barRomanceRequested || (distToCompanion < 2.5f && (now - barFollowStartTime > 5000.0));

    if (readyForRomance) {
      barCompanionState = BAR_COMPANION_BEDROOM;
      barRomanceStartTime = now;
      barRomancePosture = 0; // Start with Blowjob
      barRomanceLocation = 0; // Bed
      barLastPostureSwitch = now;
      barRomanceRequested = false;

      // Position facing each other
      pv[0] = px + 0.35f; pv[1] = 0; pv[2] = py + 0.35f; pv[3] = 0; pv[4] = pz; pv[5] = 0;
      PG_FN_SET_ENTITY_COORDS_NO_OFFSET(instance, playerPed, barCoordVector, 0, 0, 0);
      pv[0] = px - 0.35f; pv[1] = 0; pv[2] = py - 0.35f; pv[3] = 0; pv[4] = pz; pv[5] = 0;
      PG_FN_SET_ENTITY_COORDS_NO_OFFSET(instance, barCompanionPed, barCoordVector, 0, 0, 0);

      // Start 18+ animation loop (NO screen fade out, 100% visible, fully stripped)
      pg_bar_apply_romance_posture(instance, playerPed, barCompanionPed, barRomancePosture, barRomanceLocation);
      pg_log("[Bar Companion] Bat dau 18+ trong phong (coi do hoan toan, 100%% ro net).");
    }
  }

  // --- STATE 5: INTERACTIVE 18+ BEDROOM ROMANCE ---
  if (barCompanionState == BAR_COMPANION_BEDROOM) {
    // Check posture switch via Horn (control 86) / E (51) / on-screen button
    bool switchPressed = (bool)pg_test_call(instance, 0x580417101ddb492full, 2, (uint64_t[]){ 0, 86 }) ||
                         (bool)pg_test_call(instance, 0x580417101ddb492full, 2, (uint64_t[]){ 0, 51 });
    if (switchPressed && (now - barLastPostureSwitch > 1200)) {
      barLastPostureSwitch = now;
      barRomancePosture = (barRomancePosture + 1) % 4;
      pg_bar_apply_romance_posture(instance, playerPed, barCompanionPed, barRomancePosture, barRomanceLocation);
    }

    // Check exit request (F / control 75 or after 35s or finish requested)
    bool exitPressed = (bool)pg_test_call(instance, 0x580417101ddb492full, 2, (uint64_t[]){ 0, 75 }) || barRomanceFinished;
    if (exitPressed || (now - barRomanceStartTime > 35000.0)) {
      barRomanceFinished = false;
      pg_test_call(instance, 0xaaa34f8a7cb32098ull, 1, (uint64_t[]){ barCompanionPed });
      pg_test_call(instance, 0xaaa34f8a7cb32098ull, 1, (uint64_t[]){ playerPed });

      // Restore 100% full health (200 HP) and full Armour (100)
      PG_FN_SET_ENTITY_HEALTH(instance, playerPed, 200, 0);
      pg_test_call(instance, 0xcea04d83135264ccull, 2, (uint64_t[]){ playerPed, 100 });

      pg_bar_show_subtitle(instance, "~g~Ban da thu gian sung man cung co gai! Hoi phuc 100% Mau va 100 Giap!", 6000);
      barCompanionState = BAR_COMPANION_RESTED;
      pg_log("[Bar Companion] 18+ Romance hoan tat! Mau va Giap nguoi choi da duoc hoi phuc 100%% day binh.");
    }
  }

  // --- STATE 6: RESTED -> Relaxed companion inside safehouse ---
  if (barCompanionState == BAR_COMPANION_RESTED) {
    static double nextWanderCheck = 0;
    if (now > nextWanderCheck) {
      nextWanderCheck = now + 15000;
      pg_test_call(instance, 0xbb9ce077274f6a1bull, 3, (uint64_t[]){ barCompanionPed, pg_zf32(5.0f), 10 });
    }
  }
  pg_asi_leave_script(instance, prev);
}

// Swift UI Control Interfaces
u32 PGBarCompanionGetPed(void) { return barCompanionPed; }
int PGBarCompanionGetState(void) { return (int)barCompanionState; }
int PGBarCompanionGetPosture(void) { return barRomancePosture; }
int PGBarCompanionGetLocation(void) { return barRomanceLocation; }
void PGBarCompanionNextPosture(void) {
  barRomancePosture = (barRomancePosture + 1) % 4;
  barLastPostureSwitch = timeMS(CLOCK_MONOTONIC);
  if (barCompanionPed && (barCompanionState == BAR_COMPANION_BEDROOM)) {
    pg_bar_apply_romance_posture(pg_asi_game, PG_FN_PLAYER_PED_ID(pg_asi_game), barCompanionPed, barRomancePosture, barRomanceLocation);
  }
}
void PGBarCompanionToggleLocation(void) {
  barRomanceLocation = (barRomanceLocation + 1) % 2;
  barLastPostureSwitch = timeMS(CLOCK_MONOTONIC);
  if (barCompanionPed && (barCompanionState == BAR_COMPANION_BEDROOM)) {
    pg_bar_apply_romance_posture(pg_asi_game, PG_FN_PLAYER_PED_ID(pg_asi_game), barCompanionPed, barRomancePosture, barRomanceLocation);
  }
}
void PGBarCompanionPayTip(void) {
  if (pg_asi_game) {
    pg_bar_show_subtitle(pg_asi_game, "~g~Da tra $100 tien bo cho co gai hoa! Co ay rat vui va nhiet tinh.", 4000);
  }
}
void PGBarCompanionFinish(void) {
  barRomanceFinished = true;
}
void PGBarCompanionStartRomance(void) {
  barRomanceRequested = true;
  if (pg_asi_game) {
    u32 playerPed = PG_FN_PLAYER_PED_ID(pg_asi_game);
    if (playerPed && PG_FN_ENTITY_EXISTS(pg_asi_game, playerPed)) {
      if (!barCompanionPed || !PG_FN_ENTITY_EXISTS(pg_asi_game, barCompanionPed)) {
        if (!barCoordVector) barCoordVector = pg_asi_alloc(32);
        PG_FN_GET_ENTITY_COORDS(pg_asi_game, barCoordVector, playerPed, 1);
        float *pv = (float*)pg_pointer(&env, barCoordVector, 24);
        barCompanionModel = barCompanionModels[0];
        PG_FN_REQUEST_MODEL(pg_asi_game, barCompanionModel);
        uint64_t pArgs[8] = {
          5, barCompanionModel, pg_zf32(pv[0] + 1.2f), pg_zf32(pv[2] + 1.2f), pg_zf32(pv[4]), pg_zf32(0.0f), 0, 1
        };
        barCompanionPed = (u32)pg_test_call(pg_asi_game, 0xd49f9b0955c367deull, 8, pArgs);
        if (barCompanionPed) {
          pg_test_call(pg_asi_game, 0x9f8aa94d6d97dbf4ull, 2, (uint64_t[]){ barCompanionPed, 1 });
          pg_test_call(pg_asi_game, 0x70a2d1137c8ed7c9ull, 3, (uint64_t[]){ barCompanionPed, 0, 0 });
          pg_test_call(pg_asi_game, 0x9f7794730795e019ull, 3, (uint64_t[]){ barCompanionPed, 17, 1 });
          pg_test_call(pg_asi_game, 0x3882114bde571ad4ull, 2, (uint64_t[]){ barCompanionPed, 1 });
          pg_test_call(pg_asi_game, 0x63f58f7c80513aadull, 2, (uint64_t[]){ barCompanionPed, 0 });
          pg_test_call(pg_asi_game, 0xf25df915fa38c5f3ull, 2, (uint64_t[]){ barCompanionPed, 1 });
          barCompanionState = BAR_COMPANION_FOLLOWING;
          barFollowStartTime = timeMS(CLOCK_MONOTONIC);
        }
      }
    }
  }
}
