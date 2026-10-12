#include "../src/shaders/pt_geometry_bounds.hlsli"
#include <cstdio>
#include <cstdlib>
using namespace control_pt_geometry;
void check(bool x){if(!x){std::puts("FAIL geometry bounds");std::exit(1);}}
int main(){
    check(PTCheckedTriangle(0,0,2,6).valid);
    check(PTCheckedTriangle(1,2,2,14).address==8);
    check(!PTCheckedTriangle(2,2,2,14).valid);
    check(!PTCheckedTriangle(0xffffffffu,0,4,0xffffffffu).valid);
    check(!PTCheckedTriangle(0,1,2,16).valid);
    check(!PTCheckedTriangle(0,0,1,16).valid);
    check(PTCanLoadWord(4,8));check(!PTCanLoadWord(4,6));
    check(!PTCanLoadWord(0xfffffffc,0xffffffffu));
    check(PTCheckedField(2,24,8,8,64).valid);
    check(PTCheckedField(2,24,8,8,64).address==56);
    check(!PTCheckedField(3,24,8,8,64).valid);
    check(!PTCheckedField(0xffffffffu,24,8,8,0xffffffffu).valid);
    check(!PTCheckedField(0,24,22,4,64).valid);
    check(!PTCheckedField(0,24,2,4,64).valid);
    check(!PTCheckedField(0,0,0,4,64).valid);
    std::puts("PASS: geometry ranges, alignment, tail reads and overflow rejection");
}
