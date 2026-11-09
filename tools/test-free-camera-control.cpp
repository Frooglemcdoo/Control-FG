#include "../src/free_camera_control.h"
#include <cassert>
#include <cstdio>

int main() {
    using namespace control_free_camera;
    Commands commands;
    int nativeMode=0, calls=0;
    bool gameplay=true, inputOwned=false, changeWorks=true;
    auto service=[&] {
        commands.Service([&]{return nativeMode;},[&]{return gameplay;},[&](int target){
            ++calls;
            assert(commands.Pending()); // in-flight cannot accept another UI request
            assert(!commands.Request(0));
            assert(!commands.Timeout()); // timeout cannot interrupt an executing setter
            if(changeWorks){nativeMode=target;inputOwned=target==1;}
        },[&](int target){return inputOwned==(target==1);});
    };
    assert(!commands.Request(4));
    service();assert(calls==0 && commands.Mode()==0);
    const auto first=commands.Generation();
    assert(commands.Request(1));assert(!commands.Request(0));
    assert(nativeMode==0 && calls==0); // UI never mutates engine state
    service();assert(calls==1 && nativeMode==1 && inputOwned);
    assert(commands.Status()==Result::Applied && !commands.Pending());
    assert(commands.Generation()>first);
    service();assert(calls==1); // no repeated setter each frame
    assert(commands.Request(0));service();
    assert(calls==2 && nativeMode==0 && !inputOwned);
    inputOwned=true;assert(commands.Request(0));service();
    assert(calls==3 && !inputOwned); // normal request repairs input even if mode already 0
    gameplay=false;assert(commands.Request(1));service();
    assert(calls==3 && commands.Status()==Result::NotGameplay);
    gameplay=true;nativeMode=4;assert(commands.Request(1));service();
    assert(calls==3 && commands.Status()==Result::OtherMode);
    nativeMode=0;assert(commands.Request(1));assert(commands.Timeout());service();
    assert(calls==3 && commands.Status()==Result::TimedOut && !commands.Pending());
    changeWorks=false;assert(commands.Request(1));service();
    assert(calls==4 && commands.Status()==Result::VerificationFailed);
    service();assert(calls==4); // failed setter is not retried silently
    inputOwned=true;assert(commands.Request(0));service();
    assert(commands.Status()==Result::VerificationFailed); // mode alone is not sufficient
    const auto generation=commands.Generation();
    nativeMode=1;service();nativeMode=0;service();
    assert(commands.Generation()==generation+2); // detect external round trip during capture
    puts("PASS queued camera commands, timeout, gameplay/photo guards, native input restoration, verification and capture generation");
}
