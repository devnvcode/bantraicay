#include "TurboModel.h"
#include <assert.h>
#include <stdio.h>
static float run(unsigned hz) {
    PGTurboConfig c=pg_turbo_defaults();float boost=0;
    for(unsigned i=0;i<hz;i++)boost=pg_turbo_step(&c,boost,.8f,1,3,1.f/hz);
    return boost;
}
int main(void) {
    PGTurboConfig c=pg_turbo_defaults();assert(pg_turbo_valid(&c));
    assert(fabsf(run(30)-run(120))<.00001f);
    assert(run(60)>.998f&&run(60)<1);
    c.antiLag=false;assert(pg_turbo_step(&c,0,.8f,0,3,.1f)<0);
    c.antiLag=true;assert(pg_turbo_step(&c,0,.8f,0,3,.1f)>0);
    c.gearBoost=true;c.gears[0]=.25f;c.gears[9]=2;
    assert(pg_turbo_step(&c,0,.8f,1,1,.1f)<pg_turbo_step(&c,0,.8f,1,10,.1f));
    assert(pg_turbo_step(&c,0,.8f,1,0,.1f)==pg_turbo_step(&c,0,.8f,1,1,.1f));
    assert(pg_turbo_step(&c,0,.8f,1,100,.1f)==pg_turbo_step(&c,0,.8f,1,10,.1f));
    c.spoolRate=1;assert(!pg_turbo_valid(&c));c=pg_turbo_defaults();
    c.maxBoost=NAN;assert(!pg_turbo_valid(&c));c=pg_turbo_defaults();
    c.periodMS=0;assert(!pg_turbo_valid(&c));
    puts("turbo: frame-rate independence, vacuum, anti-lag, reverse/high gears and validation passed");
}
