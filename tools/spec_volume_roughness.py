"""R189: roughness-dependent volume specular approximation using existing reads.

Blend volume lighting evaluated along reflected direction toward its already
computed normal-direction value. This is a biased low-cost approximation,
not an exact GGX prefilter. Only secondary hits in the volume route change.
"""
import base64,hashlib,json
from spec_hit_view import ROOT,op,f,neg,optimize as hit_view
from spec_parallel import instructions,inst,src,dst,imm
from dxbc_transform import transform_shex


def extract(data):
    a=[];transform_shex(data,lambda w:a.extend(instructions(w)));return a


def blend_block():
    sat=op(0,dst(7,'w'),neg(22,'yyyy'),f(1));sat[0]|=0x2000
    return (sat+op(56,dst(7,'w'),src(7,'wwww'),src(7,'wwww'))+
            op(55,dst(7,'w'),src(6,'wwww'),f(0),src(7,'wwww'))+
            op(0,dst(25,'xyz'),src(27,'xyzx'),neg(29))+
            op(50,dst(29,'xyz'),src(25,'xyzx'),src(7,'wwww'),src(29,'xyzx')))


def optimize(data):
    baseline=hit_view(data) # rejects unknown native source
    native=extract(data)
    def patch(w):
        a=instructions(w);out=[]
        assert a.count(native[376])==1
        for ins in a:
            out+=ins
            if ins==native[376]:out+=blend_block()
        w[:]=[w[0],len(out)+2,*out]
    return transform_shex(baseline,patch)


def fixture(data,smoothness,miss,reference):
    def patch(w):
        a=instructions(w);body=[]
        body+=op(86,dst(40,'xyz'),src(1,'xyzx'))
        body+=op(50,dst(40,'xyz'),src(40,'xyzx'),f(.25),f(.5))
        body+=op(56,dst(27,'xyz'),src(40,'xyzx'),f(.5))
        body+=op(56,dst(29,'xyz'),src(40,'xyzx'),f(2))
        body+=inst('mov',dst(22,'y'),f(smoothness))
        body+=inst('mov',dst(6,'w'),imm(1 if miss else 0))
        if reference:
            t=0 if miss else min(1,max(0,1-smoothness))**2
            body+=op(56,dst(29,'xyz'),src(40,'xyzx'),f(2-1.5*t))
        else:body+=blend_block()
        out=sum(a[:143],[])+body+sum(a[956:],[])
        w[:]=[w[0],len(out)+2,*out]
    return transform_shex(data,patch)


def main():
    data=base64.b64decode((ROOT/'research/spec-performance/spec-resolve.dxbc.b64').read_text())
    result=optimize(data);build=ROOT/'build';build.mkdir(exist_ok=True)
    (build/'spec-volume-roughness.dxbc').write_bytes(result)
    (build/'spec_volume_roughness.generated.h').write_text('inline constexpr unsigned char kSpecVolumeRoughness[]={\n'+','.join(map(str,result))+'\n};\n')
    for smoothness in (0,.5,1):
        for miss in (False,True):
            for reference in (False,True):
                (build/f'spec-volume-{int(smoothness*2)}-{int(miss)}-{int(reference)}.dxbc').write_bytes(fixture(data,smoothness,miss,reference))
    report=dict(sha256=hashlib.sha256(result).hexdigest(),bytes=len(result),extra_instructions=5,extra_temporaries=0,extra_texture_reads=0,extra_rays=0,brightness_gain=1,weight='saturate(1-secondary_smoothness)^2; miss=0',exact_ggx_prefilter=False,hardware_quality_verified=False)
    (build/'spec-volume-roughness-validation.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))

if __name__=='__main__':main()
