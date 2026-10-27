#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "TurboModel.h"
PGTurboConfig PGTurboDefaultConfig(void);
bool PGSetTurboConfig(PGTurboConfig config);
bool PGTurboTelemetry(float *rpm, float *boost, unsigned *gear);

typedef void (*PGLogCallback)(const char *, void *);
typedef struct {
  float leftX, leftY, rightX, rightY, leftTrigger, rightTrigger;
  uint32_t buttons;
  int32_t lookDX, lookDY;
} PGControllerState;

void PGConfigureEngine(const char *const *quality, unsigned count, int width, int height, int frameLimit);
void PGSetFrameLimit(int frameLimit);
void PGSetMemoryCap(int mb);
void PGSetStartMode(int32_t mode);
void PGSetBackBufferSize(int width, int height);
void PGSetAssetServer(const char *url);
void PGSetLocalGameFolder(const char *dataPath);
void PGSetMinimapLayout(double dx, double dy, double scale);
bool PGUsingLocalGameFolder(void);
void PGStartEngine(const char *manifestPath, const char *filesRoot, PGLogCallback, void *context);
void PGUpdateController(PGControllerState state, double elapsedSeconds);
uint32_t PGCurrentTouchContext(void);
uint32_t PGGameStatus(void);
bool PGTVPresentation(int32_t *playMs, int32_t *durationMs);
void PGSetSubtitleRaise(double raise);
bool PGModFileProblem(const char *path, char *why, size_t n);
void PGAsiSetKey(int virtualKey, bool down);
bool PGAsiModKeys(uint8_t keys[256]);
#include "NativeASI.h"
void PGAsiGetKeyInfo(PGAsiKeyInfo *info);
bool PGAsiTextRequest(char *title, size_t titleSize, char *text, size_t textSize, int *maxLength);
void PGAsiTextFinish(const char *text);

typedef struct {
  bool specialShown;
  bool specialReady;
  bool specialActive;
  float specialFill;
  bool canDetonate;
  bool helpDetonate;
  bool helpShown;
  bool helpContext;
  uint32_t helpInputs;
  bool radioWheelOpen;
  uint32_t vehicleModel;
  uint32_t pedModel;
  bool playerDying;
  bool playerUnderwater;
  bool playerParachuting;
  bool phoneOpen;
  bool weaponWheelOpen;
  bool trailerAttached;
  bool submarineSurfaced;
  bool lockOnAvailable;
  bool isPassenger;
  bool isTaxiPassenger;
  bool missionScriptRunning;
} PGTouchContextState;

void PGCopyTouchContextState(PGTouchContextState *out);
void PGSetPlayerCash(int32_t amount);
void PGNextAircraftWeapon(void);

typedef enum {
  PGVehicleSpawnIdle, PGVehicleSpawnLoading,
  PGVehicleSpawnSucceeded, PGVehicleSpawnUnavailable, PGVehicleSpawnTimedOut,
  PGVehicleSpawnNotReady, PGVehicleSpawnFailed,
  PGVehicleSpawnLowMemory, PGVehicleSpawnVehicleLimit
} PGVehicleSpawnStatus;

bool PGSpawnVehicle(uint32_t model, bool enterVehicle);
PGVehicleSpawnStatus PGGetVehicleSpawnStatus(void);

typedef enum {
  PGCheatRepairVehicle, PGCheatCleanVehicle, PGCheatSetTime,
  PGCheatSetWeather, PGCheatNaturalWeather, PGCheatCameraMode
} PGCheatCommand;
typedef enum {
  PGCheatIdle, PGCheatWaiting, PGCheatSucceeded, PGCheatNoVehicle, PGCheatNotReady
} PGCheatStatus;

bool PGQueueCheat(PGCheatCommand command, int32_t value);
PGCheatStatus PGGetCheatStatus(void);

typedef enum { PGCheatGodMode, PGCheatNeverWanted } PGCheatSwitch;
bool PGCheatEnabled(PGCheatSwitch cheat);
bool PGSetPersistentCheatEnabled(PGCheatSwitch cheat, bool enabled);

int PGLoadSceneSeconds(void);
bool PGLoadSceneActive(void);
int PGInputPolledWithin(int action, double ms);
int PGInputReadWithin(int action, double ms);
void PGRequestPauseMenu(void);
void PGSetKey(uint8_t virtualKey, bool down);
void PGSetRadioWheelDirection(float x, float y, bool active);
void PGReleaseAllKeys(void);
void PGMouseInput(int32_t dx, int32_t dy, int32_t buttons);
void PGMouseWheel(int32_t notches);
void PGSetMouseFocus(bool focused, bool locked);
bool PGDebugReadMemory(uint64_t address, void *out, uint64_t length);

typedef struct {
  uint32_t buttons;
  float leftX, leftY, rightX, rightY, leftTrigger, rightTrigger;
} PGPadState;

void PGUpdateVirtualPad(PGPadState state);
void PGQueueCameraSwipe(float x, float y);
void PGClearCameraSwipe(void);
int32_t PGLastInputDevice(void);
int32_t PGTargetingMode(void);
void PGSetTargetingMode(int32_t mode);
bool PGPauseMenuActive(void);
void PGFillAudio(float *left, float *right, uint32_t count);
uint64_t PGMemoryBudgetQueries(void);
uint64_t PGMemoryBudgetClamps(void);
uint64_t PGGPUTextureBytes(void);
void PGLogMemoryPools(void);
uint64_t PGMemoryBudgetEvictions(void);
void PGAssetHealth(uint64_t out[6]);
void PGStreamingHealth(uint64_t out[6]);
double PGGPUFrameMilliseconds(void);

void PGEnableSoundMods(bool enabled);
uint32_t PGCurrentSoundVehicle(void);
bool PGSelectVehicleSound(uint32_t model,const char *profile);
void PGLogStreamingPolicy(void);

// Native LSPDFR (First Response Police System & LCPDFR Mod Suite)
bool PGLSPDFRIsOnDuty(void);
void PGLSPDFRSetDuty(bool onDuty);
void PGLSPDFRSetAgency(int agency);
int PGLSPDFRGetAgency(void);
void PGLSPDFRSpawnVehicle(int index);
void PGLSPDFRSpawnCustomVehicle(uint32_t modelHash);
void PGLSPDFRTriggerCallout(int calloutType);
void PGLSPDFRRequestBackup(int backupType);
void PGLSPDFRTrafficStop(void);
void PGLSPDFRArrestNearby(void);
void PGLSPDFRCallTransport(void);
void PGLSPDFRToggleRadar(void);
bool PGLSPDFRIsRadarActive(void);
void PGLSPDFRGetStatus(char *buffer, size_t maxLen);
