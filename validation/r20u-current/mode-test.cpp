#include <cassert>
#include <cstdint>
#include <iostream>
#include "../../src/rr_contact_shadow_policy.h"

int main(){
    // OFF and PARTIAL/pass-through style states must preserve Control's native filter args.
    for(bool contact: {false,true}){
        for(bool temporal: {false,true}){
            for(int size: {0,1,3,5,9}){
                for(int step: {1,2,4}){
                    auto p=control_rr::ContactShadowFilterPolicy(false,contact,temporal,size,step);
                    assert(!p.neutralized);
                    assert(p.temporal==temporal && p.spatialSize==size && p.spatialStep==step);
                }
            }
        }
    }
    // FULL RR without RT contact shadows remains pass-through.
    {
        auto p=control_rr::ContactShadowFilterPolicy(true,false,true,7,3);
        assert(!p.neutralized && p.temporal && p.spatialSize==7 && p.spatialStep==3);
    }
    // FULL RR + RT contact shadows: temporal accumulation off, radius zero,
    // non-zero step retained for the native identity-style output pass.
    for(bool temporal: {false,true}){
        for(int size: {-1,0,1,3,11}){
            for(int step: {0,1,2,8}){
                auto p=control_rr::ContactShadowFilterPolicy(true,true,temporal,size,step);
                assert(p.neutralized);
                assert(!p.temporal);
                assert(p.spatialSize==0);
                assert(p.spatialStep==1);
            }
        }
    }
    std::cout << "PASS r20u contact-shadow policy: FULL+contact neutralizes temporal/spatial denoising; OFF/PARTIAL/non-contact pass through\n";
    return 0;
}
