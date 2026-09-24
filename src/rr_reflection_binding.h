#pragma once
#include <cstdint>
#include <cstddef>
#include <limits>
namespace control_rr_reflection {
using Address=std::uintptr_t;
static constexpr Address BindCallRva=0x129662,BindFunctionRva=0xa3940;
static constexpr Address MaterialShaderRva=0x1291888,PositionShaderRva=0x12918d8;
struct Shape {std::uint64_t width=0;std::uint32_t height=0,format=0,layers=0,mips=0,samples=0,quality=0,flags=0,dimension=0;};
struct Texture {Address container=0,native=0,resource=0,tracker=0,device=0;std::uint32_t state=0;Shape shape{};};
struct Context {std::uint64_t frame=0;Address list=0,queue=0,device=0,view=0;};
struct Snapshot {Texture material{},position{};Context context{};Address owner=0;};
struct Access {
 void* context=nullptr;
 bool (*read)(void*,Address,void*,std::size_t)=nullptr;
 Address (*containerNative)(void*,Address)=nullptr;
 Address (*shaderNative)(void*,Address)=nullptr;
 bool (*describe)(void*,Address,Shape&,Address&)=nullptr;
 bool (*currentContext)(void*,Context&)=nullptr;
};
inline bool Same(const Context&a,const Context&b){return a.frame==b.frame&&a.list==b.list&&a.queue==b.queue&&a.device==b.device&&a.view==b.view;}
inline bool Same(const Shape&a,const Shape&b){return a.width==b.width&&a.height==b.height&&a.format==b.format&&a.layers==b.layers&&a.mips==b.mips&&a.samples==b.samples&&a.quality==b.quality&&a.flags==b.flags&&a.dimension==b.dimension;}
template<class T> bool Read(const Access&a,Address p,std::size_t offset,T&v){return p&&offset<=(std::numeric_limits<Address>::max)()-p&&a.read(a.context,p+offset,&v,sizeof(v));}
inline bool ReadTexture(const Access&a,Address container,Address shader,Texture&out){
 Texture t{};t.container=container;t.native=a.containerNative(a.context,container);
 if(!t.native||t.native!=a.shaderNative(a.context,shader))return false;
 std::uint32_t flags=0;std::uint8_t manual=0;
 if(!Read(a,t.native,0x38,flags)||!Read(a,t.native,0x68,manual)||(flags&0x30)||manual||
    !Read(a,t.native,0x88,t.resource)||!t.resource||!Read(a,t.native,0x50,t.tracker))return false;
 if(!t.tracker){if(t.native>(std::numeric_limits<Address>::max)()-0x40)return false;t.tracker=t.native+0x40;}
 if(!Read(a,t.tracker,0x20,t.state)||!t.state||(t.state&~0xc0u)||
    !a.describe(a.context,t.resource,t.shape,t.device)||!t.device)return false;
 const auto&d=t.shape;
 if(d.dimension!=3||d.width<64||d.width>8192||d.height<64||d.height>8192||
    d.layers<1||d.layers>32||d.mips!=1||d.samples!=1||d.quality||(d.flags&(0x2u|0x8u|0x10u|0x20u)))return false;
 out=t;return true;
}
inline bool Same(const Texture&a,const Texture&b){return a.container==b.container&&a.native==b.native&&a.resource==b.resource&&a.tracker==b.tracker&&a.device==b.device&&a.state==b.state&&Same(a.shape,b.shape);}
// Synchronous borrowed snapshot only. Must be consumed at this exact boundary,
// before returning to the engine. This does not prove the active DXR writer.
inline bool ReadBound(const Access&a,Address rendererBase,Address callRva,Address owner,Snapshot&out){
 out={};if(callRva!=BindCallRva||!owner||!rendererBase||!a.read||!a.containerNative||!a.shaderNative||!a.describe||!a.currentContext||
 rendererBase>(std::numeric_limits<Address>::max)()-PositionShaderRva)return false;
 Snapshot s{};s.owner=owner;
 if(!a.currentContext(a.context,s.context)||!s.context.frame||!s.context.list||!s.context.queue||!s.context.device||!s.context.view)return false;
 Address material=0,position=0;
 if(!Read(a,owner,0,material)||!Read(a,owner,0x10,position)||!material||!position||material==position)return false;
 if(!ReadTexture(a,material,rendererBase+MaterialShaderRva,s.material)||!ReadTexture(a,position,rendererBase+PositionShaderRva,s.position))return false;
 if(s.material.resource==s.position.resource||s.material.device!=s.context.device||s.position.device!=s.context.device||
    s.material.shape.format!=57||s.position.shape.format!=10||s.material.shape.width!=s.position.shape.width||
    s.material.shape.height!=s.position.shape.height||s.material.shape.layers!=s.position.shape.layers)return false;
 Snapshot again{};Address m=0,p=0;
 if(!Read(a,owner,0,m)||!Read(a,owner,0x10,p)||m!=material||p!=position||
    !ReadTexture(a,m,rendererBase+MaterialShaderRva,again.material)||!ReadTexture(a,p,rendererBase+PositionShaderRva,again.position)||
    !a.currentContext(a.context,again.context)||!Same(s.context,again.context)||!Same(s.material,again.material)||!Same(s.position,again.position))return false;
 out=s;return true;
}
struct Transition {Address resource=0;std::uint32_t before=0,after=0;};
struct CopyProgress {bool sourcesHeld=false,commandsAttempted=false,restored=false,ready=false;};
// Backend owns source references and private destinations through queue-fence retirement.
// Its methods return false on failure; after commandsAttempted, ambiguous failure
// requires retaining all resources and disabling consumption, never recycling them.
template<class Backend> bool RecordCopies(Backend&b,const Snapshot&s,Address dstMaterial,Address dstPosition,CopyProgress&progress){
 progress={};if(!s.material.resource||!s.position.resource||s.material.resource==s.position.resource||
 !s.material.state||!s.position.state||(s.material.state&~0xc0u)||(s.position.state&~0xc0u)||!dstMaterial||!dstPosition||dstMaterial==dstPosition||dstMaterial==s.material.resource||dstMaterial==s.position.resource||
 dstPosition==s.material.resource||dstPosition==s.position.resource||!b.Current(s.context)||
 !b.Destination(dstMaterial,s.material.shape,s.context.device)||!b.Destination(dstPosition,s.position.shape,s.context.device))return false;
 // Mark potential ownership before acquisition: a backend fault may occur after
 // the first AddRef. Failure must retain its partial ownership until shutdown.
 progress.sourcesHeld=true;
 if(!b.HoldSources(s.material.resource,s.position.resource))return false;
 const Transition begin[4]={{s.material.resource,s.material.state,0x800},{s.position.resource,s.position.state,0x800},
 {dstMaterial,0x40,0x400},{dstPosition,0x40,0x400}};
 const Transition end[4]={{s.material.resource,0x800,s.material.state},{s.position.resource,0x800,s.position.state},
 {dstMaterial,0x400,0x40},{dstPosition,0x400,0x40}};
 progress.commandsAttempted=true;
 if(!b.Barriers(begin,4))return false;
 if(!b.Copy(dstMaterial,s.material.resource)||!b.Copy(dstPosition,s.position.resource))return false;
 if(!b.Barriers(end,4))return false;
 progress.restored=true;progress.ready=true;return true;
}
}
