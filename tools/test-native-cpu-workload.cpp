#include "../src/native_cpu_workload.h"
#include <cassert>
#include <thread>
#include <cstdio>
template<class T> static void Put(unsigned char* base,unsigned offset,T value){std::memcpy(base+offset,&value,sizeof(value));}
int main(){
    using namespace native_workload;
    unsigned char manager[0x108]{},context[0x9c]{},job[0x20]{};
    Put<std::uint32_t>(manager,0x80,120);Put<std::uint32_t>(manager,0xd0,120);Put<std::uint64_t>(manager,0x100,93);
    U64 a[4]{};MaterialValues(manager,42,a);assert(a[0]==120&&a[1]==120&&a[2]==93&&a[3]==42);
    Put<const void*>(job,0x18,context);
    Put<std::uint32_t>(context,0x18,400);Put<std::uint32_t>(context,0x78,700);Put<std::uint32_t>(context,0x88,300);Put<std::uint32_t>(context,0x98,300);
    PreparationValues(job,a);assert(a[0]==400&&a[1]==700&&a[2]==300&&a[3]==300);
    Cell cell;std::atomic<bool> done{false};
    std::thread writer([&]{for(U64 i=1;i<=10000;++i){U64 v[4]{i,i*2,i*3,i*4};cell.Add(v);}done=true;});
    while(!done){Snapshot s;if(cell.Read(s))for(unsigned i=0;i<4;++i){assert(s.last[i]==s.samples*(i+1));assert(s.sum[i]==s.samples*(s.samples+1)/2*(i+1));}}
    writer.join();Snapshot s;assert(cell.Read(s)&&s.samples==10000);
    cell.version.fetch_add(1);assert(!cell.Read(s));
    std::puts("PASS native workload: exact scalar layouts, coherent concurrent snapshots, bounded busy reads");
}
