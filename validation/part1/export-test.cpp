#include <cassert>
#include <iostream>
struct CameraSnapshot {unsigned long long engineFrame=22;double worldToView[12]{},viewToWorld[12]{},viewToClip[16]{},clipToView[16]{},worldToClip[16]{},clipToWorld[16]{};};
#include "../../src/rr_guide_export.h"
int main(){
 unsigned char gb[8]{};float normal[8]={0,0,1,0.5f,0,0,1,0.5f};
 unsigned char table[16];for(unsigned i=0;i<16;++i)table[i]=static_cast<unsigned char>(i*13);
 RRGuideExportView v{};v.width=2;v.height=1;v.gbuffer1=v.gbuffer2=gb;v.normalRoughness=normal;
 v.gbuffer1RowPitch=v.gbuffer2RowPitch=8;v.normalRoughnessRowPitch=32;v.engineFrame=v.presentToken=22;
 v.part1Data=table;v.part1Bytes=16;wchar_t path[32768]{};
 assert(RRGuideExportCapture(v,path,32768));std::wcout<<path<<L"\n";
 v.part1Bytes=15;assert(!RRGuideExportCapture(v,path,32768) && !path[0]);v.part1Bytes=524296;assert(!RRGuideExportDetail::Validate(v));
 v.part1Bytes=16;v.part1Data=nullptr;assert(!RRGuideExportDetail::Validate(v));v.part1Data=table;
 failPart1Write=true;assert(!RRGuideExportCapture(v,path,32768) && !path[0]);failPart1Write=false;
 v.part1Bytes=0;v.part1Data=nullptr;assert(RRGuideExportCapture(v,path,32768));std::wcout<<path<<L"\n";
}
