#pragma once
#include "pt_native_job_route.h"
#include "pt_native_shader_table.h"
#include "pt_native_buffer_observer.h"
#include <atomic>
// Exact Steam native dispatch -> scheduled job -> writer handoff. Inert until a
// renderer registers a consumer. No new dispatch, GPU mutation, or capture file.
namespace PTNativeJobRuntime {
using PTNativeJobRoute::Ticket;using PTNativeJobRoute::Writer;
enum class Phase {recorded,writerReady,cancelled};
using Consumer=void(*)(Phase,const Ticket&) noexcept;
static std::atomic<Consumer> consumer{nullptr};
static std::atomic<UINT64> serial{1};
static PTNativeJobRoute::Router router;
static thread_local Ticket pending;
// Consumer is a session-lifetime static callback. Register before the first
// dispatch; do not replace it while jobs are pending. It must copy writer input
// data / acquire ownership inside writerReady, before the native job returns.
[[maybe_unused]] static bool Register(Consumer fn)noexcept {Consumer empty=nullptr;return fn&&consumer.compare_exchange_strong(empty,fn);}
static bool ReadDispatch(ID3D12GraphicsCommandList4* list,const D3D12_DISPATCH_RAYS_DESC* source,ID3D12StateObject* pipeline,Ticket& out)noexcept {
    out={};
    __try {
        if(!verifiedD3d||!list||!source||!pipeline)return false;
        auto* module=reinterpret_cast<unsigned char*>(verifiedD3d);
        if(!module[0xf6c30]||module[0x1371e0])return false; // Worker path, real identifiers.
        auto* state=*reinterpret_cast<unsigned char**>(module+0x136d28);if(!state)return false;
        auto* buffers=*reinterpret_cast<unsigned char**>(state+0x220);if(!buffers)return false;
        auto* counter=*reinterpret_cast<UINT64**>(buffers+0xa8);if(!counter||!*counter)return false;
        auto* buffer=buffers+0x50*(*counter&1);
        auto* shader=*reinterpret_cast<unsigned char**>(state+0x240);
        if(!shader||*reinterpret_cast<ID3D12StateObject**>(shader)!=pipeline)return false;
        const auto desc=*source;
        PTNativeShaderTable::Layout layout{};
        layout.buffer=*reinterpret_cast<UINT64*>(buffer+0x48);layout.bytes=*reinterpret_cast<UINT64*>(buffer+0x30);
        layout.raygenBytes=*reinterpret_cast<UINT*>(state+0x200);layout.missOffset=*reinterpret_cast<UINT*>(state+0x204);
        layout.missStride=*reinterpret_cast<UINT*>(state+0x208);layout.hitRelative=*reinterpret_cast<UINT*>(state+0x20c);
        layout.hitStride=*reinterpret_cast<UINT*>(state+0x210);layout.hitRecords=*reinterpret_cast<UINT*>(state+0x218);
        const int bytes=*reinterpret_cast<int*>(state+0x228),count=*reinterpret_cast<int*>(state+0x22c);
        const auto bindings=*reinterpret_cast<UINT64*>(state+0x230);
        UINT64 hit=0,size=0;
        if(bytes<=0||UINT64(bytes)>layout.bytes||count<=0||!bindings||!PTNativeShaderTable::HitRange(layout,static_cast<std::size_t>(count),hit,size)||
           desc.RayGenerationShaderRecord.StartAddress!=layout.buffer||desc.RayGenerationShaderRecord.SizeInBytes!=32||
           desc.MissShaderTable.StartAddress!=layout.buffer+layout.missOffset||desc.MissShaderTable.SizeInBytes!=64||desc.MissShaderTable.StrideInBytes!=32||
           desc.HitGroupTable.StartAddress!=hit||desc.HitGroupTable.SizeInBytes!=size||desc.HitGroupTable.StrideInBytes!=96||
           !desc.Width||!desc.Height||desc.Depth!=1)return false;
        out.list=reinterpret_cast<UINT_PTR>(list);out.pipeline=reinterpret_cast<UINT_PTR>(pipeline);out.counter=*counter;
        out.hitTable=hit;out.hitBytes=size;out.width=desc.Width;out.height=desc.Height;
        out.writer={reinterpret_cast<UINT_PTR>(buffer),reinterpret_cast<UINT_PTR>(shader),bindings,bytes,2,count};return true;
    }__except(EXCEPTION_EXECUTE_HANDLER){out={};return false;}
}
static void Recorded(ID3D12GraphicsCommandList4* list,const D3D12_DISPATCH_RAYS_DESC* desc,ID3D12StateObject* pipeline)noexcept {
    const auto callback=consumer.load(std::memory_order_acquire);if(!callback)return;
    const DWORD error=GetLastError();
    if(pending.serial)callback(Phase::cancelled,pending);pending={};
    if(ReadDispatch(list,desc,pipeline,pending)){
        pending.serial=serial.fetch_add(1);if(pending.serial)callback(Phase::recorded,pending);
    }
    SetLastError(error);
}
static bool ReadJob(void* handle,unsigned& slot,Writer& writer)noexcept {
    __try {
        if(!verifiedD3d)return false;
        auto* base=reinterpret_cast<unsigned char*>(verifiedD3d)+0x136d30;
        const auto address=reinterpret_cast<UINT_PTR>(handle),begin=reinterpret_cast<UINT_PTR>(base);
        if(address<begin||address-begin>=16*64||(address-begin)%64)return false;
        slot=static_cast<unsigned>((address-begin)/64);auto* job=static_cast<unsigned char*>(handle);
        writer={*reinterpret_cast<UINT64*>(job+0x10),*reinterpret_cast<UINT64*>(job+0x18),*reinterpret_cast<UINT64*>(job+0x30),
            *reinterpret_cast<int*>(job+0x20),*reinterpret_cast<int*>(job+0x24),*reinterpret_cast<int*>(job+0x28)};
        return true;
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
static void Scheduling(void* handle)noexcept {
    const auto callback=consumer.load(std::memory_order_acquire);if(!callback)return;
    const DWORD error=GetLastError();unsigned slot=0;Writer writer{};
    if(pending.serial&&(!ReadJob(handle,slot,writer)||!(writer==pending.writer)||!router.Schedule(slot,pending)))callback(Phase::cancelled,pending);
    pending={};SetLastError(error);
}
static Ticket Begin(int mode,const Writer& writer)noexcept {
    Ticket ticket{};if(!consumer.load(std::memory_order_acquire)||mode<0)return ticket;
    const DWORD error=GetLastError();router.Begin(static_cast<unsigned>(mode),writer,ticket);SetLastError(error);return ticket;
}
static void Finish(int mode,const Ticket& ticket)noexcept {
    if(!ticket.serial)return;
    const DWORD error=GetLastError();
    // Publish while the job is still running. The native waitJob cannot return
    // and release its copy work until this callback and the native worker return.
    if(router.Finish(static_cast<unsigned>(mode),ticket.serial)){
        if(const auto callback=consumer.load(std::memory_order_acquire))callback(Phase::writerReady,ticket);
    }
    SetLastError(error);
}
}
