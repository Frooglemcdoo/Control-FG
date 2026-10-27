"""Differential tests against the unchanged parser using captured shader bytes."""
from pathlib import Path
import subprocess,shutil,tempfile
root=Path(__file__).resolve().parents[1]
source=r'''
#include "rr_albedo_shader.h"
#include <cassert>
#include <fstream>
#include <iterator>
#include <iostream>
using namespace RRAlbedoShader;
static bool Same(const Contract& a,const Contract& b){
 return a.targetCount==b.targetCount&&a.componentMasks==b.componentMasks&&a.neverWritesMasks==b.neverWritesMasks&&a.instructionCount==b.instructionCount&&a.resourceCount==b.resourceCount&&a.shaderMajor==b.shaderMajor&&a.shaderMinor==b.shaderMinor;
}
static void Compare(ValidationCache& cache,const std::vector<uint8_t>& data,bool pixel){
 Contract expected{},actual{};const char* er=nullptr;const char* ar=nullptr;
 bool e=Parse(data.data(),data.size(),pixel,expected,&er);
 bool a=cache.Validate(data,pixel,actual,&ar);
 assert(e==a&&Same(expected,actual));
 assert((!er&&!ar)||(er&&ar&&!std::strcmp(er,ar)));
}
int main(int argc,char** argv){
 assert(argc>2);unsigned validPixels=0,validVertices=0;
 ValidationCache cache;
 for(int n=1;n<argc;++n){
  std::ifstream input(argv[n],std::ios::binary);std::vector<uint8_t> data((std::istreambuf_iterator<char>(input)),{});assert(!data.empty());
  for(bool pixel:{false,true}){
   Contract out{};bool valid=Parse(data.data(),data.size(),pixel,out);
   if(valid){if(pixel)++validPixels;else ++validVertices;}
   Compare(cache,data,pixel);auto hits=cache.Hits();Compare(cache,data,pixel);
   if(valid)assert(cache.Hits()==hits+1);
   // Same allocation/address with changed contents must be revalidated.
   for(size_t i:{size_t(0),size_t(20),data.size()/2,data.size()-1}){
    data[i]^=0x80;Compare(cache,data,pixel);data[i]^=0x80;
   }
   for(size_t size:{size_t(0),size_t(16),data.size()-1}){auto cut=data;cut.resize(size);Compare(cache,cut,pixel);}
   ValidationCache disabled(0,0);Compare(disabled,data,pixel);Compare(disabled,data,pixel);assert(disabled.Hits()==0&&disabled.Bytes()==0&&disabled.Parses()==2);
   ValidationCache bounded(data.size(),1);Compare(bounded,data,pixel);Compare(bounded,data,pixel);
   auto changed=data;changed.back()^=1;Compare(bounded,changed,pixel);assert(bounded.Bytes()<=data.size());
   ValidationCache fresh;Compare(fresh,data,pixel);assert(fresh.Hits()==0&&fresh.Parses()==1);
  }
 }
 assert(validPixels>=4&&validVertices>=1&&cache.Hits()>0&&cache.Bytes()<=512*1024);
 std::cout<<"PASS exact-byte cache: real vertex/pixel contracts, changed bytes at same address, stage separation, corrupt/truncated rejection, bounded/disabled fallback, fresh-session isolation\n";
}
'''
with tempfile.TemporaryDirectory() as d:
 d=Path(d);cpp=d/'test.cpp';cpp.write_text(source);exe=d/'test.exe'
 fixtures=[]
 for i in range(4):
  p=d/f'pixel-{i}.dxbc';p.write_bytes(bytes.fromhex((root/f'tools/fixtures/rr-world-albedo/variant-{i}.hex').read_text()));fixtures.append(str(p))
 fixtures.append(str(root/'validation/foliage-variant/evidence/pair-000-original_vs.dxbc'))
 compiler=shutil.which('cl') or shutil.which('g++');assert compiler
 if Path(compiler).stem.lower()=='cl':args=[compiler,'/nologo','/std:c++17','/EHsc','/W4',str(cpp),'/I'+str(root/'src'),'/Fe:'+str(exe),'/Fo:'+str(d/'test.obj')]
 else:args=[compiler,'-std=c++17','-O2','-Wall','-Wextra',str(cpp),'-I'+str(root/'src'),'-o',str(exe)]
 subprocess.run(args,check=True);subprocess.run([str(exe),*fixtures],check=True)
