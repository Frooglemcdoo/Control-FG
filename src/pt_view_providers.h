#pragma once
#include "pt_native_view.h"
#include "rr_provider_auth.h"
// Read-only adapter to the already hash-gated Steam constant provider ABI used
// by RR. Shader CBVs contain only the fields that their pass requests; private
// primary rays instead retain a complete authenticated camera snapshot.
namespace PTNativeView {
class Providers {
    unsigned char* d3d=nullptr;
    std::array<int,6> ids{{-1,-1,-1,-1,-1,-1}};
    inline static constexpr unsigned offsets[6]={8,32,96,224,1112,0};
    inline static constexpr unsigned sizes[6]={8,64,64,64,4,8};
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
            const char* names[6]={"g_vInvScreenRes","g_mWorldToView","g_mViewToWorld","g_mClipToView","g_uCurrentFrame","g_vScreenRes"};
            for(unsigned n=0;n<6;++n){struct Slot {int id,tag;} slot{-1,0};
                reinterpret_cast<Resolve>(r+0x210270)(&slot,names[n]);
                if(slot.id<0||slot.id>=4096)return false;ids[n]=slot.id;
            }
            return true;
        }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
    }
    bool Read(View& view,Pass pass){
        // The caller has already read the real bound CBV. Authenticate fields
        // actually consumed by that native pass before using any provider-only
        // fields; never accept a globally most-recent camera from another view.
        view.pass=pass;view.bound=view.bytes;DWORD fault=0;unsigned long long before=0,after=0;
        view.failure="provider_frame";
        if(!ReadEngineFrameSafe(&before,&fault)||!before)return false;
        const unsigned count=pass==Pass::environment?6u:5u;
        for(unsigned n=0;n<count;++n){view.failure="provider_snapshot";view.failureOffset=offsets[n];
            if(!Snapshot(n,view.bytes.data()+offsets[n]))return false;}
        view.failure="provider_binding_mismatch";
        if(!view.MatchesBound(view.bytes))return false;
        // Re-read each provider after the complete set, not just an individual
        // matrix. TLS/frame changes cannot silently mix separate cameras.
        for(unsigned n=0;n<count;++n){std::array<unsigned char,64> check{};
            if(!Snapshot(n,check.data())||std::memcmp(check.data(),view.bytes.data()+offsets[n],sizes[n]))return false;}
        if(!ReadEngineFrameSafe(&after,&fault)||before!=after)return false;
        view.engineFrame=before;return view.Decode();
    }
};
}
