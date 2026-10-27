// Settings › Cheats › Spawn vehicle. Included by EngineHost.c. The list comes from the running
// engine's model registry, so it has exactly the vehicles this game build (and its DLC) has; no
// vehicle list ships with the app. Everything that touches the engine runs on the game thread during
// the "main" script's turn (pg_cheats_menu_tick, from pg_asi_script_tick): creating vehicles from
// the controller thread raced the engine walking its vehicle pool and crashed after some spawns.
// Layouts are pinned to 8b0b5899ed (prepare.py checks game.wasm's SHA-256):
// GetModelInfoFromLocalIndex reads the pool's capacity at 10872120; a model stores its name hash at
// +24 and its type at +157, and vehicle models have type 5.
u64 PG_FN_MODEL_INFO_AT_INDEX(w2c_game*, u32);
u32 PG_FN_IS_VEHICLE_MODEL(w2c_game*, u32);
u64 PG_FN_HASH_STRING_TRY_GET(w2c_game*, u32, u32);
u32 PG_FN_VEHICLE_CLASS(w2c_game*, u32);
u64 PG_FN_VEHICLE_LABEL(w2c_game*, u32);
u32 PG_FN_VEHICLE_SEATS(w2c_game*, u32);
void PG_FN_REQUEST_MODEL(w2c_game*, u32);
void PG_FN_RELEASE_MODEL(w2c_game*, u32);
u32 PG_FN_HAS_MODEL_LOADED(w2c_game*, u32);
void PG_FN_GET_ENTITY_COORDS(w2c_game*, u64, u32, u32);
f32 PG_FN_GET_ENTITY_HEADING(w2c_game*, u32);
u32 PG_FN_CREATE_VEHICLE(w2c_game*, u32, u64, f32, u32, u32, u32);
void PG_FN_SET_PED_INTO_VEHICLE(w2c_game*, u32, u32, u32);
u32 PG_FN_GROUND_VEHICLE(w2c_game*, u32, f32);
u32 PG_FN_ENTITY_EXISTS(w2c_game*, u32);
u32 PG_FN_GET_ENTITY_MODEL(w2c_game*, u32);
u32 PG_FN_SPAWN_ENTITY_VISIBLE(w2c_game*, u32);
u32 PG_FN_SPAWN_ENTITY_ALPHA(w2c_game*, u32);
u32 PG_FN_ENTITY_OWNED(w2c_game*, u32, u32);
void PG_FN_DELETE_VEHICLE(w2c_game*, u64);
u32 PG_FN_VEHICLE_OCCUPANT(w2c_game*, u32, u32, u32);
u64 PG_FN_CURRENT_SCRIPT_HANDLER(w2c_game*);

// Vehicle classes with their own handling here (GET_VEHICLE_CLASS_FROM_NAME).
enum { PG_CLASS_BOAT = 14, PG_CLASS_HELICOPTER = 15, PG_CLASS_PLANE = 16, PG_CLASS_TRAIN = 21 };

// The menu keeps owning what it spawned until it deletes it. "No longer needed" alone leaves nearby
// vehicles (and their streamed models) alive, so repeated spawns grew memory without bound.
static u32 menuVehicles[2];
static u64 menuVehicleOwner;
static bool pg_vehicle_make_room(w2c_game *instance, u64 scratch, u64 owner) {
  if (menuVehicleOwner != owner) {
    memset(menuVehicles, 0, sizeof menuVehicles);
    menuVehicleOwner = owner; // A new main script: the old one cleaned up its own vehicles.
  }
  for (int i = 0; i < 2; i++) {
    u32 vehicle = menuVehicles[i];
    if (vehicle && (!PG_FN_ENTITY_EXISTS(instance, vehicle) || !PG_FN_ENTITY_OWNED(instance, vehicle, 0)))
      menuVehicles[i] = 0; // Never take a vehicle back from a mission that took it over.
  }
  if (!menuVehicles[0] || !menuVehicles[1]) return true;
  for (int i = 0; i < 2; i++) {
    bool occupied = false;
    // This build has 16 seats per vehicle; also counts anyone getting in.
    for (int seat = -1; seat < 15; seat++)
      if (PG_FN_VEHICLE_OCCUPANT(instance, menuVehicles[i], (u32)seat, 1)) { occupied = true; break; }
    if (occupied) continue;
    u32 vehicle = menuVehicles[i];
    *(u32 *)pg_pointer(&env, scratch, 4) = vehicle;
    PG_FN_DELETE_VEHICLE(instance, scratch);
    // DELETE_VEHICLE clears its argument even when it fails in this build, so ask the game instead.
    if (PG_FN_ENTITY_EXISTS(instance, vehicle)) continue;
    menuVehicles[i] = 0;
    pg_log("Vehicle menu: removed an older empty menu vehicle before loading another.");
    return true;
  }
  return false;
}
static bool pg_vehicle_memory_available(void) {
  uint64_t headroom;
  return !nativeProcessHeadroom(&headroom) || headroom >= guardReserve();
}
// The name the game shows for a vehicle, in the game's language: its display-name label
// (GET_DISPLAY_NAME_FROM_VEHICLE_MODEL, "ADDER") looked up in the game's text with _GET_LABEL_TEXT,
// the lookup script mods use. Empty when the game has no text for it.
static void pg_vehicle_display_name(w2c_game *instance, uint32_t model, char *out, size_t size) {
  out[0] = 0;
  const u64 label = PG_FN_VEHICLE_LABEL(instance, model);
  if (!label) return;
  const uint64_t argument[1] = {label};
  const u64 text = pg_test_call(instance, 0x7b5280eba9840c72ull, 1, argument);
  if (!text || text == (u64)-1) return;
  const char *words = pg_string(&env, text);
  if (!words[0] || !strcmp(words, "NULL")) return;
  // Cut a long name at a whole character: Japanese, Korean, Chinese and Russian letters take several bytes.
  size_t n = strlen(words);
  if (n >= size) { n = size - 1; while (n && ((uint8_t)words[n] & 0xC0) == 0x80) n--; }
  memcpy(out, words, n); out[n] = 0;
}

static pthread_mutex_t vehicleMenuLock = PTHREAD_MUTEX_INITIALIZER;
static PGVehicleEntry *vehicleCatalog;
static int32_t vehicleCatalogCount = -1;
static bool vehicleCatalogRequested = true; // Read the list during play, before Settings pauses the engine.
static uint32_t vehicleRequestedModel;
static bool vehicleRequestedEnter;
static PGVehicleSpawnStatus vehicleSpawnStatus;

void PGRequestVehicleCatalog(void) {
  pthread_mutex_lock(&vehicleMenuLock);
  vehicleCatalogRequested = true;
  pthread_mutex_unlock(&vehicleMenuLock);
}
int32_t PGVehicleCatalog(const PGVehicleEntry **entries) {
  pthread_mutex_lock(&vehicleMenuLock);
  int32_t count = vehicleCatalogCount;
  if (entries) *entries = vehicleCatalog;
  pthread_mutex_unlock(&vehicleMenuLock);
  return count;
}
bool PGSpawnVehicle(uint32_t model, bool enter) {
  pthread_mutex_lock(&vehicleMenuLock);
  if (vehicleRequestedModel != 0) { pthread_mutex_unlock(&vehicleMenuLock); return false; }
  vehicleRequestedModel = model;
  vehicleRequestedEnter = enter;
  vehicleSpawnStatus = PGVehicleSpawnLoading;
  pthread_mutex_unlock(&vehicleMenuLock);
  return true;
}
PGVehicleSpawnStatus PGVehicleStatus(void) {
  pthread_mutex_lock(&vehicleMenuLock);
  PGVehicleSpawnStatus s = vehicleSpawnStatus;
  pthread_mutex_unlock(&vehicleMenuLock);
  return s;
}
static void pg_vehicle_spawn_status(PGVehicleSpawnStatus status) {
  pthread_mutex_lock(&vehicleMenuLock);
  vehicleRequestedModel = 0;
  vehicleSpawnStatus = status;
  pthread_mutex_unlock(&vehicleMenuLock);
}

// Builds the catalog once, then lets the player scroll it smoothly in Settings.
static void pg_vehicle_build_catalog(w2c_game *instance) {
  const u32 capacity = *(const u32 *)pg_pointer(&env, 10872120, 4);
  PGVehicleEntry *list = malloc(capacity * sizeof *list);
  if (!list) return;
  int32_t count = 0;
  for (u32 i = 0; i < capacity; i++) {
    const u64 info = PG_FN_MODEL_INFO_AT_INDEX(instance, i);
    if (!info) continue;
    const uint8_t *bytes = pg_pointer(&env, info, 158);
    if (bytes[157] != 5) continue; // Not a vehicle model
    const uint32_t model = *(const uint32_t *)(bytes + 24);
    if (!PG_FN_IS_VEHICLE_MODEL(instance, model)) continue;
    const u32 category = PG_FN_VEHICLE_CLASS(instance, model);
    if (category == PG_CLASS_TRAIN) continue; // Trains only spawn through their tracks script
    list[count].model = model;
    list[count].category = category;
    pg_vehicle_display_name(instance, model, list[count].name, sizeof list[count].name);
    // If the game had no label for it, show its model name if the engine's name table knows it.
    if (!list[count].name[0]) {
      const u64 string = PG_FN_HASH_STRING_TRY_GET(instance, 0, model);
      if (string) snprintf(list[count].name, sizeof list[count].name, "%s", pg_string(&env, string));
      else snprintf(list[count].name, sizeof list[count].name, "0x%08x", model);
    }
    count++;
  }
  pthread_mutex_lock(&vehicleMenuLock);
  free(vehicleCatalog);
  vehicleCatalog = list;
  vehicleCatalogCount = count;
  vehicleCatalogRequested = false;
  pthread_mutex_unlock(&vehicleMenuLock);
  char line[96]; snprintf(line, sizeof line, "Vehicle menu: found %d vehicles in the model registry.", count); pg_log(line);
}

// Spawns in front of the player (or further away for planes and helicopters).
static void pg_vehicle_menu_step(w2c_game *instance, u64 vector) {
  pthread_mutex_lock(&vehicleMenuLock);
  bool readCatalog = vehicleCatalogRequested && vehicleCatalogCount < 0;
  uint32_t model = vehicleRequestedModel;
  bool enter = vehicleRequestedEnter;
  pthread_mutex_unlock(&vehicleMenuLock);
  if (readCatalog) pg_vehicle_build_catalog(instance);
  static uint32_t requestedModel;
  static bool requested;
  static u64 requestOwner;
  static double began, loadingReported;
  if (!model) return;
  double now = timeMS(CLOCK_MONOTONIC);
  bool ready = !(PGGameStatus() & 16) && !PGLoadSceneActive();
  u64 previous = pg_asi_enter_script(instance);
  u64 active = *(u64 *)pg_pointer(&env, instance->w2c_g1 + 3088, 8);
  u64 owner = active ? PG_FN_CURRENT_SCRIPT_HANDLER(instance) : 0;
  if (!owner || (requested && owner != requestOwner)) {
    pg_asi_leave_script(instance, previous);
    began = 0; requested = false;
    pg_log("Vehicle menu: the main script ended or changed; spawn cancelled.");
    pg_vehicle_spawn_status(PGVehicleSpawnNotReady); return;
  }
  PGVehicleSpawnStatus result = PGVehicleSpawnLoading;
  if (!requested) {
    if (!PG_FN_IS_VEHICLE_MODEL(instance, model) || PG_FN_VEHICLE_CLASS(instance, model) == PG_CLASS_TRAIN) result = PGVehicleSpawnUnavailable;
    else if (!pg_vehicle_make_room(instance, vector, owner)) result = PGVehicleSpawnVehicleLimit;
    else if (!pg_vehicle_memory_available()) result = PGVehicleSpawnLowMemory;
    else {
      pg_log("Vehicle menu: loading the model.");
      PG_FN_REQUEST_MODEL(instance, model); loadingReported = now; requested = true; requestOwner = owner; began = now;
      pg_vehicle_spawn_status(PGVehicleSpawnLoading);
    }
  }
  if (requested) {
    if(now-loadingReported>=5000){char line[160];snprintf(line,sizeof line,"Vehicle model wait: 0x%08x, %.1f s since REQUEST_MODEL, gameplay 0x%x.",model,(now-began)/1000,PGGameStatus());pg_log(line);loadingReported=now;}
    if (!ready) result = PGVehicleSpawnNotReady;
    else if (now - began > 30000) result = PGVehicleSpawnTimedOut;
    else if (!pg_vehicle_memory_available()) result = PGVehicleSpawnLowMemory;
    else if (PG_FN_HAS_MODEL_LOADED(instance, model)) {
      const u32 ped = PG_FN_PLAYER_PED_ID(instance);
      if (!vector || !ped) result = PGVehicleSpawnNotReady;
      else {
        PG_FN_GET_ENTITY_COORDS(instance, vector, ped, 1);
        float *v = pg_pointer(&env, vector, 24);
        float heading = PG_FN_GET_ENTITY_HEADING(instance, ped);
        uint32_t category = PG_FN_VEHICLE_CLASS(instance, model);
        float distance = (category == PG_CLASS_HELICOPTER || category == PG_CLASS_PLANE) ? 20.0f : 8.0f;
        float angle = heading * 0.017453292519943295f;
        v[0] -= sinf(angle) * distance; v[2] += cosf(angle) * distance; v[4] += 0.5f;
        pg_log("Vehicle menu: model loaded; creating the vehicle.");
        u32 vehicle = PG_FN_CREATE_VEHICLE(instance, model, vector, heading, 0, 0, 1);
        bool exists = vehicle && PG_FN_ENTITY_EXISTS(instance, vehicle);
        uint32_t actualModel = exists ? PG_FN_GET_ENTITY_MODEL(instance, vehicle) : 0;
        result = exists && actualModel == model ? PGVehicleSpawnSucceeded : PGVehicleSpawnFailed;
        if (exists && actualModel == model) {
          if (category != PG_CLASS_BOAT) PG_FN_GROUND_VEHICLE(instance, vehicle, 5.0f);
          if (enter && PG_FN_VEHICLE_SEATS(instance, model) > 0) PG_FN_SET_PED_INTO_VEHICLE(instance, ped, vehicle, (u32)-1);
          if (!menuVehicles[0]) menuVehicles[0] = menuVehicles[1];
          menuVehicles[1] = vehicle;
        }
      }
    }
  }
  if (result != PGVehicleSpawnLoading && requested) PG_FN_RELEASE_MODEL(instance, model);
  pg_asi_leave_script(instance, previous);
  if (result != PGVehicleSpawnLoading) {
    char line[128]; snprintf(line, sizeof line, "Vehicle menu: model 0x%08x finished with status %d.", model, result); pg_log(line);
    began = 0; requested = false;
    pg_vehicle_spawn_status(result);
  }
}

static void pg_vehicle_menu_tick(w2c_game *instance) {
  static bool ticking;
  if (ticking) return;
  ticking = true;
  u64 stack = instance->w2c_g0;
  instance->w2c_g0 -= 32;
  pg_vehicle_menu_step(instance, instance->w2c_g0);
  instance->w2c_g0 = stack;
  ticking = false;
}

// BẬT CHẾ ĐỘ ĐẠI DỊCH ZOMBIE VÀ THỰC THI MỖI FRAME
#include "NativeZombieMode.inc"

// Settings › Cheats, once per frame on the game thread, in the "main" script's turn
// (pg_asi_script_tick): spawns, one-off actions, and keeping God mode and Never wanted on.
static void pg_cheats_menu_tick(w2c_game *instance) {
  pg_vehicle_menu_tick(instance);
  pg_native_cheats_tick(instance);
  pg_zombie_mode_tick(instance);
}
