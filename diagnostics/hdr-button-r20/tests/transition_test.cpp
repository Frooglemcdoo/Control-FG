#include "transition.h"
#include <cstdio>
#include <cstdlib>
using namespace hdrbutton;
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"FAIL %d %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 State s{};CHECK(!s.active());CHECK(ButtonText(s.phase,false,true)[4]=='O');
 s.phase=Phase::RequestOff;CHECK(s.active());CHECK(ButtonText(s.phase,false,true)[3]==L'.');
 s.phase=Phase::Failed;CHECK(!s.active());CHECK(ButtonText(s.phase,false,true)[4]==L'E');
 s.phase=Phase::Complete;CHECK(!s.active());
 std::puts("PASS transition/button-state policy");
 return 0;
}
