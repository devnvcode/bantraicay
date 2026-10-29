// ============================================================================
// PlayGTA5Native - NORMAL MODE COMPANION & 18+ INTERACTIVE ROMANCE SYSTEM
// ============================================================================
// Features in Normal Story Mode (when Zombie Mode is inactive):
// 1. Fair-skinned, attractive female companion (s_f_y_stripper_01, s_f_y_hooker_01)
// 2. Meet at Vanilla Unicorn / Bars OR honk car horn near girls on street ($100 tip)
// 3. Companion boards passenger seat and GPS waypoint sets to player safehouse
// 4. In-car dialogue & follow to safehouse bedroom / couch
// 5. Interactive 18+ Bedroom Romance:
//    - NO black screen fade! 100% visible on-screen action
//    - Full undress: removes clothes to bare/bikini components
//    - Synchronized 18+ anims: Blowjob, Missionary/Cowgirl intimate loop, Lapdance
//    - Interactive posture switching with Horn / E / Mod Keys
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
static double barRomanceStartTime = 0;
static int barRomancePosture = 0;     // 0: Blowjob, 1: Missionary/Cowgirl, 2: Lapdance show
static double barLastPostureSwitch = 0;
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

// Franklin's 3671 Whispymound Drive Mansion (Vinewood Hills)
static const float safehouseX = 7.0f;
static const float safehouseY = 525.0f;
static const float safehouseZ = 170.5f;

// Model: s_f_y_stripper_01 (0x61c6b12a - Chastity, fair skin blonde)
static const uint32_t barCompanionModel = 0x61C6B12A;

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
}

// Apply 18+ Synchronized animation based on posture index
static void pg_bar_apply_romance_posture(w2c_game *instance, u32 playerPed, u32 companionPed, int posture) {
  if (!barAnimDictPtr1) {
    barAnimDictPtr1 = pg_asi_alloc(64);
    barAnimDictPtr2 = pg_asi_alloc(64);
    barAnimNamePedPtr = pg_asi_alloc(64);
    barAnimNamePlyrPtr = pg_asi_alloc(64);
    strcpy((char*)pg_pointer(&env, barAnimDictPtr1, 64), "mini@prostitutes@sexnorm_veh");
    strcpy((char*)pg_pointer(&env, barAnimDictPtr2, 64), "mini@strip_club@lap_dance");
  }

  // Clear previous tasks
  pg_test_call(instance, 0xe1ef3c1216ee2859ull, 1, (uint64_t[]){ companionPed }); // CLEAR_PED_TASKS_IMMEDIATELY
  pg_test_call(instance, 0xe1ef3c1216ee2859ull, 1, (uint64_t[]){ playerPed });

  // Undress companion completely to bikini/bare skin
  // 0x262b14f48d29de80: SET_PED_COMPONENT_VARIATION(ped, componentId, drawableId, textureId, paletteId)
  pg_test_call(instance, 0x262b14f48d29de80ull, 5, (uint64_t[]){ companionPed, 3, 15, 0, 0 }); // Bare upper torso
  pg_test_call(instance, 0x262b14f48d29de80ull, 5, (uint64_t[]){ companionPed, 4, 15, 0, 0 }); // Bare legs / bikini
  pg_test_call(instance, 0x262b14f48d29de80ull, 5, (uint64_t[]){ companionPed, 8, 15, 0, 0 }); // Undershirt
  pg_test_call(instance, 0x262b14f48d29de80ull, 5, (uint64_t[]){ companionPed, 11, 15, 0, 0 }); // Bare top
  pg_test_call(instance, 0x262b14f48d29de80ull, 5, (uint64_t[]){ companionPed, 6, 0, 0, 0 });  // Bare feet

  if (posture == 0) {
    // Posture 0: Oral / Blowjob
    pg_test_call(instance, 0xd2a71e1a77418a49ull, 1, (uint64_t[]){ barAnimDictPtr1 }); // REQUEST_ANIM_DICT
    strcpy((char*)pg_pointer(&env, barAnimNamePedPtr, 64), "bj_loop_female");
    strcpy((char*)pg_pointer(&env, barAnimNamePlyrPtr, 64), "bj_loop_player");
    // TASK_PLAY_ANIM companion
    uint64_t aPed[11] = { companionPed, barAnimDictPtr1, barAnimNamePedPtr, pg_zf32(8.0f), pg_zf32(-8.0f), -1, 1, 0, 0, 0, 0 };
    pg_test_call(instance, 0xea47fe3719165b94ull, 11, aPed);
    // TASK_PLAY_ANIM player
    uint64_t aPlyr[11] = { playerPed, barAnimDictPtr1, barAnimNamePlyrPtr, pg_zf32(8.0f), pg_zf32(-8.0f), -1, 1, 0, 0, 0, 0 };
    pg_test_call(instance, 0xea47fe3719165b94ull, 11, aPlyr);
    pg_bar_show_subtitle(instance, "~p~[18+ Tu the 1/3: Thoi ken] Nhan [Ken xe / E / Mod] de doi tu the | [F] Dung", 4000);
  } else if (posture == 1) {
    // Posture 1: Missionary / Cowgirl Intimate loop
    pg_test_call(instance, 0xd2a71e1a77418a49ull, 1, (uint64_t[]){ barAnimDictPtr1 });
    strcpy((char*)pg_pointer(&env, barAnimNamePedPtr, 64), "sex_loop_female");
    strcpy((char*)pg_pointer(&env, barAnimNamePlyrPtr, 64), "sex_loop_player");
    uint64_t aPed[11] = { companionPed, barAnimDictPtr1, barAnimNamePedPtr, pg_zf32(8.0f), pg_zf32(-8.0f), -1, 1, 0, 0, 0, 0 };
    pg_test_call(instance, 0xea47fe3719165b94ull, 11, aPed);
    uint64_t aPlyr[11] = { playerPed, barAnimDictPtr1, barAnimNamePlyrPtr, pg_zf32(8.0f), pg_zf32(-8.0f), -1, 1, 0, 0, 0, 0 };
    pg_test_call(instance, 0xea47fe3719165b94ull, 11, aPlyr);
    pg_bar_show_subtitle(instance, "~p~[18+ Tu the 2/3: Cuoi ngua / Nam sap] Nhan [Ken xe / E / Mod] de doi tu the | [F] Dung", 4000);
  } else {
    // Posture 2: Sensual Lapdance bedroom show
    pg_test_call(instance, 0xd2a71e1a77418a49ull, 1, (uint64_t[]){ barAnimDictPtr2 });
    strcpy((char*)pg_pointer(&env, barAnimNamePedPtr, 64), "priv_dance_p1");
    uint64_t aPed[11] = { companionPed, barAnimDictPtr2, barAnimNamePedPtr, pg_zf32(8.0f), pg_zf32(-8.0f), -1, 1, 0, 0, 0, 0 };
    pg_test_call(instance, 0xea47fe3719165b94ull, 11, aPed);
    pg_bar_show_subtitle(instance, "~p~[18+ Tu the 3/3: Mua quyen ru coi do] Nhan [Ken xe / E / Mod] de doi tu the | [F] Dung", 4000);
  }

  // Camera subtle sensual shake
  static uint64_t camShake = 0;
  if (!camShake) {
    camShake = pg_asi_alloc(32);
    strcpy((char*)pg_pointer(&env, camShake, 32), "JOLT_SHAKE");
  }
  pg_test_call(instance, 0x68edda809e2657b1ull, 2, (uint64_t[]){ camShake, pg_zf32(0.20f) });
}

// Tick function called every frame in Story Mode
static void pg_bar_companion_tick(w2c_game *instance) {
  // Only active when Zombie Mode is OFF
  if (atomic_load(&zombieActive)) {
    if (barCompanionState != BAR_COMPANION_INACTIVE) {
      pg_bar_companion_cleanup(instance);
    }
    return;
  }

  if (PGPauseMenuActive() || PGLoadSceneActive() || (PGGameStatus() & 16)) return;

  u32 playerPed = PG_FN_PLAYER_PED_ID(instance);
  if (!playerPed || !PG_FN_ENTITY_EXISTS(instance, playerPed)) return;

  if (!barCoordVector) barCoordVector = pg_asi_alloc(32);
  if (!barCoordVector) return;

  PG_FN_GET_ENTITY_COORDS(instance, barCoordVector, playerPed, 1);
  float *pv = (float*)pg_pointer(&env, barCoordVector, 24);
  float px = pv[0], py = pv[2], pz = pv[4];

  double now = timeMS(CLOCK_MONOTONIC);

  // 0x997abd671d25ca0b: IS_PED_IN_ANY_VEHICLE
  bool inVehicle = (bool)pg_test_call(instance, 0x997abd671d25ca0bull, 2, (uint64_t[]){ playerPed, 0 });
  u32 playerVeh = 0;
  if (inVehicle) {
    // 0x9a9112a0fe9a4713: GET_VEHICLE_PED_IS_IN
    playerVeh = (u32)pg_test_call(instance, 0x9a9112a0fe9a4713ull, 2, (uint64_t[]){ playerPed, 0 });
  }

  // --- STATE 0: INACTIVE -> Spawn at Vanilla Unicorn or nearby street hookers ---
  if (barCompanionState == BAR_COMPANION_INACTIVE) {
    float dxBar = px - vanillaUnicornX;
    float dyBar = py - vanillaUnicornY;
    float distToBar = sqrtf(dxBar * dxBar + dyBar * dyBar);

    // Meet at Vanilla Unicorn (within 60m) or invite anytime while driving in city
    if (distToBar < 60.0f || (inVehicle && now - barLastPromptTime > 60000.0)) {
      PG_FN_REQUEST_MODEL(instance, barCompanionModel);
      if (PG_FN_HAS_MODEL_LOADED(instance, barCompanionModel)) {
        float spawnX = inVehicle ? (px + 4.0f) : (vanillaUnicornX + 2.0f);
        float spawnY = inVehicle ? (py + 4.0f) : (vanillaUnicornY + 1.5f);
        float spawnZ = inVehicle ? pz : vanillaUnicornZ;

        uint64_t pArgs[8] = {
          5, barCompanionModel, pg_zf32(spawnX), pg_zf32(spawnY), pg_zf32(spawnZ), pg_zf32(90.0f), 0, 1
        };
        // 0xd49f9b0955c367de: CREATE_PED
        barCompanionPed = (u32)pg_test_call(instance, 0xd49f9b0955c367deull, 8, pArgs);
        if (barCompanionPed) {
          // Pink companion blip
          // 0x5cde92c702a8fce7: ADD_BLIP_FOR_ENTITY
          barCompanionBlip = (u32)pg_test_call(instance, 0x5cde92c702a8fce7ull, 1, (uint64_t[]){ barCompanionPed });
          if (barCompanionBlip) {
            pg_test_call(instance, 0x03d7fb09e75d6b7eull, 2, (uint64_t[]){ barCompanionBlip, 8 }); // Pink
            pg_test_call(instance, 0xdf735600a4696dafull, 2, (uint64_t[]){ barCompanionBlip, 280 }); // Dancer/heart
            pg_test_call(instance, 0xd38744167b2fa257ull, 2, (uint64_t[]){ barCompanionBlip, pg_zf32(0.9f) });
          }
          // Idle wander animation
          // 0xbb9ce077274f6a1b: TASK_WANDER_STANDARD
          pg_test_call(instance, 0xbb9ce077274f6a1bull, 3, (uint64_t[]){ barCompanionPed, pg_zf32(3.0f), 10 });
          barCompanionState = BAR_COMPANION_AT_BAR;
          pg_bar_show_subtitle(instance, "~y~Gap co gai xinh dep! Hay lai gan hoac bam ken xe de moi co ay len xe.", 4000);
          pg_log("[Bar Companion] Co gai xinh dep da xuat hien, san sang len xe ve nha.");
        }
      }
    }
  }

  // Verify entity alive
  if (barCompanionPed && !PG_FN_ENTITY_EXISTS(instance, barCompanionPed)) {
    pg_bar_companion_cleanup(instance);
    return;
  }

  // --- STATE 1: AT BAR / STREET -> Invitation to enter car ---
  if (barCompanionState == BAR_COMPANION_AT_BAR) {
    PG_FN_GET_ENTITY_COORDS(instance, barCoordVector, barCompanionPed, 1);
    float *cv = (float*)pg_pointer(&env, barCoordVector, 24);
    float cdx = px - cv[0], cdy = py - cv[2];
    float distToCompanion = sqrtf(cdx * cdx + cdy * cdy);

    if (distToCompanion < 12.0f && inVehicle && playerVeh) {
      if (now - barLastPromptTime > 4000) {
        barLastPromptTime = now;
        pg_bar_show_subtitle(instance, "~b~Bam ken xe (Horn) hoac lai sat co gai de moi co ay ve nha!", 3000);
      }

      // Check horn pressed (0x86 = INPUT_VEH_HORN, control 86) or close proximity (< 4.5m)
      bool hornPressed = (bool)pg_test_call(instance, 0x580417101d46564eull, 2, (uint64_t[]){ 0, 86 });
      if (hornPressed || distToCompanion < 4.5f) {
        // Invite companion to passenger seat
        // 0xc20e50aa46d09ca8: TASK_ENTER_VEHICLE(ped, vehicle, timeout, seat, speed, flag, p6)
        uint64_t eArgs[7] = {
          barCompanionPed, playerVeh, -1, 0, pg_zf32(2.0f), 1, 0 // Seat 0: Front passenger
        };
        pg_test_call(instance, 0xc20e50aa46d09ca8ull, 7, eArgs);

        barCompanionState = BAR_COMPANION_ENTERING;
        pg_bar_show_subtitle(instance, "~y~\"Chao anh dep trai! Cho em ve nha anh nhe, em se chieu anh het minh!\"", 4500);
        pg_log("[Bar Companion] Co gai dong y len xe nguoi choi.");
      }
    }
  }

  // --- STATE 2: ENTERING VEHICLE ---
  if (barCompanionState == BAR_COMPANION_ENTERING) {
    // 0x997abd671d25ca0b: IS_PED_IN_ANY_VEHICLE
    bool companionInVeh = (bool)pg_test_call(instance, 0x997abd671d25ca0bull, 2, (uint64_t[]){ barCompanionPed, 0 });
    if (companionInVeh) {
      // Set GPS Waypoint to player's Mansion Safehouse
      // 0x4f7d8a924251ba8b: SET_NEW_WAYPOINT(x, y)
      pg_test_call(instance, 0x4f7d8a924251ba8bull, 2, (uint64_t[]){ pg_zf32(safehouseX), pg_zf32(safehouseY) });

      // Add Mansion blip
      if (!barSafehouseBlip) {
        // 0x5a039bb0b1e8e3f6: ADD_BLIP_FOR_COORD
        barSafehouseBlip = (u32)pg_test_call(instance, 0x5a039bb0b1e8e3f6ull, 3, (uint64_t[]){ pg_zf32(safehouseX), pg_zf32(safehouseY), pg_zf32(safehouseZ) });
        if (barSafehouseBlip) {
          pg_test_call(instance, 0x03d7fb09e75d6b7eull, 2, (uint64_t[]){ barSafehouseBlip, 2 }); // Green
          pg_test_call(instance, 0xdf735600a4696dafull, 2, (uint64_t[]){ barSafehouseBlip, 40 }); // Safehouse icon
          pg_test_call(instance, 0xd38744167b2fa257ull, 2, (uint64_t[]){ barSafehouseBlip, pg_zf32(1.0f) });
        }
      }

      barCompanionState = BAR_COMPANION_RIDING;
      pg_bar_show_subtitle(instance, "~g~Co gai da len xe! Lai theo dau GPS ve can biet thu Mansion de thu gian.", 5000);
      pg_log("[Bar Companion] Co gai da ngoi tren xe, GPS dan duong ve safehouse.");
    }
  }

  // --- STATE 3: RIDING TO SAFEHOUSE ---
  if (barCompanionState == BAR_COMPANION_RIDING) {
    float dxSh = px - safehouseX;
    float dySh = py - safehouseY;
    float distToSafehouse = sqrtf(dxSh * dxSh + dySh * dySh);

    // Occasional sweet talk dialogue while driving
    if (now - barLastPromptTime > 12000) {
      barLastPromptTime = now;
      const char *phrases[] = {
        "~p~\"Xe anh lai em ai qua, em hao huc ve den nha anh roi day...\"",
        "~p~\"Anh dep trai ma phong do qua, ti nua vao phong em se cham soc anh can than nhe!\"",
        "~p~\"Sap ve den nha roi anh oi, phong ngu anh co am cung khong?\""
      };
      pg_bar_show_subtitle(instance, phrases[rand() % 3], 4000);
    }

    // Arrived at Safehouse (< 18m)
    if (distToSafehouse < 18.0f) {
      pg_bar_show_subtitle(instance, "~g~Da ve den nha! Xuong xe va cung co gai vao phong ngu thu gian 18+ nao.", 5000);
      // Leave vehicle
      // 0xd3dbce61a490be52: TASK_LEAVE_ANY_VEHICLE
      pg_test_call(instance, 0xd3dbce61a490be52ull, 3, (uint64_t[]){ barCompanionPed, 0, 0 });

      // Follow player closely into the house
      // 0x304ae42e357b8c7e: TASK_FOLLOW_TO_OFFSET_OF_ENTITY
      uint64_t fArgs[9] = {
        barCompanionPed, playerPed, 0, pg_zf32(-1.2f), 0, pg_zf32(1.2f), 0xFFFFFFFFull, pg_zf32(1.0f), 1
      };
      pg_test_call(instance, 0x304ae42e357b8c7eull, 9, fArgs);

      barCompanionState = BAR_COMPANION_FOLLOWING;
      pg_log("[Bar Companion] Da ve den safehouse, co gai dang di theo nguoi choi vao phong ngu.");
    }
  }

  // --- STATE 4: FOLLOWING TO BEDROOM ---
  if (barCompanionState == BAR_COMPANION_FOLLOWING) {
    PG_FN_GET_ENTITY_COORDS(instance, barCoordVector, barCompanionPed, 1);
    float *cv = (float*)pg_pointer(&env, barCoordVector, 24);
    float cdx = px - cv[0], cdy = py - cv[2];
    float distToCompanion = sqrtf(cdx * cdx + cdy * cdy);

    float dxSh = px - safehouseX;
    float dySh = py - safehouseY;
    float distToBed = sqrtf(dxSh * dxSh + dySh * dySh);

    // Inside safehouse bedroom (< 14m from bedroom bed) and player is close to companion
    if (distToBed < 14.0f && distToCompanion < 3.2f) {
      barCompanionState = BAR_COMPANION_BEDROOM;
      barRomanceStartTime = now;
      barRomancePosture = 0; // Start with posture 0 (Blowjob)
      barLastPostureSwitch = now;

      // Position player and companion together facing each other
      pv[0] = safehouseX + 0.35f; pv[1] = 0; pv[2] = safehouseY + 0.35f; pv[3] = 0; pv[4] = safehouseZ; pv[5] = 0;
      PG_FN_SET_ENTITY_COORDS_NO_OFFSET(instance, playerPed, barCoordVector, 0, 0, 0);
      pv[0] = safehouseX - 0.35f; pv[1] = 0; pv[2] = safehouseY - 0.35f; pv[3] = 0; pv[4] = safehouseZ; pv[5] = 0;
      PG_FN_SET_ENTITY_COORDS_NO_OFFSET(instance, barCompanionPed, barCoordVector, 0, 0, 0);

      // Start 18+ animation loop (NO screen fade out, 100% visible)
      pg_bar_apply_romance_posture(instance, playerPed, barCompanionPed, barRomancePosture);
      pg_log("[Bar Companion] Bat dau tu the 18+ trong phong ngu (coi do hoan toan, 100%% ro net).");
    }
  }

  // --- STATE 5: INTERACTIVE 18+ BEDROOM ROMANCE ---
  if (barCompanionState == BAR_COMPANION_BEDROOM) {
    // Check if player requested posture switch via Horn (key 0x45 / control 86) or D-pad right
    bool switchPressed = (bool)pg_test_call(instance, 0x580417101d46564eull, 2, (uint64_t[]){ 0, 86 }) ||
                         (bool)pg_test_call(instance, 0x580417101d46564eull, 2, (uint64_t[]){ 0, 51 });
    if (switchPressed && (now - barLastPostureSwitch > 1200)) {
      barLastPostureSwitch = now;
      barRomancePosture = (barRomancePosture + 1) % 3;
      pg_bar_apply_romance_posture(instance, playerPed, barCompanionPed, barRomancePosture);
      char posMsg[96];
      snprintf(posMsg, sizeof posMsg, "[Bar Companion] Nguoi choi da doi tu the 18+ sang tu the #%d.", barRomancePosture + 1);
      pg_log(posMsg);
    }

    // Check exit request (F / Enter vehicle control 75 or after 25s)
    bool exitPressed = (bool)pg_test_call(instance, 0x580417101d46564eull, 2, (uint64_t[]){ 0, 75 });
    if (exitPressed || (now - barRomanceStartTime > 25000.0)) {
      // Clear tasks
      pg_test_call(instance, 0xe1ef3c1216ee2859ull, 1, (uint64_t[]){ barCompanionPed });
      pg_test_call(instance, 0xe1ef3c1216ee2859ull, 1, (uint64_t[]){ playerPed });

      // Restore 100% full health (200 HP) and full Armour (100)
      PG_FN_SET_ENTITY_HEALTH(instance, playerPed, 200, 0);
      // 0xcea04d83135264cc: SET_PED_ARMOUR(playerPed, 100)
      pg_test_call(instance, 0xcea04d83135264ccull, 2, (uint64_t[]){ playerPed, 100 });

      pg_bar_show_subtitle(instance, "~g~Ban da thu gian sung man cung co gai! Hoi phuc 100% Mau va 100 Giap!", 6000);
      barCompanionState = BAR_COMPANION_RESTED;
      pg_log("[Bar Companion] 18+ Romance hoan tat! Mau va Giap nguoi choi da duoc hoi phuc 100%% day binh.");
    }
  }

  // --- STATE 6: RESTED -> Relaxed companion inside safehouse ---
  if (barCompanionState == BAR_COMPANION_RESTED) {
    // Companion wanders comfortably inside the mansion as a resident companion
    static double nextWanderCheck = 0;
    if (now > nextWanderCheck) {
      nextWanderCheck = now + 15000;
      pg_test_call(instance, 0xbb9ce077274f6a1bull, 3, (uint64_t[]){ barCompanionPed, pg_zf32(5.0f), 10 });
    }
  }
}
