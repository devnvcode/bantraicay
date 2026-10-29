// ============================================================================
// PlayGTA5Native - NORMAL MODE BAR COMPANION & ROMANCE INTERACTION
// ============================================================================
// Features in Normal Story Mode (when Zombie Mode is inactive):
// 1. Fair-skinned, attractive female companion (s_f_y_stripper_01 / s_f_y_stripper_02)
// 2. Meet at Vanilla Unicorn / Bars or call companion to vehicle ($100 tip)
// 3. Companion boards passenger seat and GPS waypoint sets to player safehouse
// 4. In-car dialogue & follow to safehouse bedroom
// 5. Bedroom intimate romance sequence: undress to bikini/underwear, romantic anims,
//    cinematic screen fade, camera shake & 100% full health + armour replenishment!
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
  BAR_COMPANION_BEDROOM = 5,      // Romance sequence in bedroom
  BAR_COMPANION_RESTED = 6        // Complete, health/armour restored
} BarCompanionState;

static BarCompanionState barCompanionState = BAR_COMPANION_INACTIVE;
static u32 barCompanionPed = 0;
static u32 barCompanionBlip = 0;
static u32 barSafehouseBlip = 0;
static double barLastPromptTime = 0;
static double barRomanceStartTime = 0;
static uint64_t barCoordVector = 0;
static uint64_t barSubtitleStrBuf = 0;
static uint64_t barAnimDictPtr = 0;
static uint64_t barAnimNamePtr = 0;

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

  // --- STATE 0: INACTIVE -> Spawn at Vanilla Unicorn or nearby when invited ---
  if (barCompanionState == BAR_COMPANION_INACTIVE) {
    float dxBar = px - vanillaUnicornX;
    float dyBar = py - vanillaUnicornY;
    float distToBar = sqrtf(dxBar * dxBar + dyBar * dyBar);

    // Near Vanilla Unicorn (within 60m)
    if (distToBar < 60.0f) {
      PG_FN_REQUEST_MODEL(instance, barCompanionModel);
      if (PG_FN_HAS_MODEL_LOADED(instance, barCompanionModel)) {
        float spawnX = vanillaUnicornX + 2.0f;
        float spawnY = vanillaUnicornY + 1.5f;
        float spawnZ = vanillaUnicornZ;

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
          // Idle animation
          // 0xbb9ce077274f6a1b: TASK_WANDER_STANDARD
          pg_test_call(instance, 0xbb9ce077274f6a1bull, 3, (uint64_t[]){ barCompanionPed, pg_zf32(3.0f), 10 });
          barCompanionState = BAR_COMPANION_AT_BAR;
          pg_bar_show_subtitle(instance, "~y~Gap co gai xinh dep tai quan bar! Hay lai gan moi co ay len xe.", 4000);
          pg_log("[Bar Companion] Co gai xinh dep da xuat hien tai quan bar Vanilla Unicorn.");
        }
      }
    }
  }

  // Verify entity alive
  if (barCompanionPed && !PG_FN_ENTITY_EXISTS(instance, barCompanionPed)) {
    pg_bar_companion_cleanup(instance);
    return;
  }

  // --- STATE 1: AT BAR -> Invitation to enter car ---
  if (barCompanionState == BAR_COMPANION_AT_BAR) {
    PG_FN_GET_ENTITY_COORDS(instance, barCoordVector, barCompanionPed, 1);
    float *cv = (float*)pg_pointer(&env, barCoordVector, 24);
    float cdx = px - cv[0], cdy = py - cv[2];
    float distToCompanion = sqrtf(cdx * cdx + cdy * cdy);

    if (distToCompanion < 8.0f && inVehicle && playerVeh) {
      if (now - barLastPromptTime > 3000) {
        barLastPromptTime = now;
        pg_bar_show_subtitle(instance, "~g~Da tra $100 tip! Co gai dang len xe ve nha cung ban.", 4000);
      }
      // Enter front passenger seat (seat index 0)
      // 0xc20e50aa46d09ca8: TASK_ENTER_VEHICLE(ped, vehicle, timeout, seat, speed, flag, p6)
      uint64_t evArgs[7] = { barCompanionPed, playerVeh, 15000, 0, pg_zf32(1.5f), 1, 0 };
      pg_test_call(instance, 0xc20e50aa46d09ca8ull, 7, evArgs);

      // Set GPS Waypoint to Safehouse (Franklin's mansion)
      // 0xfe43368d2aa4f2fc: SET_NEW_WAYPOINT
      pg_test_call(instance, 0xfe43368d2aa4f2fcull, 2, (uint64_t[]){ pg_zf32(safehouseX), pg_zf32(safehouseY) });

      // Add green safehouse blip
      if (!barSafehouseBlip) {
        uint64_t bArgs[3] = { pg_zf32(safehouseX), pg_zf32(safehouseY), pg_zf32(safehouseZ) };
        barSafehouseBlip = (u32)pg_test_call(instance, 0x5a039bb0bca604b6ull, 3, bArgs);
        if (barSafehouseBlip) {
          pg_test_call(instance, 0xdf735600a4696dafull, 2, (uint64_t[]){ barSafehouseBlip, 40 }); // Safehouse icon
          pg_test_call(instance, 0x03d7fb09e75d6b7eull, 2, (uint64_t[]){ barSafehouseBlip, 2 }); // Green
        }
      }

      barCompanionState = BAR_COMPANION_ENTERING;
      pg_log("[Bar Companion] Co gai da nhan $100 va dang buoc len ghe phu xe.");
    }
  }

  // --- STATE 2: ENTERING -> In vehicle check ---
  if (barCompanionState == BAR_COMPANION_ENTERING) {
    // 0x826aa586edb9fef8: IS_PED_SITTING_IN_ANY_VEHICLE
    bool seated = (bool)pg_test_call(instance, 0x826aa586edb9fef8ull, 1, (uint64_t[]){ barCompanionPed });
    if (seated) {
      barCompanionState = BAR_COMPANION_RIDING;
      pg_bar_show_subtitle(instance, "~p~Co gai: \"Nha anh o dau the? Minh cung ve do nhe anh!\"", 4500);
      pg_log("[Bar Companion] Co gai da yen vi tren xe, bat dau hanh trinh ve nha.");
    }
  }

  // --- STATE 3: RIDING TO SAFEHOUSE ---
  if (barCompanionState == BAR_COMPANION_RIDING) {
    float dxSh = px - safehouseX;
    float dySh = py - safehouseY;
    float distToSafehouse = sqrtf(dxSh * dxSh + dySh * dySh);

    // Arrived at Safehouse (< 28m)
    if (distToSafehouse < 28.0f) {
      pg_bar_show_subtitle(instance, "~g~Da ve den nha! Xuong xe va cung co gai vao phong ngu thu gian nao.", 5000);
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

    // When inside safehouse bedroom (< 12m from bedroom bed) and player is close to companion
    if (distToBed < 14.0f && distToCompanion < 3.0f) {
      barCompanionState = BAR_COMPANION_BEDROOM;
      barRomanceStartTime = now;

      // 1. Undress to bikini/underwear using native component variations
      // 0x262b14f48d29de80: SET_PED_COMPONENT_VARIATION(ped, componentId, drawableId, textureId, paletteId)
      pg_test_call(instance, 0x262b14f48d29de80ull, 5, (uint64_t[]){ barCompanionPed, 3, 0, 0, 0 }); // Torso / top bikini
      pg_test_call(instance, 0x262b14f48d29de80ull, 5, (uint64_t[]){ barCompanionPed, 4, 0, 0, 0 }); // Bottoms / bikini
      pg_test_call(instance, 0x262b14f48d29de80ull, 5, (uint64_t[]){ barCompanionPed, 8, 0, 0, 0 }); // Undershirt
      pg_test_call(instance, 0x262b14f48d29de80ull, 5, (uint64_t[]){ barCompanionPed, 6, 0, 0, 0 }); // Bare feet

      // 2. Play intimate romantic dance / resting animation
      if (!barAnimDictPtr) {
        barAnimDictPtr = pg_asi_alloc(64);
        barAnimNamePtr = pg_asi_alloc(64);
        strcpy((char*)pg_pointer(&env, barAnimDictPtr, 64), "mini@strip_club@lap_dance");
        strcpy((char*)pg_pointer(&env, barAnimNamePtr, 64), "priv_dance_p1");
      }
      pg_test_call(instance, 0xd2a71e1a77418a49ull, 1, (uint64_t[]){ barAnimDictPtr }); // REQUEST_ANIM_DICT
      uint64_t aArgs[11] = {
        barCompanionPed, barAnimDictPtr, barAnimNamePtr, pg_zf32(8.0f), pg_zf32(-8.0f), 5000, 1, 0, 0, 0, 0
      };
      pg_test_call(instance, 0xea47fe3719165b94ull, 11, aArgs); // TASK_PLAY_ANIM

      // 3. Cinematic gentle screen fade with romantic mood
      // 0x891b5b39ac6302af: DO_SCREEN_FADE_OUT(1200)
      pg_test_call(instance, 0x891b5b39ac6302afull, 1, (uint64_t[]){ 1200 });

      pg_bar_show_subtitle(instance, "~p~Co gai coi do va cung ban co nhung giay phut am ap tuyet voi...", 4000);
      pg_log("[Bar Companion] Dang trong phong ngu cung co gai: coi do bikini, animation lap dance & phai man hinh.");
    }
  }

  // --- STATE 5: BEDROOM ROMANCE -> Screen Fade In & Health/Armour Restoration ---
  if (barCompanionState == BAR_COMPANION_BEDROOM) {
    if (now - barRomanceStartTime > 3200) {
      // 0xd4e8e24955024033: DO_SCREEN_FADE_IN(1200)
      pg_test_call(instance, 0xd4e8e24955024033ull, 1, (uint64_t[]){ 1200 });

      // Restore 100% full health (200 HP) and full Armour (100)
      PG_FN_SET_ENTITY_HEALTH(instance, playerPed, 200, 0);
      // 0xcea04d83135264cc: SET_PED_ARMOUR(playerPed, 100)
      pg_test_call(instance, 0xcea04d83135264ccull, 2, (uint64_t[]){ playerPed, 100 });

      // Screen camera shake
      static uint64_t camShake = 0;
      if (!camShake) {
        camShake = pg_asi_alloc(32);
        strcpy((char*)pg_pointer(&env, camShake, 32), "JOLT_SHAKE");
      }
      pg_test_call(instance, 0x68edda809e2657b1ull, 2, (uint64_t[]){ camShake, pg_zf32(0.4f) });

      pg_bar_show_subtitle(instance, "~g~Ban da nghi ngoi sung man cung co gai! Hoi phuc 100% Mau va 100 Giap!", 6000);
      barCompanionState = BAR_COMPANION_RESTED;
      pg_log("[Bar Companion] Romance hoan tat! Mau va Giap nguoi choi da duoc hoi phuc 100%% day binh.");
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
