#pragma once
// Caller serializes access. No COM references or resource-state changes.
namespace control_fg_source {
struct Submission { unsigned long long generation=0;unsigned index=~0u; };
class Tracker {
    struct Pending {const void* list=nullptr;unsigned index=~0u;};
    const void* owner_=nullptr;
    const void* buffers_[8]{};
    Pending pending_[256]{};
    unsigned long long generation_=1;
    unsigned latest_=~0u;
    bool overflow_=false;
public:
    void Clear(const void* owner) noexcept {
        if(owner_!=owner)return;
        owner_=nullptr;for(auto& b:buffers_)b=nullptr;
        for(auto& p:pending_)p={};
        ++generation_;latest_=~0u;overflow_=false;
    }
    bool Register(const void* owner,const void* buffer,unsigned index) noexcept {
        if(!owner||!buffer||index>=8||(owner_&&owner_!=owner))return false;
        owner_=owner;buffers_[index]=buffer;return true;
    }
    void Reset(const void* list) noexcept {for(auto& p:pending_)if(p.list==list)p={};}
    void Record(const void* list,const void* destination) noexcept {
        if(!owner_||overflow_)return;
        unsigned index=~0u;for(unsigned i=0;i<8;++i)if(buffers_[i]&&buffers_[i]==destination)index=i;
        if(index==~0u)return;
        Pending* empty=nullptr;
        for(auto& p:pending_){if(p.list==list){p.index=index;return;}if(!p.list&&!empty)empty=&p;}
        if(empty){*empty={list,index};return;}
        overflow_=true;latest_=~0u;
    }
    Submission Submitted(const void* list) noexcept {
        if(overflow_)return {};
        for(auto& p:pending_)if(p.list==list){Submission result{generation_,p.index};p={};return result;}
        return {};
    }
    void Commit(Submission submission,bool sameQueue) noexcept {
        if(submission.generation!=generation_||submission.index==~0u||overflow_)return;
        latest_=sameQueue?submission.index:~0u;
    }
    unsigned Take(const void* owner,unsigned fallback,bool& observed) noexcept {
        observed=owner==owner_&&!overflow_&&latest_!=~0u;
        const unsigned result=observed?latest_:fallback;
        if(owner==owner_)latest_=~0u;
        return result;
    }
};
}
