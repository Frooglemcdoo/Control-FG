#pragma once
// Read-only observations in the already hash-verified renderer. The three RT
// counts are set from the same native graphics-menu integer. These are option
// values, not proof of shader execution; GI's producer logs its actual override.
struct RRSampleOptions { unsigned specular,diffuse,shadow,specularFilter; };
static bool RRReadSampleOptions(HMODULE renderer,RRSampleOptions* out) noexcept {
    if(!renderer||!out)return false;
    __try {
        const auto* base=reinterpret_cast<const unsigned char*>(renderer);
        memcpy(&out->specular,base+0x9137e0,4);
        memcpy(&out->diffuse,base+0x913bd0,4);
        memcpy(&out->shadow,base+0x913c90,4);
        memcpy(&out->specularFilter,base+0x913630,4);
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
static void RRReportSampleOptions(HMODULE renderer,unsigned long long frame,bool selected,unsigned giRequest) noexcept {
    const DWORD saved=GetLastError();
    RRSampleOptions now{};
    static RRSampleOptions previous{};
    static unsigned lastRequest=~0u;
    static bool lastSelected=false,initialized=false;
    if(RRReadSampleOptions(renderer,&now)){
        const bool changed=!initialized||memcmp(&now,&previous,sizeof(now))!=0||lastRequest!=giRequest||lastSelected!=selected;
        if(changed||frame%240==0){
            native_lighting::Trace("rt_sample_options frame=%llu specular=%u diffuse=%u shadow=%u native_specular_filter=%u gi_request=%u rr_selected=%u option_values_only=1",
                frame,now.specular,now.diffuse,now.shadow,now.specularFilter,giRequest,unsigned(selected));
            previous=now;lastRequest=giRequest;lastSelected=selected;initialized=true;
        }
    }
    SetLastError(saved);
}
