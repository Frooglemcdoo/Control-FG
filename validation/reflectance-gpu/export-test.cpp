#include <cassert>
#include <iostream>
struct CameraSnapshot {unsigned long long engineFrame=22;double worldToView[12]{},viewToWorld[12]{},viewToClip[16]{},clipToView[16]{},worldToClip[16]{},clipToWorld[16]{};};
#include "../../src/rr_guide_export.h"
int main() {
    unsigned char gb[8]{};float normal[8]={0,0,1,.5f,0,0,1,.5f};
    unsigned char bytes[48];for(unsigned i=0;i<48;++i) bytes[i]=static_cast<unsigned char>(i);
    RRGuideExportView v{};v.width=2;v.height=1;v.gbuffer1=v.gbuffer2=gb;v.normalRoughness=normal;
    v.gbuffer1RowPitch=v.gbuffer2RowPitch=8;v.normalRoughnessRowPitch=32;v.engineFrame=v.presentToken=22;
    v.reflectanceData=bytes;v.reflectanceBytes=48;v.reflectanceMaxError=1e-7;
    wchar_t path[32768]{};
    assert(RRGuideExportCapture(v,path,32768));std::wcout<<path<<L"\n";
    failReflectanceWrite=true;assert(!RRGuideExportCapture(v,path,32768) && !path[0]);failReflectanceWrite=false;
    v.reflectanceBytes=47;assert(!RRGuideExportDetail::Validate(v));v.reflectanceBytes=48;
    v.reflectanceData=nullptr;assert(!RRGuideExportDetail::Validate(v));v.reflectanceData=bytes;
    v.reflectanceMaxError=std::numeric_limits<double>::infinity();assert(!RRGuideExportDetail::Validate(v));
}
