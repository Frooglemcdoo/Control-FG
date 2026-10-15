#include "../src/pt_owned_scene.h"
#include <chrono>
#include <cstdio>
int main(){PTOwnedScene::Index index;for(unsigned i=1;i<=4096;++i)index.Insert({i,0x100000ull+i*65536ull,65536});volatile unsigned long long total=0;for(unsigned round=0;round<3;++round){auto start=std::chrono::steady_clock::now();for(unsigned frame=0;frame<400;++frame){if(round!=0)index.Insert({4097,0x50000000,4096});for(unsigned i=0;i<16384;++i){unsigned n=(i*4051)%4096+1;total+=index.Find(0x100000ull+n*65536ull+(i%4)*64,64);}}printf("%s %.3f ms/frame checksum=%llu\n",round?"invalidated":"warm",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/400,total);}}
