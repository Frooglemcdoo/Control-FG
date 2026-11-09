#pragma once
struct RRDiffuseViewSnapshot {
    void* tls=nullptr;void* context=nullptr;void* state=nullptr;void* view=nullptr;bool worker=false;
};
static bool RRDiffuseReadView(RRDiffuseViewSnapshot* out,DWORD* fault) noexcept {
    *out={};*fault=0;
    __try {
        if(!RRAlbedoNativeContext(&rrAlbedoAPI,&out->tls,&out->context,&out->state,&out->worker)) return false;
        out->view=rrDiffuseJoinView;
        return !out->worker || control_rr_albedo::ReadWorkerView(reinterpret_cast<std::uintptr_t>(verifiedRenderer),&out->view,fault);
    } __except(EXCEPTION_EXECUTE_HANDLER) {*fault=GetExceptionCode();return false;}
}
// Read only: never prepare, transition or bind a rejected depth texture.
static void RRDiffuseDepthEvidence(unsigned long long frame,int first,int end,const RRAlbedoNativeBindings* saved) noexcept {
    unsigned int flags=0,state=0;unsigned int special=0;bool readable=false;DWORD fault=0;
    __try {
        if(saved->depthTarget) {
            auto* bytes=static_cast<unsigned char*>(saved->depthTarget);
            flags=*reinterpret_cast<const unsigned int*>(bytes+0x38);special=bytes[0x68];
            auto* tracker=*reinterpret_cast<unsigned char**>(bytes+0x50);if(!tracker) tracker=bytes+0x40;
            state=*reinterpret_cast<const unsigned int*>(tracker+0x20);readable=true;
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {fault=GetExceptionCode();}
    Log("RR_G12_DIFFUSE_DEPTH_EVIDENCE frame=%llu first=%d end=%d primary_view_selected=1 worker=%u depth=%p readable=%u flags=0x%X special=%u state=0x%X expected_state=0x10 exception=0x%08lX rr_eval=disabled",frame,first,end,unsigned(saved->workerContext),saved->depthTarget,unsigned(readable),flags,special,state,fault);
}
