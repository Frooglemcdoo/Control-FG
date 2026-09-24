#include "../../src/rr_no_clamp_test.h"
#include <cassert>
#include <cstdio>
int main(){for(unsigned contaminated=0;contaminated<2;++contaminated)for(unsigned prepared=0;prepared<2;++prepared)assert(control_rr::UseNativeSignalClamp(contaminated!=0,prepared!=0)==(contaminated==0&&prepared!=0));puts("PASS CS2 defaults to prepared uncontaminated native clamp");}
