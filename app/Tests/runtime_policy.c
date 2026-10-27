#include "NativeRuntimePolicy.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 assert(pg_default_engine_reservation_mb(2348)==4095);
 assert(pg_default_engine_reservation_mb(3072)==4095);
 assert(pg_default_engine_reservation_mb(4096)==4095);
 assert(pg_default_engine_reservation_mb(5120)==4096);
 assert(pg_default_engine_reservation_mb(8192)==7168);
 assert(pg_default_engine_reservation_mb(16384)==7168);
 assert(pg_asi_instruction_slice(false,20000000)==20000000);
 assert(pg_asi_instruction_slice(true,0)==20000000);
 assert(pg_asi_instruction_slice(true,20000000)==8000000);
 assert(!pg_asi_runaway(2999999999ull));assert(pg_asi_runaway(3000000000ull));
 puts("PASS: 4 GB device reservation remains virtual; high-budget reservation bounded at 7 GiB; script slices and runaway boundary.");
}
