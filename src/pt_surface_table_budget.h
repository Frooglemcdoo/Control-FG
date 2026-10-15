#pragma once
#include <cstdint>
// A frame reserves one contiguous native-allocator range. Never recycle a slot
// across recorded draws or wrap onto a GPU-referenced table.
namespace PTSurfaceComposition {
class TableBudget {
    std::uint32_t used=0;
    bool exhausted=false;
public:
    static constexpr std::uint32_t Width=49,Capacity=1024,Descriptors=Width*Capacity;
    bool Take(std::uint32_t reserved,std::uint32_t& offset){
        if(exhausted||reserved!=Descriptors||used>=Capacity){exhausted=true;return false;}
        offset=Width*used++;return true;
    }
    std::uint32_t Used()const{return used;}
    bool Exhausted()const{return exhausted;}
};
}
