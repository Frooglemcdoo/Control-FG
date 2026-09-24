#include <cassert>
#include <iostream>
struct CameraSnapshot {unsigned long long engineFrame=22;double worldToView[12]{},viewToWorld[12]{},viewToClip[16]{},clipToView[16]{},worldToClip[16]{},clipToWorld[16]{};};
#include "../../src/rr_guide_export.h"
#include "../../src/rr_vertex_evidence.h"
int main() {
    RRVertexEvidence::Store evidence;
    std::vector<uint8_t> a(131072,1),b(131072,2),p(131072,3);
    assert(evidence.Add(a,b,p)==0);evidence.AddReference({0,4,12,14,true,0});
    auto audit=std::string("{\"schema\":\"ControlFG.RRMaterialRejectionAudit.v4\",\"engine_frame\":22,\"source_batches\":1,\"rejected_vertex_evidence\":")+evidence.Json()+"}";
    assert(audit.size()>262144 && audit.size()<8388608);
    unsigned char gb[8]{};float normal[8]={0,0,1,.5f,0,0,1,.5f};
    RRGuideExportView v{};v.width=2;v.height=1;v.gbuffer1=v.gbuffer2=gb;v.normalRoughness=normal;
    v.gbuffer1RowPitch=v.gbuffer2RowPitch=8;v.normalRoughnessRowPitch=32;v.engineFrame=v.presentToken=22;
    v.materialRejectionAuditJson=audit.data();v.materialRejectionAuditBytes=audit.size();
    wchar_t path[32768]{};assert(RRGuideExportCapture(v,path,32768));std::wcout<<path<<L'\n';
    v.materialRejectionAuditBytes=8388609;assert(!RRGuideExportDetail::Validate(v));
}
