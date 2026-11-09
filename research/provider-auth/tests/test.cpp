#include "../src/rr_provider_auth.h"
#include <array>
#include <cassert>
#include <cstdio>
using namespace control_rr_provider;
using Matrix=std::array<float,16>;
struct Fake {
 Entry meta{128,0,64,2}; Matrix data{}; int reads=0,compares=0;
 bool unavailable=false,readFail=false,mutation=false,bytesFail=false,bytesMutate=false;
 int byteReads=0;
 static bool Bytes(void* p,int offset,void* out,std::size_t size){auto& f=*static_cast<Fake*>(p);
  ++f.byteReads;assert(offset==f.meta.offset&&size==64);std::memcpy(out,f.data.data(),size);
  if(f.bytesMutate)f.data[0]+=1;
  return !f.bytesFail;}
 static bool Read(void* p,int,Entry& e){auto& f=*static_cast<Fake*>(p);e=f.meta;
  if(f.mutation&&++f.reads==2)++e.offset;
  return !f.readFail;}
 static bool Compare(void* p,int,const void* bytes,int& result){auto& f=*static_cast<Fake*>(p);
  ++f.compares;result=std::memcmp(f.data.data(),bytes,sizeof(Matrix));return !f.unavailable;}
 Access Callbacks(){return {this,Read,Compare};}
};
int main(){
 Fake f;for(unsigned i=0;i<16;++i)f.data[i]=float(i+1);
 const Matrix candidate=f.data;Matrix out{};
 assert(Authenticate(f.Callbacks(),7,candidate,out)&&out==candidate);
 // Every byte differs independently, including float sign/exponent bits.
 for(unsigned i=0;i<sizeof(Matrix);++i){Matrix bad=candidate;
  reinterpret_cast<unsigned char*>(&bad)[i]^=1;
  assert(!Authenticate(f.Callbacks(),7,bad,out)&&out==Matrix{});}
 f.unavailable=true;assert(!Authenticate(f.Callbacks(),7,candidate,out));f.unavailable=false;
 f.mutation=true;f.reads=0;assert(!Authenticate(f.Callbacks(),7,candidate,out));f.mutation=false;
 for(int size: {0,8,63,65,128,-1}){f.meta.size=size;f.compares=0;
  assert(!Authenticate(f.Callbacks(),7,candidate,out)&&f.compares==0);}
 f.meta.size=64;f.meta.offset=-1;f.compares=0;
 assert(!Authenticate(f.Callbacks(),7,candidate,out)&&f.compares==0);f.meta.offset=128;
 f.readFail=true;assert(!Authenticate(f.Callbacks(),7,candidate,out));f.readFail=false;
 assert(!Authenticate(f.Callbacks(),-1,candidate,out));
 auto a=f.Callbacks();a.compare=nullptr;assert(!Authenticate(a,7,candidate,out));
 out=candidate;assert(Authenticate(f.Callbacks(),7,out,out)&&out==candidate);
 Matrix stale{};
 assert(!Authenticate(f.Callbacks(),7,stale,out));
 assert(Snapshot(f.Callbacks(),7,Fake::Bytes,out)&&out==f.data);
 f.bytesFail=true;assert(!Snapshot(f.Callbacks(),7,Fake::Bytes,out)&&out==Matrix{});f.bytesFail=false;
 f.bytesMutate=true;assert(!Snapshot(f.Callbacks(),7,Fake::Bytes,out)&&out==Matrix{});f.bytesMutate=false;
 f.mutation=true;f.reads=0;assert(!Snapshot(f.Callbacks(),7,Fake::Bytes,out));f.mutation=false;
 for(int offset:{-1,0x40000-63,0x40000,0x7fffffff}){f.meta.offset=offset;f.byteReads=0;
  assert(!Snapshot(f.Callbacks(),7,Fake::Bytes,out)&&f.byteReads==0);}
 f.meta.offset=0x40000-64;assert(Snapshot(f.Callbacks(),7,Fake::Bytes,out));
 f.meta.size=65;f.byteReads=0;assert(!Snapshot(f.Callbacks(),7,Fake::Bytes,out)&&f.byteReads==0);
 puts("PASS bound snapshot despite stale mirror, changed bytes/metadata, failed reads, arena bounds; byte mismatch, unavailable provider, size-before-compare, metadata mutation, read failure, aliasing");
}
