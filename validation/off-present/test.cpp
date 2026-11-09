#include "../../src/fg_off_present_proof.h"
#include <cassert>
#include <cstdio>
int main() {
    using control_fg_off_present::Proof;
    Proof proof;int a=0,b=0;
    assert(proof.Take(&a)==0); // No successful Off presentation yet.
    proof.Observe(&a,10,true);assert(proof.Take(&a)==10);
    assert(proof.Take(&a)==0); // One boundary cannot be reused twice.
    proof.Observe(&a,11,true);proof.Invalidate();assert(proof.Take(&a)==0); // Enable attempt retires proof.
    proof.Observe(&a,12,true);assert(proof.Take(&b)==0);assert(proof.Take(&a)==0);
    proof.Observe(&a,13,true);proof.Observe(&a,14,false);assert(proof.Take(&a)==0); // Failed/occluded/FG-on.
    proof.Observe(nullptr,15,true);assert(proof.Take(&a)==0);
    proof.Observe(&a,0,true);assert(proof.Take(&a)==0);
    // Five menu transitions: normal Off frames permit reset without extra rotation.
    unsigned injected=0;
    for(unsigned transition=0;transition<5;++transition){
        proof.Invalidate(); // Gameplay enabled FG.
        const unsigned frame=100+transition*100;
        proof.Observe(&a,frame,true);proof.Observe(&a,frame+1,true);
        if(!proof.Take(&a))++injected;
    }
    assert(injected==0);
    // A live transition before Off is committed still takes the original path.
    proof.Invalidate();if(!proof.Take(&a))++injected;assert(injected==1);
    std::puts("PASS off-present proof lifecycle, five menu transitions, live-transition fallback");
}
