#include <windows.h>
#include <dxgi1_6.h>
#include <cassert>
#include <cstdio>
#include "../src/pt_glass_resources.h"
using Microsoft::WRL::ComPtr;
int main(){
    PTGlass::SubmissionHold handoff;
    assert(!handoff.Finish(true));assert(!handoff.Ready());
    handoff.Submitted();assert(!handoff.Ready());
    assert(handoff.Finish(true));assert(!handoff.Ready());
    handoff.Submitted();assert(handoff.Ready());
    PTGlass::SubmissionHold skipped;
    assert(skipped.Finish(false));assert(!skipped.Ready());
    skipped.Submitted();assert(skipped.Ready());
    PTGlass::Ticket ticket;
    assert(!ticket.Lease(0));assert(ticket.Lease(17));assert(!ticket.Lease(18));
    assert(!ticket.Submit(18,1));assert(ticket.Submit(17,9));
    assert(!ticket.Complete(8));assert(!ticket.Complete(UINT64_MAX));
    assert(!ticket.Discard(17));assert(ticket.Complete(9));assert(ticket.Releasable());
    UINT64 bytes=123;assert(!PTGlass::HitBytes(0,1,&bytes)&&bytes==0);
    assert(!PTGlass::HitBytes(UINT32_MAX,UINT32_MAX,&bytes));
    assert(PTGlass::HitBytes(3840,2160,&bytes)&&bytes==1061683200ull);
    ComPtr<IDXGIFactory4> factory;assert(SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))));
    ComPtr<IDXGIAdapter> adapter;assert(SUCCEEDED(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter))));
    ComPtr<ID3D12Device> device;assert(SUCCEEDED(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device))));
    D3D12_COMMAND_QUEUE_DESC qd{};qd.Type=D3D12_COMMAND_LIST_TYPE_DIRECT;
    ComPtr<ID3D12CommandQueue> queue;assert(SUCCEEDED(device->CreateCommandQueue(&qd,IID_PPV_ARGS(&queue))));
    ComPtr<ID3D12Fence> gate;assert(SUCCEEDED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&gate))));
    PTGlass::Resources pool;assert(SUCCEEDED(pool.Initialize(device.Get(),queue.Get(),16ull*1024*1024)));
    PTGlass::Resources::Slot* first=nullptr;assert(SUCCEEDED(pool.Acquire(1,128,128,&first)));
    assert(first->hits->GetDesc().Width==128ull*128*64*PTGlass::HitLayers);
    assert(first->radiance->GetDesc().Format==DXGI_FORMAT_R16G16B16A16_FLOAT);
    assert(first->radiance->GetDesc().Height==128*PTGlass::HitLayers);
    assert(first->history->GetDesc().Width==128ull*128*48);
    assert(first->historySerial==0&&first->historyGeneration==0);
    const auto one=pool.ResidentBytes();assert(one>0);
    PTGlass::Resources::Slot* duplicate=nullptr;assert(FAILED(pool.Acquire(1,128,128,&duplicate)));
    // Hold real GPU execution behind an unsignalled fence, then queue the pool's
    // completion markers behind it. Polling cannot falsely free these slots.
    assert(SUCCEEDED(queue->Wait(gate.Get(),1)));
    assert(SUCCEEDED(pool.Submitted(1)));
    PTGlass::Resources::Slot* second=nullptr;assert(SUCCEEDED(pool.Acquire(2,128,128,&second)));
    assert(first!=second);assert(SUCCEEDED(pool.Submitted(2)));
    PTGlass::Resources::Slot* third=nullptr;assert(SUCCEEDED(pool.Acquire(3,128,128,&third)));
    assert(third!=first&&third!=second);
    assert(pool.Acquire(4,256,256,&duplicate)==DXGI_ERROR_WAS_STILL_DRAWING);
    assert(pool.ResidentBytes()==3*one);
    assert(!pool.Discarded(1));assert(pool.Discarded(3));
    assert(!pool.StopAndTrim());assert(pool.ResidentBytes()==2*one);
    assert(FAILED(pool.Acquire(5,128,128,&duplicate)));
    assert(SUCCEEDED(gate->Signal(1)));
    // A second queue signal orders this test's wait after both pool markers.
    ComPtr<ID3D12Fence> finished;assert(SUCCEEDED(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&finished))));
    assert(SUCCEEDED(queue->Signal(finished.Get(),1)));
    HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);assert(event);
    assert(SUCCEEDED(finished->SetEventOnCompletion(1,event)));
    assert(WaitForSingleObject(event,10000)==WAIT_OBJECT_0);CloseHandle(event);
    assert(pool.StopAndTrim());assert(pool.ResidentBytes()==0);
    PTGlass::Resources resize;assert(SUCCEEDED(resize.Initialize(device.Get(),queue.Get(),16ull*1024*1024)));
    assert(SUCCEEDED(resize.Acquire(10,128,128,&first)));first->historySerial=10;first->historyGeneration=2;assert(resize.Discarded(10));
    assert(SUCCEEDED(resize.Acquire(11,256,256,&first)));
    assert(first->historySerial==0&&first->historyGeneration==0);
    assert(first->history->GetDesc().Width==256ull*256*48);
    assert(first->hits->GetDesc().Width==256ull*256*64*PTGlass::HitLayers);
    assert(resize.Discarded(11));assert(resize.StopAndTrim());assert(resize.ResidentBytes()==0);
    PTGlass::Resources limited;assert(SUCCEEDED(limited.Initialize(device.Get(),queue.Get(),1)));
    assert(limited.Acquire(12,128,128,&first)==E_OUTOFMEMORY);assert(limited.ResidentBytes()==0);
    assert(limited.StopAndTrim());
    puts("PASS: WARP allocations, bounded pool, blocked-queue lifetime, resize, discard, budget rejection, full retirement; no native textures modified.");
}
