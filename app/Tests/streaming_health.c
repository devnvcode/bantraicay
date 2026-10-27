#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
static uint8_t memory[12000000];
const uint8_t *pg_engine_memory(uint64_t *size){*size=sizeof memory;return memory;}
#include "../Source/NativeStreamingHealth.c"
int main(void){
 uint64_t base=11700000;uint32_t count=4;memcpy(memory+11607304,&base,8);memcpy(memory+11607312,&count,4);
 uint64_t out[6];
 for(unsigned n=0;n<1000000;n++)pg_stream_removed(0);
 PGStreamingHealth(out);assert(out[5]==0);
 memory[base+4]=LOADED;pg_stream_removed(0);PGStreamingHealth(out);assert(out[5]==1);
 memory[base+16+4]=REQUESTED;pg_stream_requested(1);pg_stream_removed(1);
 PGStreamingHealth(out);assert(out[0]==0&&out[5]==1);
 puts("PASS streaming health: one million no-op removals excluded; actual removal calls retained");return 0;
}
