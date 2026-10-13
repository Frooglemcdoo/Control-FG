#include "../src/pt_primary_readiness.h"
#include <cassert>
#include <cstdio>
int main(){using namespace PTPrimaryReadiness;
    assert(Check(2000,1980,20,20)==Result::healthy);
    assert(Check(2000,1600,400,400)==Result::healthy); // Aiming at unsupported hair/cloth.
    assert(Check(2000,0,2000,2000)==Result::noSamples); // No global demotion or false promotion.
    assert(Check(2000,1600,400,0)==Result::unhealthy); // Real transport failures still demote.
    assert(Check(2000,1500,500,400)==Result::unhealthy);
    assert(Check(2000,1500,0,0)==Result::unhealthy); // Failed output commits.
    assert(Check(0,0,0,0)==Result::noSamples);
    assert(Check(1,2,0,0)==Result::unhealthy);
    assert(Check(1,0,0,1)==Result::unhealthy);
    puts("PASS: unsupported pixels stay local; real faults and failed commits retain admission gate.");
}
