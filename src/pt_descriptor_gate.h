#pragma once
#include <shared_mutex>
#include <cassert>
namespace PTTableMirror {
// One process-wide descriptor gate. Snapshot/metadata transactions recurse
// exclusively; proven independent copies take shared ownership. A shared
// transaction must finish before invoking any observer requiring exclusivity.
class DescriptorGate {
    std::shared_mutex mutex;
    inline static thread_local unsigned exclusiveDepth=0,sharedDepth=0;
public:
    void lock(){
        if(exclusiveDepth){++exclusiveDepth;return;}
        assert(!sharedDepth); // No shared-to-exclusive upgrade is supported.
        mutex.lock();exclusiveDepth=1;
    }
    void unlock(){assert(exclusiveDepth);if(!--exclusiveDepth)mutex.unlock();}
    void lock_shared(){
        if(!exclusiveDepth&&!sharedDepth)mutex.lock_shared();
        ++sharedDepth;
    }
    void unlock_shared(){assert(sharedDepth);if(!--sharedDepth&&!exclusiveDepth)mutex.unlock_shared();}
};
}
