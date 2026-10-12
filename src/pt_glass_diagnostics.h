#pragma once
#include <cstdint>
#include <cmath>
namespace PTGlass {
struct Diagnostic {
    std::uint32_t identity[4];
    float levels[4],filter[4],history[4],point[4],spot[4],output[4],reserved[4];
};
static_assert(sizeof(Diagnostic)==128,"GPU diagnostic stride");
struct DiagnosticSummary {
    unsigned sampled=0,shaded=0,diffuse=0,rejected=0,accepted=0,warm=0,changed=0,invalid=0;
    double native=0,raw=0,spatial=0,resolved=0,weight=0,samples=0,hits=0,count=0;
    double delta=0,absoluteDelta=0,finalLight=0;
    double point[4]{},spot[4]{};
    void Add(const Diagnostic& d,std::uint32_t serial,std::uint32_t mode) noexcept {
        if(d.identity[0]!=serial||d.identity[2]!=mode)return;
        ++sampled;if(d.identity[1]!=1)return;
        const float* groups[]{d.levels,d.filter,d.history,d.point,d.spot,d.output};
        for(const auto* values:groups)for(unsigned i=0;i<4;++i)if(!std::isfinite(values[i])){++invalid;return;}
        ++shaded;
        for(unsigned i=0;i<4;++i){point[i]+=d.point[i];spot[i]+=d.spot[i];}
        if(d.history[3]!=1)return;
        ++diffuse;if(d.history[1]>0.5f)++accepted;else ++rejected;
        if(d.history[0]>=8)++warm;
        native+=d.levels[0];raw+=d.levels[1];spatial+=d.levels[2];resolved+=d.levels[3];
        weight+=d.filter[0];samples+=d.filter[1];hits+=d.filter[2];count+=d.history[0];
        delta+=d.output[1];absoluteDelta+=d.output[2];finalLight+=d.output[0];
        if(d.output[2]>0.01f*(d.output[0]>1e-4f?d.output[0]:1e-4f))++changed;
    }
};
}
