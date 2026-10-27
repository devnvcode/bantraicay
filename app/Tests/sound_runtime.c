#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
typedef uint64_t u64;typedef uint32_t u32;
typedef struct { u64 w2c_g0; } w2c_game;
static char env[2048];static double now;
static unsigned lookupCalls,forced,logs;static bool ready;
static u32 currentVehicle=10;
static double timeMS(int clock){(void)clock;return now;}
static int PGPauseMenuActive(void){return 0;}
static int PGGameStatus(void){return 0;}
static bool sceneActive;
static bool PGLoadSceneActive(void){return sceneActive;}
static u64 pg_asi_enter_script(w2c_game *i){(void)i;return 0;}
static void pg_asi_leave_script(w2c_game *i,u64 old){(void)i;(void)old;}
static u32 PG_FN_PLAYER_PED_ID(w2c_game *i){(void)i;return 1;}
static u32 PG_FN_PED_VEHICLE(w2c_game *i,u32 p,int last){(void)i;(void)p;(void)last;return currentVehicle;}
static u32 pg_joaat(const char *p){(void)p;return 123;}
static void *pg_pointer(void *e,u64 p,unsigned n){(void)e;assert(p+n<=sizeof env);return env+p;}
static void pg_log(const char *line){(void)line;logs++;}
#include "../Source/NativeSoundMods.inc"
u64 PG_FN_AUDIO_PROFILE(w2c_game *i,u64 manager,u32 hash,u32 type){
 (void)i;(void)manager;(void)hash;assert(type==3);lookupCalls++;return ready?1:0;
}
void PG_FN_FORCE_AUDIO(w2c_game *i,u32 v,u64 address){(void)i;assert(v==currentVehicle);assert(!strcmp(env+address,"brabus850"));forced++;}
u32 PG_FN_GET_ENTITY_MODEL(w2c_game *i,u32 v){(void)i;(void)v;return 99;}
int main(void){
 w2c_game instance={1024};PGEnableSoundMods(true);assert(PGSelectVehicleSound(99,"brabus850"));
 sceneActive=true;pg_sound_mod_tick(&instance);assert(lookupCalls==0&&forced==0);sceneActive=false;
 pg_sound_mod_tick(&instance);assert(lookupCalls==1&&forced==0);
 now=500;pg_sound_mod_tick(&instance);assert(lookupCalls==1);
 now=2000;pg_sound_mod_tick(&instance);assert(lookupCalls==2);
 ready=true;now=4000;pg_sound_mod_tick(&instance);assert(lookupCalls==3&&forced==1&&instance.w2c_g0==1024);
 for(unsigned j=0;j<100;j++){now+=500;pg_sound_mod_tick(&instance);}
 assert(forced==1&&lookupCalls==3);
 ready=false;currentVehicle=11;
 for(unsigned j=0;j<100;j++){now+=500;pg_sound_mod_tick(&instance);}
 assert(lookupCalls==13&&forced==1&&logs==2);
 PGEnableSoundMods(false);now+=500;pg_sound_mod_tick(&instance);assert(lookupCalls==13);
 puts("PASS: delayed audio mount retried, applied once, retry and logging bounded");return 0;
}
