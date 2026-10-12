// Executes the shader's scalar clamp functions, not a separately copied model.
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
using std::min;
using std::max;
#include "../src/shaders/pt_reflection_energy.hlsli"
struct RGB {float r,g,b;};
static float lum(RGB v){return (v.r+v.g+v.b)*0.3333f;}
static RGB add(RGB a,RGB b){return {a.r+b.r,a.g+b.g,a.b+b.b};}
static RGB mul(RGB a,float s){return {a.r*s,a.g*s,a.b*s};}
static bool near(float a,float b){return std::abs(a-b)<0.00001f*max(1.0f,max(std::abs(a),std::abs(b)));}
int main(){
    const RGB retainedCases[]{{0.8f,0.3f,0.1f},{4,1,0.2f},{0,0,0},{0.02f,0.5f,1.4f}};
    const RGB oldCases[]{{0.1f,0.2f,0.3f},{2,4,8},{0,0,0}};
    const RGB newCases[]{{0,0,0},{100,0,0},{0,100,0},{0,0,100},{1,2,3}};
    const float limits[]{-1,0,0.1f,1,10};
    unsigned cases=0;
    for(auto retained:retainedCases)for(auto oldSpec:oldCases)for(float limit:limits){
        RGB native=add(retained,oldSpec);
        float ns=PTReflectionNativeScale(lum(native),limit);
        float same=PTReflectionReplacementScale(lum(retained),lum(oldSpec),ns,limit);
        assert(near(same,ns)); // equal replacement reproduces native
        for(auto bounce:newCases){
            float bs=PTReflectionReplacementScale(lum(retained),lum(bounce),ns,limit);
            RGB keep=mul(retained,ns),out=add(keep,mul(bounce,bs));
            assert(out.r>=keep.r&&out.g>=keep.g&&out.b>=keep.b);
            assert(bs>=0&&bs<=ns+0.000001f);
            if(limit>=0)assert(lum(out)<=limit+0.00001f);
            if(limit<0)assert(ns==1&&bs==1);
            ++cases;
        }
    }
    // Reproduce the old failure: a hot red bounce erases green/blue diffuse.
    RGB kept{0.8f,0.3f,0.1f},old{0.1f,0.2f,0.3f},hot{100,0,0};
    float nativeScale=PTReflectionNativeScale(lum(add(kept,old)),1);
    float oldScale=PTReflectionNativeScale(lum(add(kept,hot)),1);
    assert(oldScale<nativeScale*0.1f);
    float newScale=PTReflectionReplacementScale(lum(kept),lum(hot),nativeScale,1);
    RGB result=add(mul(kept,nativeScale),mul(hot,newScale));
    assert(near(result.g,kept.g*nativeScale)&&near(result.b,kept.b*nativeScale));
    std::printf("PASS: %u shader-function cases; hot-bounce regression, retained RGB, equal/black replacement, and clamp budget\n",cases);
}
