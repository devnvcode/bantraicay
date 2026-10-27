#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <stdlib.h>
#include "TurboModel.h"
typedef uint64_t u64;typedef uint32_t u32;
typedef struct {int unused;} w2c_game;
static char env[2048];static double now=1000;static bool paused,loaded=true,driver=true;
static u32 vehicle=10;static unsigned fxCalls,audioCalls,requestCalls,nativeCalls,enters,leaves,toggleCalls;
static double timeMS(int clock){(void)clock;return now;}
static int PGPauseMenuActive(void){return paused;}
static int PGGameStatus(void){return 0;}
static bool sceneActive;
static bool PGLoadSceneActive(void){return sceneActive;}
static void pg_log(const char *s){assert(s&&*s);}
static u64 pg_asi_enter_script(w2c_game *i){(void)i;enters++;return 77;}
static void pg_asi_leave_script(w2c_game *i,u64 old){(void)i;assert(old==77);leaves++;}
static u32 PG_FN_PLAYER_PED_ID(w2c_game *i){(void)i;return 1;}
static u32 PG_FN_PED_VEHICLE(w2c_game *i,u32 ped,u32 last){(void)i;(void)last;assert(ped==1);return vehicle;}
u64 PG_FN_TURBO_ENTITY_ADDRESS(w2c_game *i,u32 handle){(void)i;return 10000+handle;}
void PGTurboAudioEnable(bool enabled,unsigned set){(void)enabled;assert(set<=2);}
void PGTurboPlayPop(float volume,bool loud){(void)loud;assert(volume>=0&&volume<=1);audioCalls++;}
static void *pg_pointer(void *memory,u64 offset,size_t size){(void)memory;assert(offset+size<=sizeof(env));return env+offset;}
static u64 pg_asi_alloc(size_t size){static u64 offset=32;u64 value=offset;offset+=size;assert(offset<sizeof(env));return value;}
static u64 pg_wasm_text(u64 *slot,const char *s){if(!*slot){*slot=pg_asi_alloc(strlen(s)+1);strcpy(env+*slot,s);}return *slot;}
static u64 pg_f32_arg(float v){u32 bits;memcpy(&bits,&v,4);return bits;}
static uint32_t random_value(uint32_t max){return max?0:0;}
#define arc4random_uniform random_value
static u64 pg_test_call(w2c_game *i,u64 hash,int count,const u64 *a){
    (void)i;nativeCalls++;
    switch(hash){
    case 0xbb40dd2270b65366ull:assert(count==3&&a[0]==vehicle);return driver?1:2;
    case 0x1f2aa07f00b3217aull:return 0;
    case 0x2a1f4f37f95bad08ull:assert(a[1]==18&&a[2]==1);toggleCalls++;return 0;
    case 0xfb71170b7e76acbaull:return !strcmp(env+a[1],"exhaust")?7:(u64)-1;
    case 0x8702416e512ec454ull:return loaded;
    case 0xb80d8756b4668ab6ull:requestCalls++;return 0;
    case 0x6c38af3693a69a91ull:assert(!strcmp(env+a[0],"core"));return 0;
    case 0x02b1f2a72e0f5325ull:
        assert(count==14&&a[1]==vehicle&&a[8]==7&&a[9]==pg_f32_arg(1));fxCalls++;return 1;
    default:assert(!"unexpected native");return (u64)-1;
    }
}
#include "NativeTurbo.inc"
int main(void){
    w2c_game game={0};PGTurboConfig c=PGTurboDefaultConfig();
    assert(PGSetTurboConfig(c));pg_turbo_tick(&game);assert(nativeCalls==0);
    assert(pg_native_turbo_boost(10010,0,.8f,0,2,.02f,.4f)==.4f);
    c.enabled=true;c.forceTurbo=true;c.randomMS=0;assert(PGSetTurboConfig(c));
    sceneActive=true;unsigned beforeScene=nativeCalls;pg_turbo_tick(&game);assert(nativeCalls==beforeScene);sceneActive=false;
    pg_turbo_tick(&game);assert(toggleCalls==1);
    assert(pg_native_turbo_boost(10011,0,.8f,0,2,.02f,.4f)==.4f);
    now+=250;float value=pg_native_turbo_boost(10010,0,.8f,0,2,.02f,.4f);assert(value>0&&value<1);
    pg_turbo_tick(&game);assert(fxCalls==1&&audioCalls==1);
    pg_turbo_tick(&game);assert(fxCalls==1&&audioCalls==1); // cooldown
    float rpm,boost;unsigned gear;assert(PGTurboTelemetry(&rpm,&boost,&gear));assert(rpm==.8f&&gear==2&&boost==value);
    loaded=false;now+=101;pg_native_turbo_boost(10010,value,.8f,0,2,.02f,.4f);pg_turbo_tick(&game);
    assert(requestCalls==1&&fxCalls==1&&audioCalls==2); // audio independent of streaming
    for(unsigned j=0;j<20;j++){now+=1001;pg_native_turbo_boost(10010,value,.8f,0,2,.02f,.4f);pg_turbo_tick(&game);}
    assert(requestCalls==10); // missing asset has bounded retry
    paused=true;pg_turbo_tick(&game);assert(!PGTurboTelemetry(&rpm,&boost,&gear));
    assert(pg_native_turbo_boost(10010,value,.8f,0,2,.02f,.4f)==.4f);
    paused=false;driver=false;pg_turbo_tick(&game);assert(!PGTurboTelemetry(&rpm,&boost,&gear));
    c.enabled=false;assert(PGSetTurboConfig(c));unsigned old=nativeCalls;pg_turbo_tick(&game);assert(nativeCalls==old);
    assert(enters==leaves);puts("turbo runtime: disabled/other-vehicle passthrough, driver gate, physics telemetry, bone FX, sound, cooldown, bounded streaming retries, pause and script context passed");
}
