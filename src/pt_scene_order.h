#pragma once
#include <cstdint>
#include <map>
#include <set>
#include <vector>
#include <algorithm>
// CPU model of OBSERVED queue operations, not a replacement for GPU fences.
// Hosts append command-list operations in submission/list order, never recording
// thread order. Queue/fence IDs must be lifetime identities, not reusable pointers.
namespace PTSceneOrder {
using U64=std::uint64_t;
enum class Status {ready,pending,missing,unordered,invalid};
struct Key {U64 token=0,address=0;bool operator<(const Key& b)const{return token<b.token||(token==b.token&&address<b.address);}};
struct Selection {U64 version=0,node=0;};
class Graph {
    struct Node {U64 previous=0,waitFence=0,waitValue=0;Key target{};U64 version=0;};
    struct Fence {U64 last=0;std::map<U64,U64> signals;};
    std::vector<Node> nodes{Node{}};
    std::map<U64,U64> queues;
    std::map<U64,Fence> fences;
    bool valid=true;
    static constexpr std::size_t MaximumNodes=65536,MaximumQueues=16,MaximumFences=256;
    U64 Append(U64 queue,Node node){
        if(!valid||!queue||nodes.size()>=MaximumNodes||(!queues.count(queue)&&queues.size()>=MaximumQueues)){valid=false;return 0;}
        node.previous=queues[queue];nodes.push_back(node);queues[queue]=nodes.size()-1;return nodes.size()-1;
    }
    Status Ancestors(U64 at,std::set<U64>& complete)const {
        complete.clear();if(!valid||!at||at>=nodes.size())return Status::invalid;
        // Iterative DFS avoids recursion depth depending on runtime duration.
        std::vector<std::pair<U64,bool>> todo{{at,false}};std::set<U64> visiting;
        while(!todo.empty()){
            const auto item=todo.back();todo.pop_back();const U64 id=item.first;
            if(!id||complete.count(id))continue;
            if(item.second){visiting.erase(id);complete.insert(id);continue;}
            if(!visiting.insert(id).second)return Status::invalid; // Cyclic waits cannot establish ordering.
            todo.push_back({id,true});const auto& n=nodes[id];
            if(n.waitFence){
                const auto f=fences.find(n.waitFence);
                if(f==fences.end())return Status::pending;
                const auto signal=f->second.signals.lower_bound(n.waitValue);
                if(signal==f->second.signals.end())return Status::pending;
                todo.push_back({signal->second,false});
            }
            if(n.previous)todo.push_back({n.previous,false});
        }
        return Status::ready;
    }
public:
    // Zero version records a known write with unknown/unadmitted metadata. It
    // must invalidate an older snapshot rather than making that old one usable.
    U64 Write(U64 queue,Key target,U64 version){
        if(!target.token||!target.address){valid=false;return 0;}
        Node n;n.target=target;n.version=version;return Append(queue,n);
    }
    U64 Mark(U64 queue){return Append(queue,{});}
    U64 Signal(U64 queue,U64 fence,U64 value){
        if(!valid||!fence||!value||value==UINT64_MAX||(!fences.count(fence)&&fences.size()>=MaximumFences)){valid=false;return 0;}
        auto& f=fences[fence];if(value<=f.last){valid=false;return 0;}
        const auto node=Append(queue,{});if(!node)return 0;
        f.last=value;f.signals.emplace(value,node);return node;
    }
    U64 Wait(U64 queue,U64 fence,U64 value){
        if(!fence||!value||value==UINT64_MAX){valid=false;return 0;}
        Node node;node.waitFence=fence;node.waitValue=value;return Append(queue,node);
    }
    Status Select(U64 dispatch,Key target,Selection& result)const {
        result={};std::set<U64> reachable;const auto state=Ancestors(dispatch,reachable);
        if(state!=Status::ready)return state;
        // A larger signal on another queue can satisfy the same wait first.
        // Require a single ordered timeline for every observed fence, including
        // queue handoffs; increasing CPU-side values alone are insufficient.
        for(const auto& fence:fences){
            U64 previous=0;
            for(const auto& signal:fence.second.signals){
                if(previous){
                    std::set<U64> history;const auto s=Ancestors(signal.second,history);
                    if(s!=Status::ready)return s;
                    if(!history.count(previous))return Status::unordered;
                }
                previous=signal.second;
            }
        }
        std::set<U64> maximal;
        for(const auto id:reachable){const auto& n=nodes[id];
            if(n.target.token==target.token&&n.target.address==target.address)maximal.insert(id);
        }
        if(maximal.empty())return Status::missing;
        const auto candidates=maximal;
        for(const auto candidate:candidates){
            std::set<U64> prior;const auto s=Ancestors(candidate,prior);if(s!=Status::ready)return s;
            for(const auto ancestor:prior)if(ancestor!=candidate)maximal.erase(ancestor);
        }
        if(maximal.size()!=1)return Status::unordered;
        const U64 selected=*maximal.begin();
        if(!nodes[selected].version)return Status::missing;
        result={nodes[selected].version,selected};return Status::ready;
    }
};
}
