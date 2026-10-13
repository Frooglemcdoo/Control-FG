#include "../src/pt_primary_profiles.h"
#include <cassert>
#include <cstdio>
int main(){
    for(const auto& p:PTPrimaryProfiles::profiles){
        assert(PTPrimaryProfiles::CandidateSize(p.bytes));
        assert(PTPrimaryProfiles::Find(p.bytes,p.sha256)==&p);
        assert(!PTPrimaryProfiles::Find(p.bytes-1,p.sha256));
        char changed[65]{};std::memcpy(changed,p.sha256,65);
        changed[0]=changed[0]=='0'?'1':'0';
        assert(!PTPrimaryProfiles::Find(p.bytes,changed));
    }
    assert(!PTPrimaryProfiles::CandidateSize(0));
    assert(!PTPrimaryProfiles::Find(69704,nullptr));
    std::puts("PASS: reviewed primary variants admitted; wrong hashes and sizes rejected.");
}
