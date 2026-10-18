#pragma once
#include "pt_native_view.h"
#include "rr_provider_auth.h"
// Read-only adapter to the already hash-gated Steam constant provider ABI used
// by RR. Shader CBVs contain only the fields that their pass requests; private
// primary rays instead retain a complete authenticated camera snapshot.
namespace PTNativeView {
class Providers {
    unsigned char* d3d=nullptr;
    std::array<int,9> ids{{-1,-1,-1,-1,-1,-1,-1,-1,-1}};
    inline static constexpr unsigned offsets[6]={8,32,96,224,1112,0};
    inline static constexpr unsigned sizes[9]={8,64,64,64,4,8,4,4,4};
    static bool Entry(void* context,std::int32_t id,control_rr_provider::Entry& out) noexcept {
        auto& self=*static_cast<Providers*>(context);
        __try {if(!self.d3d||id<0||id>=4096)return false;
            std::memcpy(&out,self.d3d+0x112840+std::size_t(id)*16,16);return true;
        }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
    }
    static bool Compare(void* context,std::int32_t id,const void* data,int& difference) noexcept {
        __try {auto& self=*static_cast<Providers*>(context);
            using Fn=bool(*)(int,const void*,int*);
            return reinterpret_cast<Fn>(self.d3d+0x16a40)(id,data,&difference);
        }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
    }
    static bool Copy(void* context,std::int32_t offset,void* out,std::size_t size) noexcept {
        __try {auto& self=*static_cast<Providers*>(context);
            if(offset<0||size>0x40000||std::size_t(offset)>0x40000-size)return false;
            const auto index=*reinterpret_cast<const unsigned*>(self.d3d+0x1115fc);if(index>=4096)return false;
            auto** array=reinterpret_cast<unsigned char**>(__readgsqword(0x58));
            if(!array||!array[index])return false;
            std::memcpy(out,array[index]+0x4250+offset,size);return true;
        }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
    }
    bool Snapshot(unsigned n,unsigned char* out) {
        control_rr_provider::Entry before{},after{};int difference=1;
        if(!Entry(this,ids[n],before)||before.offset<0||before.size!=int(sizes[n])||
           !Copy(this,before.offset,out,sizes[n])||!Compare(this,ids[n],out,difference)||difference||
           !Entry(this,ids[n],after)||std::memcmp(&before,&after,sizeof(before)))return false;
        return true;
    }
public:
    bool Initialize(HMODULE renderer,HMODULE native) noexcept {
        __try {
            if(!renderer||!native||renderer!=verifiedRenderer||native!=verifiedD3d)return false;
            auto* r=reinterpret_cast<unsigned char*>(renderer);d3d=reinterpret_cast<unsigned char*>(native);
            std::int32_t rel=0;std::memcpy(&rel,r+0x17e2e9+1,4);
            if(r[0x17e2e9]!=0xe8||r+0x17e2e9+5+rel!=r+0x210270)return false;
            using Resolve=void(*)(void*,const char*);
            const char* names[9]={"g_vInvScreenRes","g_mWorldToView","g_mViewToWorld","g_mClipToView","g_uCurrentFrame","g_vScreenRes",
                "g_uPointLightTotalCount","g_uSpotLightTotalCount","g_uSunLightTotalCount"};
            for(unsigned n=0;n<9;++n){struct Slot {int id,tag;} slot{-1,0};
                reinterpret_cast<Resolve>(r+0x210270)(&slot,names[n]);
                if(slot.id<0||slot.id>=4096)return false;ids[n]=slot.id;
            }
            return true;
        }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
    }
    // Primary DXBC only requests the sun total; obtain point/spot counts
    // from authenticated providers. PT imports no native ambient intensity.
    bool ReadLights(const View& view,const std::array<unsigned char,416>& bound,std::array<unsigned char,416>& complete){
        DWORD fault=0;unsigned long long before=0,after=0;
        if(!ReadEngineFrameSafe(&before,&fault)||before!=view.engineFrame)return false;
        complete=bound;std::memset(complete.data()+244,0,4);
        constexpr unsigned lightOffsets[3]={256,260,264};
        for(unsigned n=0;n<3;++n)if(!Snapshot(n+6,complete.data()+lightOffsets[n]))return false;
        // This field IS consumed by both authenticated primary variants.
        if(std::memcmp(bound.data()+264,complete.data()+264,4))return false;
        for(unsigned n=0;n<3;++n){std::array<unsigned char,4> check{};
            if(!Snapshot(n+6,check.data())||std::memcmp(check.data(),complete.data()+lightOffsets[n],4))return false;}
        // Tie view-space lights to the camera authenticated in this callback.
        for(unsigned n=1;n<5;++n){std::array<unsigned char,64> check{};
            if(!Snapshot(n,check.data())||std::memcmp(check.data(),view.bytes.data()+offsets[n],sizes[n]))return false;}
        return ReadEngineFrameSafe(&after,&fault)&&before==after;
    }
    bool Read(View& view,Pass pass,bool deferSurfaceBinding=false){
        // The caller has already read the real bound CBV. Authenticate fields
        // actually consumed by that native pass before using any provider-only
        // fields; never accept a globally most-recent camera from another view.
        view.pass=pass;view.bound=view.bytes;DWORD fault=0;unsigned long long before=0,after=0;
        view.failure="provider_frame";
        if(!ReadEngineFrameSafe(&before,&fault)||!before)return false;
        const unsigned count=5u;
        for(unsigned n=1;n<count;++n){view.failure="provider_snapshot";view.failureOffset=offsets[n];
            if(!Snapshot(n,view.bytes.data()+offsets[n]))return false;}
        view.failure="provider_binding_mismatch";
        view.failureOffset=view.Mismatch(view.bytes);
        // A surface candidate can be retained before its upload is finalized.
        // It is NOT authenticated for execution: submission must compare the
        // real CBV with this complete snapshot. Other passes remain strict.
        if(view.failureOffset!=UINT32_MAX&&!(deferSurfaceBinding&&pass==Pass::surface))return false;
        // Re-read each provider after the complete set, not just an individual
        // matrix. TLS/frame changes cannot silently mix separate cameras.
        for(unsigned n=1;n<count;++n){std::array<unsigned char,64> check{};
            if(!Snapshot(n,check.data())||std::memcmp(check.data(),view.bytes.data()+offsets[n],sizes[n]))return false;}
        if(!ReadEngineFrameSafe(&after,&fault)||before!=after)return false;
        // Rendering passes have their own resolution. TLS inverse screen
        // resolution can already refer to output/UI dimensions. It is not a
        // camera identity field. Keep the actual bound value; the environment
        // pass supplies screen resolution instead of its inverse.
        if(pass==Pass::environment){float extent[2]{};std::memcpy(extent,view.bound.data(),8);
            if(!std::isfinite(extent[0])||!std::isfinite(extent[1])||extent[0]<1||extent[1]<1)return false;
            extent[0]=1.f/extent[0];extent[1]=1.f/extent[1];std::memcpy(view.bytes.data()+8,extent,8);
        }
        view.engineFrame=before;return view.Decode();
    }
};
}
