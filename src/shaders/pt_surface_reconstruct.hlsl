#include "ControlPrimaryGuideDecode.hlsli"
cbuffer Reconstruction : register(b0) {
    float4 current[18];
    float4 previousWorldToView[4];
    float4 previousViewToClip[4];
    uint width,height,useHistory,maximumHistory;
};
ByteAddressBuffer rawRadiance : register(t0);
ByteAddressBuffer history : register(t1);
Texture2D<float4> gbuffer1 : register(t2);
Texture2D<float4> gbuffer2 : register(t3);
Texture2D<float4> clipDepth : register(t4);
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
[numthreads(8,8,1)]
void main(uint3 id:SV_DispatchThreadID){
    uint2 p=id.xy;if(p.x>=width||p.y>=height)return;
    // Replayed/rejected producers cannot read old guides or scene resources.
    if(admission.Load(32)!=1)return;
    uint at=Index(p)*32;float4 center=asfloat(rawRadiance.Load4(Index(p)*16));
    result.Store4(at,0);result.Store4(at+16,0);
    if(center.w!=1||!all(isfinite(center))||any(center.rgb<0))return;
    float3 position=Position(p),normal=Normal(p);
    uint material=ControlDecodeMaterialID(gbuffer2.Load(int3(p,0)));
    float roughness=ControlDecodeMaterialRoughness(gbuffer1.Load(int3(p,0)));
    if(!all(isfinite(position))||!all(isfinite(normal)))return;
    Measure(p,0,1);
    float3 sum=0;float total=0;
    // Same-material/normal/plane rejection protects object and shadow boundaries.
    // Smooth specular surfaces retain their local sample rather than spreading
    // reflection detail over a diffuse-sized kernel.
    int radius=roughness<.25?0:2;
    [loop] for(int y=-radius;y<=radius;++y)[loop] for(int x=-radius;x<=radius;++x){
        int2 q=int2(p)+int2(x,y);if(any(q<0)||q.x>=(int)width||q.y>=(int)height)continue;
        float4 value=asfloat(rawRadiance.Load4(Index((uint2)q)*16));
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
            float3 oldSum=0;float oldWeight=0,oldCount=0;
            bool covered=false,compatible=false,materialMatch=false,normalMatch=false;
            bool nearestAccepted=false;
            uint2 nearest=(uint2)(uv*float2(width,height));
            [unroll] for(int y=0;y<2;++y)[unroll] for(int x=0;x<2;++x){
                int2 q=base+int2(x,y);
                float weight=(x?fraction.x:1-fraction.x)*(y?fraction.y:1-fraction.y);
                if(weight<=0||any(q<0)||q.x>=(int)width||q.y>=(int)height)continue;
                uint offset=Index((uint2)q)*32;
                float4 old=asfloat(history.Load4(offset));uint4 guide=history.Load4(offset+16);
                if(old.w!=1)continue;
                covered=true;
                bool sameMaterial=guide.y==material;
                bool sameNormal=dot(UnpackNormal(guide.x),normal)>.95;
                materialMatch=materialMatch||sameMaterial;normalMatch=normalMatch||sameNormal;
                if(!sameMaterial||!sameNormal)continue;
                compatible=true;
                float oldDepth=asfloat(guide.z),count=asfloat(guide.w);
                if(!all(isfinite(old))||any(old.rgb<0)||!isfinite(count)||count<1||count>(float)maximumHistory||
                   !isfinite(oldDepth)||abs(oldDepth-previous.z)>max(.003,abs(previous.z)*.002))continue;
                oldSum+=old.rgb*weight;oldCount+=count*weight;oldWeight+=weight;
                if(all((uint2)q==nearest))nearestAccepted=true;
            }
            if(covered)Measure(p,3,1);
            if(compatible)Measure(p,4,1);
            if(materialMatch)Measure(p,7,1);
            if(normalMatch)Measure(p,8,1);
            // Ignore infinitesimal edge overlap rather than amplifying a tap
            // whose bilinear weight is only floating-point projection noise.
            if(oldWeight>=.01){
                samples=min(oldCount/oldWeight+1,(float)maximumHistory);
                float motion=length(uv*float2(width,height)-(float2(p)+.5));
                if(radius==0||motion>1)samples=min(samples,8);
                filtered=lerp(oldSum/oldWeight,filtered,1/samples);
                Measure(p,5,1);Measure(p,6,(uint)samples);
                if(!nearestAccepted)Measure(p,9,1);
            }
        }
    }
    if(!all(isfinite(filtered))||any(filtered<0))return;
    result.Store4(at,asuint(float4(filtered,1)));
    result.Store4(at+16,uint4(PackNormal(normal),material,asuint(position.z),asuint(samples)));
}
