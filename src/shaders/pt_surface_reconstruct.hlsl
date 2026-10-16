#include "ControlPrimaryGuideDecode.hlsli"
cbuffer Reconstruction : register(b0) {
    float4 current[18];
    float4 previousWorldToView[4];
    float4 previousViewToClip[4];
    uint width,height,useHistory,maximumHistory;
    uint signalStride;uint3 reserved;
};
ByteAddressBuffer rawRadiance : register(t0);
ByteAddressBuffer history : register(t1);
Texture2D<float4> gbuffer1 : register(t2);
Texture2D<float4> gbuffer2 : register(t3);
Texture2D<float4> clipDepth : register(t4);
ByteAddressBuffer previousMaterialKeys : register(t5);
StructuredBuffer<uint2> materialFlags : register(t6);
struct MaterialRecord { uint words[84]; };
StructuredBuffer<MaterialRecord> materialRecords : register(t7);
ByteAddressBuffer geometryKeys : register(t8);
RWByteAddressBuffer materialKeys : register(u2);
ByteAddressBuffer previousSignals : register(t9);
RWByteAddressBuffer signals : register(u3);
RWByteAddressBuffer scratch : register(u4);
float Luma(float3 v){return dot(v,float3(.2126,.7152,.0722));}
bool SplitSignals(){return signalStride==64;}
float4 Raw(uint2 p){return asfloat(rawRadiance.Load4((p.y*width+p.x)*(SplitSignals()?64:16)));}
float3 Factor(uint2 p,bool spec){return asfloat(rawRadiance.Load4((p.y*width+p.x)*64+(spec?48:32))).rgb;}
float3 Noisy(uint2 p,bool spec){return asfloat(rawRadiance.Load4((p.y*width+p.x)*64+(spec?16:0))).rgb/Factor(p,spec);}

cbuffer Phase : register(b1) { uint materialPhase; };
RWByteAddressBuffer result : register(u0);
RWByteAddressBuffer admission : register(u1);
void Measure(uint2 p,uint counter,uint amount){
    if(((p.x-1)|(p.y-1))%64==0){uint old;admission.InterlockedAdd(100+counter*4,amount,old);}
}
uint Index(uint2 p){return p.y*width+p.x;}
float3 Position(uint2 p){
    float2 uv=(float2(p)+.5)/float2(width,height);
    float4 v=(uv.x*2-1)*current[14]+(1-uv.y*2)*current[15]+clipDepth.Load(int3(p,0)).x*current[16]+current[17];
    return v.xyz/v.w;
}
float3 Normal(uint2 p){return ControlNormalViewToWorld(ControlDecodeNormalView(gbuffer1.Load(int3(p,0))),current[6].xyz,current[7].xyz,current[8].xyz);}
uint PackNormal(float3 n){uint3 v=(uint3)round(saturate(n*.5+.5)*1023);return v.x|(v.y<<10)|(v.z<<20);}
float3 UnpackNormal(uint n){return normalize(float3(n&1023,(n>>10)&1023,(n>>20)&1023)*(2.0/1023)-1);}
// Two edge-aware wavelet scales. Temporal channels remain untouched for next
// frame; only the display result is filtered. Variance is propagated with w^2.
void FilterSignals(uint2 p,uint phase){
    uint at=Index(p)*32;if(asfloat(result.Load(at+12))!=1)return;
    float3 position=Position(p),normal=Normal(p);
    uint material=ControlDecodeMaterialID(gbuffer2.Load(int3(p,0)));
    float roughness=ControlDecodeMaterialRoughness(gbuffer1.Load(int3(p,0)));
    float3 combined=0;
    [unroll] for(uint lobe=0;lobe<2;++lobe){
        uint offset=lobe*16;float4 center=phase==2?asfloat(signals.Load4(at+offset)):asfloat(scratch.Load4(at+offset));
        float centerL=Luma(center.rgb),variance=phase==2?max(center.w-centerL*centerL,0):max(center.w,0);
        // Bootstrap variance after disocclusion; a single sample has no temporal variance.
        if(phase==2){float mean=0,second=0,count=0;
            [unroll] for(int y=-1;y<=1;++y)[unroll] for(int x=-1;x<=1;++x){
                int2 q=int2(p)+int2(x,y);if(any(q<0)||q.x>=(int)width||q.y>=(int)height)continue;
                if(Raw((uint2)q).w!=1||ControlDecodeMaterialID(gbuffer2.Load(int3(q,0)))!=material)continue;
                float value=Luma(Noisy((uint2)q,lobe!=0));mean+=value;second+=value*value;count+=1;
            }
            if(count>0)variance=max(variance,max(second/count-(mean/count)*(mean/count),0)/max(asfloat(result.Load(at+28)),1));
        }
        float3 sum=0;float total=0,varSum=0;int step=phase==2?1:2;
        [unroll] for(int y=-1;y<=1;++y)[unroll] for(int x=-1;x<=1;++x){
            int2 q=int2(p)+int2(x,y)*step;if(any(q<0)||q.x>=(int)width||q.y>=(int)height)continue;
            uint address=Index((uint2)q)*32;
            if(asfloat(result.Load(address+12))!=1||ControlDecodeMaterialID(gbuffer2.Load(int3(q,0)))!=material)continue;
            float alignment=dot(Normal((uint2)q),normal);
            float plane=abs(dot(Position((uint2)q)-position,ControlDecodeNormalView(gbuffer1.Load(int3(p,0)))));
            if(alignment<.95||plane>max(.002,abs(position.z)*.001))continue;
            float4 value=phase==2?asfloat(signals.Load4(address+offset)):asfloat(scratch.Load4(address+offset));
            float valueL=Luma(value.rgb),valueVar=phase==2?max(value.w-valueL*valueL,0):max(value.w,0);
            float weight=(x==0?2:1)*(y==0?2:1)*pow(saturate(alignment),lobe==0?16:64);
            weight*=exp(-abs(valueL-centerL)/(4*sqrt(max(variance,1e-8))+.001));
            if(lobe!=0){
                float r=ControlDecodeMaterialRoughness(gbuffer1.Load(int3(q,0)));
                weight*=exp(-abs(r-roughness)*32);
                float h=asfloat(rawRadiance.Load(Index(p)*64+28)),qh=asfloat(rawRadiance.Load(Index((uint2)q)*64+28));
                weight*=exp(-abs(h-qh)/max(.01,max(h,qh)*max(roughness,.02)));
            }
            sum+=value.rgb*weight;total+=weight;varSum+=valueVar*weight*weight;
        }
        float3 value=total>0?sum/total:center.rgb;
        if(phase==2)scratch.Store4(at+offset,asuint(float4(value,total>0?varSum/(total*total):variance)));
        else combined+=value*Factor(p,lobe!=0);
    }
    if(phase==3&&all(isfinite(combined))&&all(combined>=0))result.Store3(at,asuint(combined));
}
[numthreads(8,8,1)]
void main(uint3 id:SV_DispatchThreadID,uint3 group:SV_GroupID,uint lane:SV_GroupIndex){
    // The small first dispatch fingerprints each table record once, not per pixel.
    if(materialPhase==0){
        uint m=group.x*64+lane;if(m>=6971)return;
        if(admission.Load(32)!=1)return;
        uint at=m*16;materialKeys.Store4(at,0);
        uint2 geometry=geometryKeys.Load2(m*8);if(all(geometry==0))return;
        uint count,stride,flagsCount,flagsStride;
        materialRecords.GetDimensions(count,stride);materialFlags.GetDimensions(flagsCount,flagsStride);
        if(m>=count||m>=flagsCount||stride!=336||flagsStride!=8)return;
        uint2 hash=uint2(2166136261u,2246822519u),flags=materialFlags[m];
        hash=(hash^flags.x)*uint2(16777619u,3266489917u);
        hash=(hash^flags.y)*uint2(16777619u,3266489917u);
        [loop] for(uint n=0;n<84;++n)hash=(hash^materialRecords[m].words[n])*uint2(16777619u,3266489917u);
        materialKeys.Store4(at,uint4(geometry,hash));return;
    }
    uint2 p=id.xy;if(p.x>=width||p.y>=height)return;
    // Replayed/rejected producers cannot read old guides or scene resources.
    if(admission.Load(32)!=1)return;
    if(materialPhase>=2){if(SplitSignals())FilterSignals(p,materialPhase);return;}
    uint at=Index(p)*32;float4 center=Raw(p);
    signals.Store4(at,0);signals.Store4(at+16,0);
    result.Store4(at,0);result.Store4(at+16,0);
    if(center.w!=1||!all(isfinite(center))||any(center.rgb<0))return;
    float3 position=Position(p),normal=Normal(p);
    uint material=ControlDecodeMaterialID(gbuffer2.Load(int3(p,0)));
    uint4 materialKey=material<6971?materialKeys.Load4(material*16):uint4(0,0,0,0);
    float roughness=ControlDecodeMaterialRoughness(gbuffer1.Load(int3(p,0)));
    if(!all(isfinite(position))||!all(isfinite(normal)))return;
    Measure(p,0,1);
    float3 sum=0;float total=0;
    // Same-material/normal/plane rejection protects object and shadow boundaries.
    // Smooth specular surfaces retain their local sample rather than spreading
    // reflection detail over a diffuse-sized kernel.
    bool split=SplitSignals();
    int radius=split?0:(roughness<.25?0:2);
    [loop] for(int y=-radius;y<=radius;++y)[loop] for(int x=-radius;x<=radius;++x){
        int2 q=int2(p)+int2(x,y);if(any(q<0)||q.x>=(int)width||q.y>=(int)height)continue;
        float4 value=Raw((uint2)q);
        if(value.w!=1||!all(isfinite(value))||any(value.rgb<0)||ControlDecodeMaterialID(gbuffer2.Load(int3(q,0)))!=material)continue;
        float3 n=Normal((uint2)q),v=Position((uint2)q);
        float alignment=dot(n,normal);
        // Positions are view space; use the matching view-space normal here.
        float plane=abs(dot(v-position,ControlDecodeNormalView(gbuffer1.Load(int3(p,0)))));
        if(!all(isfinite(v))||alignment<.95||plane>max(.002,abs(position.z)*.001))continue;
        float weight=exp(-.35*(x*x+y*y))*pow(saturate(alignment),16);
        sum+=value.rgb*weight;total+=weight;
    }
    float3 filtered=total>0?sum/total:center.rgb;float samples=1;
    float3 diffuse=split?Noisy(p,false):filtered,specular=split?Noisy(p,true):0;
    float2 moments=float2(Luma(diffuse)*Luma(diffuse),Luma(specular)*Luma(specular));
    if(useHistory!=0){
        Measure(p,1,1);
        float3 world=position.x*current[6].xyz+position.y*current[7].xyz+position.z*current[8].xyz+current[9].xyz;
        float3 previous=world.x*previousWorldToView[0].xyz+world.y*previousWorldToView[1].xyz+world.z*previousWorldToView[2].xyz+previousWorldToView[3].xyz;
        float4 clip=previous.x*previousViewToClip[0]+previous.y*previousViewToClip[1]+previous.z*previousViewToClip[2]+previousViewToClip[3];
        float2 uv=float2(clip.x,-clip.y)/clip.w*.5+.5;
        if(all(isfinite(uv))&&clip.w>0&&all(uv>=0)&&all(uv<1)){
            Measure(p,2,1);
            // Reprojection lands between texel centers. Validate each of the
            // four bilinear taps independently, then renormalize only survivors.
            // A rejected nearest tap must not discard another valid surface tap.
            float2 texel=uv*float2(width,height)-.5;
            int2 base=(int2)floor(texel);float2 fraction=frac(texel);
            float3 oldSum=0,oldDiffuse=0,oldSpecular=0;float2 oldMoments=0;float oldWeight=0,oldCount=0;
            bool covered=false,compatible=false,materialMatch=false,normalMatch=false;
            bool nearestAccepted=false,stableMatch=false,reindexedAccepted=false;
            uint2 nearest=(uint2)(uv*float2(width,height));
            [unroll] for(int y=0;y<2;++y)[unroll] for(int x=0;x<2;++x){
                int2 q=base+int2(x,y);
                float weight=(x?fraction.x:1-fraction.x)*(y?fraction.y:1-fraction.y);
                if(weight<=0||any(q<0)||q.x>=(int)width||q.y>=(int)height)continue;
                uint offset=Index((uint2)q)*32;
                float4 old=asfloat(history.Load4(offset));uint4 guide=history.Load4(offset+16);
                if(old.w!=1)continue;
                covered=true;
                bool sameIndex=guide.y==material;
                uint4 oldKey=guide.y<6971?previousMaterialKeys.Load4(guide.y*16):uint4(0,0,0,0);
                bool sameMaterial=any(materialKey.xy!=0)&&all(oldKey==materialKey);
                stableMatch=stableMatch||sameMaterial;
                bool sameNormal=dot(UnpackNormal(guide.x),normal)>.95;
                materialMatch=materialMatch||sameIndex;normalMatch=normalMatch||sameNormal;
                if(!sameMaterial||!sameNormal)continue;
                compatible=true;
                float oldDepth=asfloat(guide.z),count=asfloat(guide.w);
                if(!all(isfinite(old))||any(old.rgb<0)||!isfinite(count)||count<1||count>(float)maximumHistory||
                   !isfinite(oldDepth)||abs(oldDepth-previous.z)>max(.003,abs(previous.z)*.002))continue;
                if(!sameIndex)reindexedAccepted=true;
                oldSum+=old.rgb*weight;oldCount+=count*weight;oldWeight+=weight;
                if(split){float4 d=asfloat(previousSignals.Load4(offset)),s=asfloat(previousSignals.Load4(offset+16));
                    oldDiffuse+=d.rgb*weight;oldSpecular+=s.rgb*weight;oldMoments+=float2(d.w,s.w)*weight;}
                if(all((uint2)q==nearest))nearestAccepted=true;
            }
            if(covered)Measure(p,3,1);
            if(compatible)Measure(p,4,1);
            if(materialMatch)Measure(p,7,1);
            if(normalMatch)Measure(p,8,1);
            if(stableMatch)Measure(p,10,1);
            // Ignore infinitesimal edge overlap rather than amplifying a tap
            // whose bilinear weight is only floating-point projection noise.
            if(oldWeight>=.01){
                samples=min(oldCount/oldWeight+1,(float)maximumHistory);
                float motion=length(uv*float2(width,height)-(float2(p)+.5));
                if((!split&&radius==0)||motion>1)samples=min(samples,8);
                filtered=lerp(oldSum/oldWeight,filtered,1/samples);
                if(split){diffuse=lerp(oldDiffuse/oldWeight,diffuse,1/samples);
                    specular=lerp(oldSpecular/oldWeight,specular,1/samples);moments=lerp(oldMoments/oldWeight,moments,1/samples);}
                Measure(p,5,1);Measure(p,6,(uint)samples);
                if(!nearestAccepted)Measure(p,9,1);
                if(reindexedAccepted)Measure(p,11,1);
            }
        }
    }
    if(split){
        filtered=diffuse*Factor(p,false)+specular*Factor(p,true);
        if(!all(isfinite(diffuse))||!all(isfinite(specular))||!all(isfinite(moments)))return;
        signals.Store4(at,asuint(float4(diffuse,moments.x)));signals.Store4(at+16,asuint(float4(specular,moments.y)));
    }
    if(!all(isfinite(filtered))||any(filtered<0))return;
    result.Store4(at,asuint(float4(filtered,1)));
    result.Store4(at+16,uint4(PackNormal(normal),material,asuint(position.z),asuint(samples)));
}
