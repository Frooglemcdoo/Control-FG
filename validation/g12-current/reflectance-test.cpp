#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <limits>
#include "../../src/rr_reflectance.h"
using namespace control_rr_reflectance;
static void require(bool ok) { if (!ok) { std::cerr << "FAIL\n"; std::exit(1); } }
static void pass(const char* name) { std::cout << "PASS " << name << '\n'; }
static bool near(double a,double b) { return std::fabs(a-b) <= 3e-6 + 3e-6*std::fabs(b); }
// Separately expressed double-precision matrix reference from NVIDIA section 4.2.1.
// This checks transcription/algebra, not independent GPU or engine semantics.
template<size_t N> double bilinear(const double (&m)[N][N],const double (&x)[N],const double (&y)[N]) {
    double result=0;
    for(size_t row=0;row<N;++row) for(size_t col=0;col<N;++col) result+=y[row]*m[row][col]*x[col];
    return result;
}
static Float3 reference(Float3 f,float alpha,float nv) {
    const double v=std::fabs(double(nv)),a=alpha;
    const double xy[]={1,v}, xw[]={1,v,v*v*v}, xz[]={1,v*v,v*v*v};
    const double ya[]={1,a}, yw[]={1,a,a*a*a};
    const double m1[][2]={{.99044,-1.28514},{1.29678,-.755907}};
    const double m2[][3]={{1,2.92338,59.4188},{20.3225,-27.0302,222.592},{121.563,626.13,316.627}};
    const double m3[][2]={{.0365463,3.32707},{9.0632,-9.04756}};
    const double m4[][3]={{1,3.59685,-1.36772},{9.04401,-16.3174,9.22949},{5.56589,19.7886,-20.2123}};
    const double bias=std::fmax(0.,bilinear(m1,xy,ya)/bilinear(m2,xw,yw)*std::fmin(1.,std::fmax(0.,f.y*50.)));
    const double scale=std::fmax(0.,bilinear(m3,xy,ya)/bilinear(m4,xz,yw));
    return {float(f.x*scale+bias),float(f.y*scale+bias),float(f.z*scale+bias)};
}
int main() {
    // Every binary16 bit pattern, including signed zeros, infinities and NaNs.
    for(unsigned bits=0;bits<65536;++bits) {
        std::uint16_t s[]={std::uint16_t(bits),std::uint16_t(bits^0x5555),std::uint16_t(bits^0xaaaa),0xffff},d[4]{};
        require(CopyRGBA16FToDiffuseReflectance(s,8,d,8,1,1));
        require(!std::memcmp(s,d,6)&&d[3]==0x3c00);
    }
    pass("all_half_bit_patterns_rgb_exact_alpha_one");
    unsigned char src[48],dst[64]; std::memset(src,0x55,sizeof(src));std::memset(dst,0xcc,sizeof(dst));
    require(CopyRGBA16FToDiffuseReflectance(src,24,dst,32,2,2));
    for(unsigned y=0;y<2;++y) { require(dst[y*32+7]==0x3c&&dst[y*32+15]==0x3c);for(unsigned i=16;i<32;++i)require(dst[y*32+i]==0xcc); }
    require(CopyRGBA16FToDiffuseReflectance(src,24,src,24,2,2));
    pass("two_row_pitch_padding_and_in_place");
    require(CopyRGBA16FToDiffuseReflectance(nullptr,0,nullptr,0,0,5));
    require(!CopyRGBA16FToDiffuseReflectance(nullptr,8,dst,8,1,1));
    require(!CopyRGBA16FToDiffuseReflectance(src,7,dst,8,1,1));
    require(!CopyRGBA16FToDiffuseReflectance(src,(std::numeric_limits<size_t>::max)(),dst,8,1,2));
    require(!CopyRGBA16FToDiffuseReflectance(src,24,src,32,2,2));
    pass("invalid_copy_contracts_rejected");
    auto f=DecodeControlMaterialF0(0x80402001u,.2f);
    require(near(f.x,128./255)&&near(f.y,64./255)&&near(f.z,32./255));
    auto tint=DecodeControlMaterialF0(0xFF000000u,.25f);
    require(near(tint.x,.25/(.2126+1e-6))&&near(tint.y,0)&&near(tint.z,0));
    auto black=DecodeControlMaterialF0(0,.25f);require(near(black.x,.25)&&near(black.y,.25)&&near(black.z,.25));
    pass("control_f0_modes_and_black_tint");
    for(unsigned i=0;i<=100;++i) for(int j=-100;j<=100;++j) {
        float a=float(i)/100,v=float(j)/100;
        for(auto color:{Float3{.04f,.04f,.04f},Float3{.8f,.35f,.1f},Float3{1,0,0},Float3{.5f,.001f,.2f},Float3{0,0,0}}) {
            auto actual=EnvBRDFApprox2(color,a,v),want=reference(color,a,v);
            require(near(actual.x,want.x)&&near(actual.y,want.y)&&near(actual.z,want.z));
        }
    }
    pass("101505_cases_against_double_matrix_reference");
    auto a=SpecularReflectanceFromLinearRoughness({.04f,.04f,.04f},.5f,.5f);
    auto b=reference({.04f,.04f,.04f},.25f,.5f);
    require(near(a.x,b.x));
    pass("linear_roughness_squared_exactly_once");
    auto red=EnvBRDFApprox2({1,0,0},0,0);require(near(red.x,.0365463)&&red.y==0&&red.z==0);
    auto zero=EnvBRDFApprox2({0,0,0},0,0); require(zero.x==0&&zero.y==0&&zero.z==0);
    pass("nvidia_green_channel_bias_suppression");
    auto p=EnvBRDFApprox2({.8f,.35f,.1f},.2f,.85f),n=EnvBRDFApprox2({.8f,.35f,.1f},.2f,-.85f);
    require(p.x==n.x&&p.y==n.y&&p.z==n.z);
    pass("signed_view_cosine_symmetry");
}
