#include "NativeGPUMetrics.h"
#include <cassert>
#include <limits>
#include <cstdio>
int main(){
 PGGPUWorkWindow w;
 // A 20-buffer frame is 20 ms, not 1 ms. Reproduce the tester's buffer count.
 for(int i=0;i<20;i++)w.record(.001,i==19);
 assert(w.takeMillisecondsPerFrame()==20.0);
 assert(w.takeMillisecondsPerFrame()==0);
 for(int i=0;i<6;i++)w.record(.002,i==2||i==5);
 assert(w.takeMillisecondsPerFrame()==6.0);
 w.record(std::numeric_limits<double>::quiet_NaN(),false);
 w.record(-1,false);w.record(2,false);w.record(.001,true);
 assert(w.takeMillisecondsPerFrame()==1.0);
 w.record(.002,false);assert(w.takeMillisecondsPerFrame()==0);
 w.record(.003,true);assert(w.takeMillisecondsPerFrame()==5.0);
 puts("PASS: GPU work sums command buffers per presented frame, invalid timestamps ignored, pending work retained and windows reset.");
}
