#pragma once
#include <array>
#include <cstdint>
#include <mutex>
namespace PTNativeJobRoute {
using U64=std::uint64_t;
struct Writer {
    U64 buffer=0,state=0,bindings=0;
    int bytes=0,rays=0,count=0;
    bool operator==(const Writer& b)const noexcept {
        return buffer==b.buffer&&state==b.state&&bindings==b.bindings&&bytes==b.bytes&&rays==b.rays&&count==b.count;
    }
};
struct TableLayout {
    U64 buffer=0,bytes=0;
    std::uint32_t raygenBytes=0,missOffset=0,missStride=0,hitRelative=0,hitStride=0,hitRecords=0;
};
struct Ticket {U64 serial=0,list=0,pipeline=0,counter=0,hitTable=0,hitBytes=0;std::uint32_t width=0,height=0;Writer writer{};TableLayout layout{};};
// Native job slots are reused. A slot number alone is never a lifetime token.
// The live host must register BEFORE runJob and finish BEFORE the worker returns.
class Router {
    struct Slot {Ticket ticket{};bool writing=false;};
    std::mutex mutex;std::array<Slot,16> slots{};
public:
    bool Schedule(unsigned slot,const Ticket& ticket){
        std::lock_guard<std::mutex> guard(mutex);
        if(slot>=slots.size()||slots[slot].ticket.serial||!ticket.serial||!ticket.list||!ticket.pipeline||!ticket.counter||
           !ticket.writer.buffer||!ticket.writer.state||!ticket.writer.bindings||ticket.writer.bytes<=0||ticket.writer.rays!=2||ticket.writer.count<=0)return false;
        slots[slot].ticket=ticket;return true;
    }
    bool Begin(unsigned slot,const Writer& writer,Ticket& out){
        out={};std::lock_guard<std::mutex> guard(mutex);
        if(slot>=slots.size()||!slots[slot].ticket.serial||slots[slot].writing||!(slots[slot].ticket.writer==writer))return false;
        slots[slot].writing=true;out=slots[slot].ticket;return true;
    }
    bool Finish(unsigned slot,U64 serial){
        std::lock_guard<std::mutex> guard(mutex);
        if(slot>=slots.size()||!serial||slots[slot].ticket.serial!=serial||!slots[slot].writing)return false;
        slots[slot]={};return true;
    }
};
}
