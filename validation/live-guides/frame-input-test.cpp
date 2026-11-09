#include <cassert>
#include <cstdio>
#include <limits>
#include "../../src/rr_live_frame_input.h"
int main(){
 using control_rr::LiveFrameInput;
 LiveFrameInput in{42,-.4375f,.388888955f,1,0x7F,0,2560,1440};
 assert(in.MatchesExtent(2560,1440)&&!in.MatchesExtent(1920,1080));assert(in.Matches(42));assert(!in.Matches(41));assert(!in.Matches(43));
 for(unsigned bit=0x10;bit<=0x40;bit<<=1){auto bad=in;bad.valid&=~bit;assert(!bad.Matches(42));}
 auto bad=in;bad.fault=123;assert(!bad.Matches(42));bad=in;bad.frame=0;assert(!bad.Matches(0));
 bad=in;bad.jitterX=std::numeric_limits<float>::quiet_NaN();assert(!bad.Matches(42));
 bad=in;bad.jitterY=std::numeric_limits<float>::infinity();assert(!bad.Matches(42));
 const auto owned=in;in.frame=43;in.jitterX=.125f;in.reset=0;
 assert(owned.Matches(42)&&owned.jitterX==-.4375f&&owned.reset==1);
 // Zero native jitter is valid, not a missing-value marker.
 in.jitterX=in.jitterY=0;assert(in.Matches(43));
 puts("PASS: actual native metadata gate rejects missing/nonfinite/faulted/stale input; value copy survives next frame changes; zero jitter accepted");
}
