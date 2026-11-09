#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>

static double pq(double nits) {
    const double m1=2610.0/16384.0;
    const double m2=2523.0/32.0;
    const double c1=3424.0/4096.0;
    const double c2=2413.0/128.0;
    const double c3=2392.0/128.0;
    const double L=std::clamp(std::max(nits,0.0)/10000.0,0.0,1.0);
    const double Lm=std::pow(L,m1);
    return std::pow((c1+c2*Lm)/(1.0+c3*Lm),m2);
}

static std::array<double,3> linear709To2020(std::array<double,3> c) {
    return {
        0.6274040*c[0]+0.3292820*c[1]+0.0433136*c[2],
        0.0690970*c[0]+0.9195400*c[1]+0.0113612*c[2],
        0.0163916*c[0]+0.0880132*c[1]+0.8955950*c[2]
    };
}

static std::array<double,3> bridge(std::array<double,3> scrgb) {
    auto c=linear709To2020(scrgb);
    for(auto& v:c)v=pq(std::max(v,0.0)*80.0);
    return c;
}

static bool near(double a,double b,double eps=1e-9){return std::abs(a-b)<=eps;}

int main(){
    auto white=bridge({1.0,1.0,1.0});
    // The Rec.709 -> Rec.2020 matrix preserves neutral white, so 1.0 scRGB
    // becomes 80 nits in all channels before PQ encoding.
    for(double v:white)assert(near(v,0.4858567653886785,2e-7));
    auto black=bridge({0.0,0.0,0.0});
    for(double v:black)assert(near(v,7.309559025783966e-7,1e-12));
    auto red=bridge({1.0,0.0,0.0});
    assert(red[0]>red[1]&&red[1]>red[2]);
    auto hdr=bridge({12.5,12.5,12.5}); // 1000 nit neutral
    for(double v:hdr)assert(near(v,0.751827096247041,2e-7));
    assert(pq(10000.0)==1.0);
    std::puts("PASS HDR HUDless color conversion: shared scRGB/709 -> BT.2020 -> 80-nit-scaled PQ reference vectors");
}
