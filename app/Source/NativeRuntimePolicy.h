#pragma once
#include <stdint.h>
#include <stdbool.h>
static inline uint64_t pg_default_engine_reservation_mb(uint64_t processMB){
 return processMB>=5120?(processMB-1024<7168?processMB-1024:7168):4095;
}
static inline uint64_t pg_asi_instruction_slice(bool script,uint64_t work){return script&&work?8000000:20000000;}
static inline bool pg_asi_runaway(uint64_t work){return work>=3000000000ull;}
