#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <vector>
#include <cassert>
#include <cstdio>
static HMODULE verifiedD3d=nullptr;
#include "../src/pt_native_job_runtime.h"
using namespace PTNativeJobRuntime;
static unsigned recorded=0,ready=0,cancelled=0;
static Ticket received;
static void Receive(Phase phase,const Ticket& ticket)noexcept {
    if(phase==Phase::recorded)++recorded;
    if(phase==Phase::writerReady)++ready;
    if(phase==Phase::cancelled)++cancelled;
    received=ticket;
}
template<class T>static void Put(unsigned char* at,T value){std::memcpy(at,&value,sizeof(value));}
int main(){
    std::vector<unsigned char> module(0x1371e1),state(0x248),buffers(0xb0),shader(0x40);
    verifiedD3d=reinterpret_cast<HMODULE>(module.data());
    UINT64 counter=1;auto* buffer=buffers.data()+0x50;
    auto* list=reinterpret_cast<ID3D12GraphicsCommandList4*>(0x1111);
    auto* pipeline=reinterpret_cast<ID3D12StateObject*>(0x2222);
    Put(module.data()+0x136d28,state.data());module[0xf6c30]=1;
    Put(state.data()+0x220,buffers.data());Put(buffers.data()+0xa8,&counter);Put(state.data()+0x240,shader.data());Put(shader.data(),pipeline);
    Put(buffer+0x30,UINT64(320));Put(buffer+0x48,UINT64(0x1000));
    Put(state.data()+0x200,UINT(32));Put(state.data()+0x204,UINT(64));Put(state.data()+0x208,UINT(32));
    Put(state.data()+0x20c,UINT(64));Put(state.data()+0x210,UINT(96));Put(state.data()+0x218,UINT(2));
    Put(state.data()+0x228,int(320));Put(state.data()+0x22c,int(1));Put(state.data()+0x230,UINT64(0x3333));
    D3D12_DISPATCH_RAYS_DESC desc{};desc.RayGenerationShaderRecord={0x1000,32};desc.MissShaderTable={0x1040,64,32};desc.HitGroupTable={0x1080,192,96};desc.Width=1280;desc.Height=720;desc.Depth=1;
    Recorded(list,&desc,pipeline);assert(!pending.serial); // No consumer is inert.
    assert(Register(&Receive));assert(!Register(&Receive));SetLastError(1234);
    Recorded(list,&desc,pipeline);assert(recorded==1&&received.hitTable==0x1080&&received.width==1280);
    auto first=received;auto* job=module.data()+0x136d30;
    Put(job+0x10,first.writer.buffer);Put(job+0x18,first.writer.state);Put(job+0x20,first.writer.bytes);
    Put(job+0x24,first.writer.rays);Put(job+0x28,first.writer.count);Put(job+0x30,first.writer.bindings);
    Scheduling(job);assert(!pending.serial);
    auto wrong=first.writer;wrong.bindings++;assert(!Begin(0,wrong).serial);
    auto claimed=Begin(0,first.writer);assert(claimed.serial==first.serial);Finish(0,claimed);assert(ready==1);
    Finish(0,claimed);assert(ready==1&&GetLastError()==1234);
    Recorded(list,&desc,pipeline);auto second=received;assert(second.serial!=first.serial);
    Scheduling(job);claimed=Begin(0,second.writer);Finish(0,first);assert(ready==1);Finish(0,claimed);assert(ready==2);
    Recorded(list,&desc,pipeline);Put(job+0x30,UINT64(0x4444));Scheduling(job);assert(cancelled==1&&!Begin(0,first.writer).serial);
    module[0x1371e0]=1;Recorded(list,&desc,pipeline);assert(!pending.serial);module[0x1371e0]=0;
    module[0xf6c30]=0;Recorded(list,&desc,pipeline);assert(!pending.serial);module[0xf6c30]=1;
    ++desc.HitGroupTable.StartAddress;Recorded(list,&desc,pipeline);assert(!pending.serial);--desc.HitGroupTable.StartAddress;
    Recorded(list,reinterpret_cast<D3D12_DISPATCH_RAYS_DESC*>(1),pipeline);assert(!pending.serial);
    std::puts("PASS live job bridge: native dispatch layout, pre-schedule registration, worker publication, stale generations, mismatches and guarded reads.");
}
