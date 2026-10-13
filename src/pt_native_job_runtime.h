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
static std::atomic<bool> workerHooksReady{false},dispatchHookReady{false};
static std::atomic<UINT64> serial{1};
static PTNativeJobRoute::Router router;
static thread_local Ticket pending;
enum class RouteFailure : unsigned {none,input,frame,worker,state,buffers,counter,pipeline,layout,raygen,miss,hit,shape,frameChanged,exception,count};
static std::atomic<UINT64> routes[static_cast<unsigned>(RouteFailure::count)]{};
static std::atomic<UINT64> scheduled{0},scheduleRejected{0},writerFinished{0};
template<class F> static void Diagnostics(F&& log){
    log("PT_PRIMARY_NATIVE_ROUTE accepted=%llu input=%llu frame=%llu worker=%llu state=%llu buffers=%llu counter=%llu pipeline=%llu layout=%llu raygen=%llu miss=%llu hit=%llu shape=%llu frame_changed=%llu exception=%llu scheduled=%llu schedule_rejected=%llu writer_finished=%llu",
        routes[0].load(),routes[1].load(),routes[2].load(),routes[3].load(),routes[4].load(),routes[5].load(),routes[6].load(),routes[7].load(),routes[8].load(),routes[9].load(),routes[10].load(),routes[11].load(),routes[12].load(),routes[13].load(),routes[14].load(),scheduled.load(),scheduleRejected.load(),writerFinished.load());
}
// Consumer is a session-lifetime static callback. Register before the first
// dispatch; do not replace it while jobs are pending. It must copy writer input
// data / acquire ownership inside writerReady, before the native job returns.
[[maybe_unused]] static bool Register(Consumer fn)noexcept {
    Consumer empty=nullptr;
    return workerHooksReady.load(std::memory_order_acquire)&&dispatchHookReady.load(std::memory_order_acquire)&&fn&&consumer.compare_exchange_strong(empty,fn);
}
static bool ReadDispatch(ID3D12GraphicsCommandList4* list,const D3D12_DISPATCH_RAYS_DESC* source,ID3D12StateObject* pipeline,Ticket& out,RouteFailure& failure)noexcept {
    out={};failure=RouteFailure::input;
    __try {
        if(!verifiedD3d||!list||!source||!pipeline)return false;
        auto* module=reinterpret_cast<unsigned char*>(verifiedD3d);
        // The SBT counter advances at each native ray dispatch (RVA 3570f),
        // not once per frame. Use the established engine frame counter also
        // consumed by the camera/RR capture paths; freeze it before scheduling.
        auto* frameAddress=reinterpret_cast<volatile const UINT64*>(module+0x136c68);
        failure=RouteFailure::frame;const auto frame=*frameAddress;if(!frame)return false;
        failure=RouteFailure::worker;
        if(!module[0xf6c30]||module[0x1371e0])return false; // Worker path, real identifiers.
        failure=RouteFailure::state;auto* state=*reinterpret_cast<unsigned char**>(module+0x136d28);if(!state)return false;
        failure=RouteFailure::buffers;auto* buffers=*reinterpret_cast<unsigned char**>(state+0x220);if(!buffers)return false;
        failure=RouteFailure::counter;auto* counter=*reinterpret_cast<UINT64**>(buffers+0xa8);if(!counter||!*counter)return false;
        auto* buffer=buffers+0x50*(*counter&1);
        auto* shader=*reinterpret_cast<unsigned char**>(state+0x240);
        failure=RouteFailure::pipeline;if(!shader||*reinterpret_cast<ID3D12StateObject**>(shader)!=pipeline)return false;
        const auto desc=*source;
        PTNativeShaderTable::Layout layout{};
        layout.buffer=*reinterpret_cast<UINT64*>(buffer+0x48);layout.bytes=*reinterpret_cast<UINT64*>(buffer+0x30);
        layout.raygenBytes=*reinterpret_cast<UINT*>(state+0x200);layout.missOffset=*reinterpret_cast<UINT*>(state+0x204);
        layout.missStride=*reinterpret_cast<UINT*>(state+0x208);layout.hitRelative=*reinterpret_cast<UINT*>(state+0x20c);
        layout.hitStride=*reinterpret_cast<UINT*>(state+0x210);layout.hitRecords=*reinterpret_cast<UINT*>(state+0x218);
        const int bytes=*reinterpret_cast<int*>(state+0x228),count=*reinterpret_cast<int*>(state+0x22c);
        const auto bindings=*reinterpret_cast<UINT64*>(state+0x230);
        UINT64 hit=0,size=0;
        failure=RouteFailure::layout;
        if(bytes<=0||UINT64(bytes)>layout.bytes||count<=0||!bindings||!PTNativeShaderTable::HitRange(layout,static_cast<std::size_t>(count),hit,size))return false;
        failure=RouteFailure::raygen;
        if(desc.RayGenerationShaderRecord.StartAddress!=layout.buffer||desc.RayGenerationShaderRecord.SizeInBytes!=32)return false;
        failure=RouteFailure::miss;
        if(desc.MissShaderTable.StartAddress!=layout.buffer+layout.missOffset||desc.MissShaderTable.SizeInBytes!=64||desc.MissShaderTable.StrideInBytes!=32)return false;
        failure=RouteFailure::hit;
        if(desc.HitGroupTable.StartAddress!=hit||desc.HitGroupTable.SizeInBytes!=size||desc.HitGroupTable.StrideInBytes!=96)return false;
        failure=RouteFailure::shape;if(!desc.Width||!desc.Height||desc.Depth!=1)return false;
        failure=RouteFailure::frameChanged;if(*frameAddress!=frame)return false;
        out.engineFrame=frame;out.list=reinterpret_cast<UINT_PTR>(list);out.pipeline=reinterpret_cast<UINT_PTR>(pipeline);out.counter=*counter;
        out.hitTable=hit;out.hitBytes=size;out.width=desc.Width;out.height=desc.Height;out.layout=layout;
        out.writer={reinterpret_cast<UINT_PTR>(buffer),reinterpret_cast<UINT_PTR>(shader),bindings,bytes,2,count};failure=RouteFailure::none;return true;
    }__except(EXCEPTION_EXECUTE_HANDLER){out={};failure=RouteFailure::exception;return false;}
}
static void Recorded(ID3D12GraphicsCommandList4* list,const D3D12_DISPATCH_RAYS_DESC* desc,ID3D12StateObject* pipeline)noexcept {
    const auto callback=consumer.load(std::memory_order_acquire);if(!callback)return;
    const DWORD error=GetLastError();
    if(pending.serial)callback(Phase::cancelled,pending);pending={};
    RouteFailure failure{};const bool read=ReadDispatch(list,desc,pipeline,pending,failure);++routes[static_cast<unsigned>(failure)];
    if(read){
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
    if(pending.serial){
        if(!ReadJob(handle,slot,writer)||!(writer==pending.writer)||!router.Schedule(slot,pending)){++scheduleRejected;callback(Phase::cancelled,pending);}
        else ++scheduled;
    }
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
        ++writerFinished;
        if(const auto callback=consumer.load(std::memory_order_acquire))callback(Phase::writerReady,ticket);
    }
    SetLastError(error);
}
}
