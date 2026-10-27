#pragma once
#include <atomic>
#include <cmath>
#include <cstdint>
// Sum every completed command buffer's GPU duration; divide by completed presents,
// not the number of command buffers. Windows are approximate across callback races.
struct PGGPUWorkWindow {
 std::atomic<uint64_t> micros{0},frames{0};
 void record(double seconds,bool present){
  if(std::isfinite(seconds)&&seconds>0&&seconds<1)micros.fetch_add((uint64_t)(seconds*1e6));
  if(present)frames.fetch_add(1);
 }
 double takeMillisecondsPerFrame(){
  uint64_t count=frames.exchange(0);
  if(!count)return 0;
  return micros.exchange(0)/1000.0/count;
 }
};
