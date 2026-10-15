#include "../src/pt_surface_table_budget.h"
#include <array>
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <random>
int main(){
    using Budget=PTSurfaceComposition::TableBudget;
    // Same 200 eligible draws, changing native worker order. The old 64-table
    // policy selects a different subset each frame; every draw must now route.
    std::array<unsigned,200> order{};for(unsigned i=0;i<order.size();++i)order[i]=i;
    std::mt19937 rng(17);
    for(unsigned frame=0;frame<40;++frame){
        std::shuffle(order.begin(),order.end(),rng);Budget budget;
        std::array<bool,200> covered{};
        for(auto draw:order){std::uint32_t offset=~0u;
            assert(budget.Take(Budget::Descriptors,offset));assert(offset+Budget::Width<=Budget::Descriptors);
            covered[draw]=true;
        }
        assert(std::all_of(covered.begin(),covered.end(),[](bool b){return b;}));
    }
    Budget full;for(unsigned i=0;i<Budget::Capacity;++i){std::uint32_t offset=~0u;
        assert(full.Take(Budget::Descriptors,offset));assert(offset==i*Budget::Width);}
    std::uint32_t sentinel=~0u;assert(!full.Take(Budget::Descriptors,sentinel));
    assert(full.Exhausted()&&full.Used()==Budget::Capacity&&sentinel==~0u);
    assert(!full.Take(Budget::Descriptors,sentinel)); // never wrap/reuse
    Budget shortRange;assert(!shortRange.Take(64*Budget::Width,sentinel));
    assert(shortRange.Exhausted()&&shortRange.Used()==0&&sentinel==~0u);
    puts("PASS: worker-order-independent coverage beyond 64 draws, bounded unique slots, exhaustion and short-reservation rejection");
}
