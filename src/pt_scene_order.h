#pragma once
#include <cstdint>
#include <map>
#include <set>
#include <vector>
#include <algorithm>
#include <array>
// CPU model of OBSERVED queue operations, not a replacement for GPU fences.
// Hosts append command-list operations in submission/list order, never recording
// thread order. Queue/fence IDs must be lifetime identities, not reusable pointers.
namespace PTSceneOrder {
using U64=std::uint64_t;
enum class Status {ready,pending,missing,unordered,invalid};
struct Key {U64 token=0,address=0;bool operator<(const Key& b)const{return token<b.token||(token==b.token&&address<b.address);}};
struct Selection {U64 version=0,node=0;};
class Graph {
    using Clock=std::array<U64,17>;
    struct Node {U64 previous=0,waitFence=0,waitValue=0;Key target{};U64 version=0;unsigned lane=0;U64 stamp=0;Clock base{};};
    struct Fence {U64 last=0,completed=0;std::map<U64,U64> signals;};
    std::vector<Node> nodes{Node{}};
    std::map<U64,U64> queues;
    std::map<U64,Fence> fences;
    bool valid=true;U64 completedPrefix=0;
    std::map<U64,unsigned> lanes;
    U64 serial=0;Clock observedComplete{}; // Actual polled GPU completion, never a guessed queue edge.
    mutable std::vector<Clock> clocks;
    mutable std::vector<unsigned char> clockState;
    mutable bool clockDirty=true,clockCycle=false;
    Status ClockAt(U64 at)const {
        if(!valid||!at||at>=nodes.size())return Status::invalid;
        if(clockDirty){clocks.assign(nodes.size(),{});clockState.assign(nodes.size(),0);clockDirty=false;clockCycle=false;}
        if(clockCycle)return Status::invalid;
        if(clockState[at]>=2)return clockState[at]==2?Status::ready:Status::pending;
        std::vector<std::pair<U64,bool>> todo{{at,false}};
        while(!todo.empty()){
            const auto task=todo.back();todo.pop_back();const U64 id=task.first;
            if(!id||clockState[id]>=2)continue;
            const auto& n=nodes[id];U64 signal=0;
            if(n.waitFence){const auto f=fences.find(n.waitFence);
                if(f==fences.end()){clockState[id]=3;continue;}
                if(n.waitValue>f->second.completed){const auto found=f->second.signals.lower_bound(n.waitValue);
                    if(found==f->second.signals.end()){clockState[id]=3;continue;}signal=found->second;}
            }
            if(!task.second){
                if(clockState[id]==1){clockCycle=true;return Status::invalid;}
                clockState[id]=1;todo.push_back({id,true});
                if(signal)todo.push_back({signal,false});if(n.previous)todo.push_back({n.previous,false});continue;
            }
            if((n.previous&&clockState[n.previous]!=2)||(signal&&clockState[signal]!=2)){clockState[id]=3;continue;}
            auto& c=clocks[id];c=n.base;if(n.previous)for(unsigned lane=0;lane<c.size();++lane)c[lane]=std::max(c[lane],clocks[n.previous][lane]);
            if(signal)for(unsigned lane=0;lane<c.size();++lane)c[lane]=std::max(c[lane],clocks[signal][lane]);
            c[n.lane]=std::max(c[n.lane],n.stamp);clockState[id]=2;
        }
        return clockState[at]==2?Status::ready:Status::pending;
    }
    bool Before(U64 a,U64 b)const {return clocks[b][nodes[a].lane]>=nodes[a].stamp;}

    static constexpr std::size_t MaximumNodes=65536,MaximumQueues=16,MaximumFences=256;
    U64 Append(U64 queue,Node node){
        if(!valid||!queue||nodes.size()>=MaximumNodes||(!queues.count(queue)&&queues.size()>=MaximumQueues)){valid=false;return 0;}
        if(!lanes.count(queue))lanes.emplace(queue,static_cast<unsigned>(lanes.size()+1));
        node.lane=lanes.at(queue);node.stamp=++serial;node.base=observedComplete;clockDirty=true;
        node.previous=queues.count(queue)?queues.at(queue):completedPrefix;nodes.push_back(node);queues[queue]=nodes.size()-1;return nodes.size()-1;
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
                if(n.waitValue<=f->second.completed){if(n.previous)todo.push_back({n.previous,false});continue;}
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
        if(!fence||value==UINT64_MAX){valid=false;return 0;}
        if(!value)return Mark(queue);
        Node node;node.waitFence=fence;node.waitValue=value;return Append(queue,node);
    }
    // A CPU drain is allowed only when every dependency needed by already
    // queued work has a queued signal. Never block waiting for future CPU work.
    bool ClosedDependencies()const {
        if(!valid)return false;
        for(const auto& q:queues)if(ClockAt(q.second)!=Status::ready)return false;
        return true;
    }
    // Only after the host proves EVERY observed queue tail has completed,
    // with no submitted ticket still referring to a historical cut. Completion
    // is an explicit CPU happens-before edge for all subsequent queue calls.
    bool CompletedCheckpoint(std::map<Key,Selection>& latest){
        latest.clear();if(!valid)return false;
        std::map<Key,std::vector<U64>> writes;
        for(U64 n=1;n<nodes.size();++n)if(nodes[static_cast<std::size_t>(n)].target.token)writes[nodes[static_cast<std::size_t>(n)].target].push_back(n);
        std::map<Key,Selection> candidate;
        for(const auto& entry:writes){
            std::set<U64> maximal;
            for(const U64 n:entry.second){
                if(ClockAt(n)!=Status::ready)return false;
                bool dominated=false;
                for(auto it=maximal.begin();it!=maximal.end();){
                    if(Before(n,*it)){dominated=true;break;}
                    if(Before(*it,n))it=maximal.erase(it);else ++it;
                }
                if(!dominated)maximal.insert(n);
            }
            if(maximal.size()!=1)return false;
            const U64 n=*maximal.begin();candidate.emplace(entry.first,Selection{nodes[static_cast<std::size_t>(n)].version,n});
        }
        latest=std::move(candidate);return true;
    }
    // Latest versions have already been materialized by the catalog. Give all
    // existing AND future queues the same completed prefix, preserving fence
    // high-water marks so waits on old signals cannot become unresolved.
    bool RebaseCompleted(const std::map<Key,U64>& versions){
        if(!valid||versions.size()+2>=MaximumNodes)return false;
        std::vector<Node> baseline{Node{}};U64 tail=0;
        for(const auto& entry:versions){Node n;n.previous=tail;n.target=entry.first;n.version=entry.second;n.stamp=++serial;baseline.push_back(n);tail=baseline.size()-1;}
        if(!tail){Node n;n.stamp=++serial;baseline.push_back(n);tail=1;}
        nodes=std::move(baseline);completedPrefix=tail;clockDirty=true;
        for(auto& q:queues)q.second=tail;
        for(auto& f:fences){f.second.completed=f.second.last;f.second.signals.clear();}
        return true;
    }
    U64 TailStamp(U64 queue)const {const auto q=queues.find(queue);return q==queues.end()?0:nodes[q->second].stamp;}
    std::size_t Size()const{return nodes.size();}
    // Compact immutable CPU metadata without draining the GPU. Baked vector
    // clocks preserve EXACT ordering of existing operations. Only *future*
    // appends acquire edges from completion fences polled by the host.
    // External roots are active dispatch cuts and unresolved compaction cuts.
    bool Compact(const std::set<U64>& external,const std::map<U64,U64>& completed,
                 std::map<U64,U64>& remap,std::set<U64>& retainedVersions,const std::set<U64>* liveTokens=nullptr){
        remap.clear();retainedVersions.clear();if(!valid)return false;
        for(U64 id=1;id<nodes.size();++id)if(ClockAt(id)!=Status::ready)return false;
        // Preserve rejection of non-monotonic cross-queue fence ownership.
        for(const auto& f:fences){U64 previous=0;
            for(const auto& signal:f.second.signals){if(previous&&!Before(previous,signal.second))return false;previous=signal.second;}
        }
        Clock done=observedComplete;
        for(const auto& proof:completed){const auto lane=lanes.find(proof.first);if(lane==lanes.end())continue;
            for(U64 id=1;id<nodes.size();++id)if(nodes[id].lane==lane->second&&nodes[id].stamp<=proof.second)
                for(unsigned n=0;n<done.size();++n)done[n]=std::max(done[n],clocks[id][n]);
        }
        std::set<U64> roots=external;
        for(const auto& q:queues)roots.insert(q.second);
        auto newFences=fences;
        for(auto& f:newFences){
            auto& signals=f.second.signals;
            for(auto it=signals.begin();it!=signals.end();){const auto& node=nodes[it->second];
                if(done[node.lane]>=node.stamp){f.second.completed=std::max(f.second.completed,it->first);it=signals.erase(it);}
                else {roots.insert(it->second);++it;}
            }
        }
        roots.erase(0);for(const U64 id:roots)if(id>=nodes.size())return false;
        // For each key/lane keep its last write at every root, plus its latest
        // write. The latter preserves concurrent-write rejection even when no
        // current dispatch can reach that lane. Tombstones remain writes.
        std::array<std::set<U64>,17> thresholds;
        for(const U64 root:roots)for(unsigned lane=0;lane<thresholds.size();++lane)thresholds[lane].insert(clocks[root][lane]);
        std::map<Key,std::map<unsigned,std::vector<U64>>> writes;
        for(U64 id=1;id<nodes.size();++id)if(nodes[id].target.token)writes[nodes[id].target][nodes[id].lane].push_back(id);
        // Dead allocation generations cannot be selected by a future dispatch.
        // Keep them while any historical consumer exists or any write is still
        // in flight. Queue/fence root nodes survive with their baked clocks.
        std::set<Key> retired;
        if(liveTokens&&external.empty())for(const auto& key:writes){
            if(liveTokens->count(key.first.token))continue;
            bool finished=true;
            for(const auto& lane:key.second)for(const U64 id:lane.second)
                if(done[nodes[id].lane]<nodes[id].stamp)finished=false;
            if(finished)retired.insert(key.first);
        }
        std::set<U64> keep=roots;
        for(const auto& key:writes)for(const auto& lane:key.second){
            if(retired.count(key.first))continue;
            const auto& sequence=lane.second;keep.insert(sequence.back());
            // Invert the query: a write is a predecessor/successor iff a
            // root threshold lies in its neighbouring stamp interval. Static
            // geometry now costs O(writes log roots), not O(keys * roots).
            const auto& points=thresholds[lane.first];
            for(std::size_t i=0;i<sequence.size();++i){
                const U64 stamp=nodes[sequence[i]].stamp;
                const auto predecessor=points.lower_bound(stamp);
                const auto successor=points.lower_bound(i?nodes[sequence[i-1]].stamp:0);
                if((predecessor!=points.end()&&(i+1==sequence.size()||*predecessor<nodes[sequence[i+1]].stamp))||
                   (successor!=points.end()&&*successor<stamp))keep.insert(sequence[i]);
            }
        }
        std::vector<Node> compact{Node{}};
        for(const U64 id:keep){auto node=nodes[id];node.previous=node.waitFence=node.waitValue=0;node.base=clocks[id];
            if(retired.count(node.target)){node.target={};node.version=0;}
            remap.emplace(id,compact.size());compact.push_back(node);if(node.target.token&&node.version)retainedVersions.insert(node.version);
        }
        for(auto& q:queues)q.second=remap.at(q.second);
        for(auto& f:newFences)for(auto& signal:f.second.signals)signal.second=remap.at(signal.second);
        // Existing nodes have baked clocks; no rewritten dependency can acquire
        // a future signal or lose the original cross-queue ordering.
        nodes=std::move(compact);fences=std::move(newFences);observedComplete=done;completedPrefix=0;clockDirty=true;
        return true;
    }
    Status Select(U64 dispatch,Key target,Selection& result)const {
        result={};const auto state=ClockAt(dispatch);if(state!=Status::ready)return state;
        for(U64 id=1;id<nodes.size();++id){const auto& n=nodes[static_cast<std::size_t>(id)];
            if(n.target.token!=target.token||n.target.address!=target.address||Before(id,dispatch))continue;
            const auto status=ClockAt(id);if(status!=Status::ready)return status;
            if(!Before(dispatch,id))return Status::unordered;
        }
        // Increasing CPU fence values alone do not prove a queue handoff.
        for(const auto& fence:fences){U64 previous=0;
            for(const auto& signal:fence.second.signals){
                if(previous){const auto status=ClockAt(signal.second);if(status!=Status::ready)return status;
                    if(!Before(previous,signal.second))return Status::unordered;}
                previous=signal.second;
            }
        }
        std::set<U64> maximal;
        for(U64 id=1;id<nodes.size();++id){const auto& n=nodes[static_cast<std::size_t>(id)];
            if(n.target.token!=target.token||n.target.address!=target.address||!Before(id,dispatch))continue;
            const auto status=ClockAt(id);if(status!=Status::ready)return status;
            bool dominated=false;
            for(auto it=maximal.begin();it!=maximal.end();){
                if(Before(id,*it)){dominated=true;break;}
                if(Before(*it,id))it=maximal.erase(it);else ++it;
            }
            if(!dominated)maximal.insert(id);
        }
        if(maximal.empty())return Status::missing;
        if(maximal.size()!=1)return Status::unordered;
        const U64 selected=*maximal.begin();
        if(!nodes[selected].version)return Status::missing;
        result={nodes[selected].version,selected};return Status::ready;
    }
};
}
