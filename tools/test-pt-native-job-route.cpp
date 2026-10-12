#include "../src/pt_native_job_route.h"
#include <cassert>
#include <cstdio>
#include <thread>
int main(){
    using namespace PTNativeJobRoute;
    Router router;Ticket ticket;ticket.serial=1;ticket.list=2;ticket.pipeline=3;ticket.counter=4;
    ticket.writer={5,6,7,320,2,1};ticket.layout={0x1000,320,32,64,32,64,96,2};Ticket out;
    assert(!router.Schedule(16,ticket));assert(router.Schedule(0,ticket));
    assert(!router.Schedule(0,ticket));auto wrong=ticket.writer;wrong.buffer++;
    assert(!router.Begin(0,wrong,out)&&!out.serial);
    std::thread worker([&](){assert(router.Begin(0,ticket.writer,out));assert(out.serial==1&&out.layout.buffer==0x1000&&out.layout.hitRecords==2);
        Ticket duplicate;assert(!router.Begin(0,ticket.writer,duplicate));assert(!router.Finish(0,2));assert(router.Finish(0,1));});worker.join();
    assert(!router.Finish(0,1));ticket.serial=2;assert(router.Schedule(0,ticket));
    assert(!router.Finish(0,1));assert(router.Begin(0,ticket.writer,out)&&out.serial==2);assert(router.Finish(0,2));
    ticket.writer.rays=1;assert(!router.Schedule(0,ticket));
    std::puts("PASS native job route: cross-thread handoff, exact writer identity, duplicate claims and stale slot generations.");
}
