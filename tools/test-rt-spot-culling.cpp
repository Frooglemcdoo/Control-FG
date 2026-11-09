#include "../src/rt_spot_culling_policy.h"
#include <cassert>
#include <cmath>
#include <limits>
#include <iostream>
int main() {
    using namespace native_lighting::spot_culling;
    assert(ExpandedView(1,150)==150);
    assert(ExpandedView(60,150)==150);
    assert(ExpandedView(180,150)==180);
    assert(ExpandedView(250,150)==250);
    assert(ExpandedView(1,1000)==200);
    assert(ExpandedView(1,20)==60);
    assert(ExpandedView(1,0)==1);
    assert(ExpandedView(-1,150)==-1);
    assert(ExpandedView(1,std::numeric_limits<float>::infinity())==1);
    assert(std::isnan(ExpandedView(std::numeric_limits<float>::quiet_NaN(),150)));
    // For a normalized side/near plane, expansion by radius retains any caster
    // inside that camera-centered radius despite changes in camera orientation.
    for(int angle=0;angle<360;++angle) {
        const float a=angle*3.14159265358979323846f/180;
        const float signedDistance=40*std::cos(a)+25*std::sin(a);
        assert(signedDistance>=-ExpandedView(1,150));
    }
    std::cout<<"PASS bounded caster envelope, native larger values, invalid inputs, camera rotation\n";
}
