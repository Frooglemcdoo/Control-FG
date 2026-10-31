#ifdef _WIN32
#include <windows.h>
#endif
#include "../../src/storage_startup_policy.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
int main(){
    using namespace storage_startup_policy;
    assert(Flag(nullptr,0,true)&&!Flag(nullptr,0,false));
    assert(Flag("1",1,false)&&!Flag("0",1,true));
    assert(!Flag("true",4,true)&&!Flag("11",2,true)&&!Flag("1",100,true));
    auto s=Resolve(0,true,16);assert(s.budget==4&&s.slices==16);
    s=Resolve(0,false,0);assert(s.budget==1&&s.slices==8);
    for(unsigned budget:{4u,6u,8u,10u,12u}){
        s=Resolve(budget,true,16);assert(s.budget==budget&&s.slices==16);
        s=Resolve(budget,false,0);assert(s.budget==budget&&s.slices==16);
        s=Resolve(budget,true,8);assert(s.budget==budget&&s.slices==8);
    }
    s=Resolve(99,true,16);assert(s.budget==4&&s.slices==16);
    s=Resolve(0,false,16);assert(s.budget==1&&s.slices==16);
#ifdef _WIN32
    constexpr auto key="CONTROLFG_TEST_STORAGE_STARTUP";
    SetEnvironmentVariableA(key,nullptr);
    assert(EnvironmentFlag(key,true)&&!EnvironmentFlag(key,false));
    SetEnvironmentVariableA(key,"0");assert(!EnvironmentFlag(key,true));
    SetEnvironmentVariableA(key,"1");assert(EnvironmentFlag(key,false));
    SetEnvironmentVariableA(key,"1111111111111111");assert(!EnvironmentFlag(key,true));
    SetEnvironmentVariableA(key,nullptr);
#endif
    puts("PASS R255 startup policy: defaults, explicit opt-out, malformed overrides, native fallback, saved budgets and pinned partition consistency");
}
