"""R187: attribute reflected diffuse versus environment specular.

Environment-only output receives a labeled 8x diagnostic boost after the
original-total cap. Not a proposed rendering fix or performance experiment.
"""
import base64,hashlib,json
from spec_hit_view import ROOT,op,f,view_block
from spec_parallel import SOURCE_SHA,instructions,inst,src,dst,imm
from dxbc_transform import transform_shex


def blocks(environment):
    return {
        404:inst('mov',dst(43,'xyz'),src(29,'xyzx') if environment else imm(0)),
        942:op(56,dst(43,'xyz'),src(21,'xywx'),src(29,'xyzx')) if environment else [],
        953:[] if environment else op(56,dst(43,'xyz'),src(14,'xywx'),src(16,'xyzx')),
    }


def tail(a,environment):
    out=[]
    for i in range(956,len(a)):
        ins=a[i].copy()
        if i==957:ins[4]=43
        if i==960:ins[-1]=43
        if i==961 and environment:out+=op(56,dst(14,'xyw'),src(14,'xyww'),f(8))
        out+=ins
    return out


def component(data,environment):
    if hashlib.sha256(data).hexdigest()!=SOURCE_SHA:raise ValueError('Unsupported native SPEC shader')
    def patch(w):
        a=instructions(w);assert len(a)==984 and a[41]==[0x02000068,43]
        out=[];additions=blocks(environment)
        for i,original in enumerate(a[:956]):
            if i in (405,406,444,445):continue
            if i==943:out+=a[405]+a[406]
            out+=additions.get(i,[])
            ins=original.copy()
            if i==41:ins[1]=44
            out+=view_block(a) if i==443 else ins
        out+=tail(a,environment)
        w[:]=[w[0],len(out)+2,*out]
    return transform_shex(data,patch)


def fixture(data,environment,reference,miss=False):
    def patch(w):
        a=instructions(w);body=[]
        body+=op(86,dst(40,'xyz'),src(1,'xyzx'))
        body+=op(50,dst(40,'xyz'),src(40,'xyzx'),f(.25),f(.5))
        for reg,swizzle,scale in [(14,'xyw',1),(16,'xyz',.5),(21,'xyw',.25),(29,'xyz',2),(34,'xyz',4),(19,'xyz',.125)]:
            body+=op(56,dst(reg,swizzle),src(40,'xyzz' if swizzle=='xyw' else 'xyzx'),f(scale))
        if reference:
            if miss:
                body+=op(56,dst(43,'xyz'),src(40,'xyzx'),f(2 if environment else 0))
            else:
                body+=op(56,dst(43,'xyz'),src(40,'xyzx'),f(.5))
                body+=op(56,dst(43,'xyz'),src(43,'xyzx'),src(40,'xyzx'))
            if environment:body+=op(56,dst(43,'xyz'),src(43,'xyzx'),f(8))
        else:
            body+=blocks(environment)[404]
            if not miss:body+=blocks(environment)[942]+blocks(environment)[953]
        if miss and not environment:
            # Test-only nonzero witness: verifies that the isolated diffuse
            # miss is zero without triggering the harness's vacuous-test guard.
            body+=op(0,dst(43,'xyz'),src(43,'xyzx'),f(.125))
        if not miss:body+=a[942]+a[953]+a[954]
        # Reference applies gain to the component before cap scaling; production
        # applies it after. Power-of-two gain preserves the same finite result.
        out=sum(a[:143],[])+body+tail(a,environment and not reference)
        out[sum(len(x) for x in a[:41])+1]=44
        w[:]=[w[0],len(out)+2,*out]
    return transform_shex(data,patch)


def main():
    data=base64.b64decode((ROOT/'research/spec-performance/spec-resolve.dxbc.b64').read_text())
    build=ROOT/'build';build.mkdir(exist_ok=True);header='#pragma once\n';report={}
    for name,environment in [('Diffuse',False),('Environment',True)]:
        result=component(data,environment)
        (build/f'spec-indirect-{name.lower()}.dxbc').write_bytes(result)
        header+=f'inline constexpr unsigned char kSpecIndirect{name}[]={{\n'+','.join(map(str,result))+'\n};\n'
        report[name]={'sha256':hashlib.sha256(result).hexdigest(),'bytes':len(result),'diagnostic_gain':8 if environment else 1}
        for miss in (False,True):
            for reference in (False,True):
                (build/f'spec-indirect-{name.lower()}-{int(miss)}-{int(reference)}.dxbc').write_bytes(fixture(data,environment,reference,miss))
    (build/'spec_indirect_components.generated.h').write_text(header)
    (build/'spec-indirect-validation.json').write_text(json.dumps({'diagnostic_only':True,'source_sha256':SOURCE_SHA,'variants':report},indent=2)+'\n')
    print(json.dumps(report))


if __name__=='__main__':main()
