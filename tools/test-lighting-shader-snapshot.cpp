#include "../src/lighting_shader_snapshot.h"
#include <cassert>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <iostream>
int main(){
 using Store=native_lighting::LightingShaderSnapshot;
 auto store=std::make_unique<Store>();
 const char hash[]="0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";
 std::string bytes="DXBC"+std::string(300,'x');
 store->Record(1,bytes.data(),bytes.size(),hash);store->Record(1,bytes.data(),bytes.size(),hash);
 assert(store->Count()==1);
 std::string out;assert(store->Export([&](const char* p,size_t n){out.append(p,n);return true;}));
 assert(out.find("offset=256")!=std::string::npos&&out.find("shader_snapshot_end export=1 count=1")!=std::string::npos);
 auto complete=out;out.clear();unsigned writes=0;
 assert(!store->Export([&](const char* p,size_t n){if(++writes==3)return false;out.append(p,n);return true;}));
 assert(out.find("shader_snapshot_end")==std::string::npos);
 out.clear();assert(store->Export([&](const char* p,size_t n){out.append(p,n);return true;}));
 assert(out.find("shader_snapshot_end export=3 count=1")!=std::string::npos);
 store->Record(1,bytes.data(),Store::MaxShaderBytes+1,hash);store->Record(1,"bad!",4,hash);assert(store->Dropped()==2);
 // Publication and export run concurrently; published records remain immutable.
 std::thread producer([&]{store->Record(2,bytes.data(),bytes.size(),hash);});
 assert(store->Export([](const char*,size_t){return true;}));producer.join();assert(store->Count()==2);
 std::string large="DXBC"+std::string(Store::MaxShaderBytes-4,'z');
 for(unsigned i=0;i<40;++i){char unique[65]{};std::snprintf(unique,sizeof(unique),"%064u",i);store->Record(1,large.data(),large.size(),unique);}
 assert(store->Count()==33 && store->Dropped()==11);
 std::cout<<"PASS R148 shader snapshots: deduplication, bounds, concurrent publication, failed export and retry\n";
}
