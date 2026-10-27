"""Split secondary point/spot specular from other reflected lighting.

Preserves R184 direction correction and the full original per-hit cap basis.
This attributes noise; it is not an optimization or quality fix.
"""
import base64, hashlib, json
from dxbc_transform import transform_shex
from spec_parallel import SOURCE_SHA, instructions, inst, src, dst, imm
from spec_hit_view import ROOT, op, f, view_block


def blocks(local):
    return {
        404:inst('mov',dst(43,'xyz'),imm(0) if local else src(29,'xyzx')),
        942:inst('mov',dst(43,'xyz'),src(34,'xyzx')) if local else op(56,dst(43,'xyz'),src(21,'xywx'),src(29,'xyzx')),
        953:[] if local else op(50,dst(43,'xyz'),src(14,'xywx'),src(16,'xyzx'),src(43,'xyzx')),
        954:[] if local else op(0,dst(43,'xyz'),src(19,'xyzx'),src(43,'xyzx')),
    }


def tail(a):
    result=[]
    for i in range(956,len(a)):
        ins=a[i].copy()
        if i==957:ins[4]=43
        if i==960:ins[-1]=43
        result+=ins
    return result


def component(data,local):
    if hashlib.sha256(data).hexdigest()!=SOURCE_SHA:raise ValueError('Unsupported native SPEC shader')
    def patch(w):
        a=instructions(w);assert len(a)==984 and a[41]==[0x02000068,43]
        additions=blocks(local);out=[]
        for i,original in enumerate(a):
            if i>=956:break
            if i in (405,406,444,445):continue
            if i==943:out+=a[405]+a[406]
            out+=additions.get(i,[])
            ins=original.copy()
            if i==41:ins[1]=44
            out+=view_block(a) if i==443 else ins
        out+=tail(a)
        w[:]=[w[0],len(out)+2,*out]
    return transform_shex(data,patch)


def fixture(data,local,reference):
    def patch(w):
        a=instructions(w);body=[]
        # Controlled nonnegative diffuse, albedo, env factor, env radiance,
        # local specular and emission vary by pixel/sample. Exact binary values.
        body+=op(86,dst(40,'xyz'),src(1,'xyzx'))
        body+=op(50,dst(40,'xyz'),src(40,'xyzx'),f(.25),f(.5))
        for reg,swizzle,scale in [(14,'xyw',1),(16,'xyz',.5),(21,'xyw',.25),(29,'xyz',2),(34,'xyz',4),(19,'xyz',.125)]:
            # Match the xyw packed layout used by the production color math.
            body+=op(56,dst(reg,swizzle),src(40,'xyzz' if swizzle=='xyw' else 'xyzx'),f(scale))
        if reference:
            if local:body+=op(56,dst(43,'xyz'),src(40,'xyzx'),f(4))
            else:
                # D*A + E*F + emission = x*x + .125*x.
                body+=op(56,dst(43,'xyz'),src(40,'xyzx'),f(.125))
                body+=op(50,dst(43,'xyz'),src(40,'xyzx'),src(40,'xyzx'),src(43,'xyzx'))
        else:body+=blocks(local)[942]
        body+=a[942]
        if not reference:body+=blocks(local)[953]
        body+=a[953]
        if not reference:body+=blocks(local)[954]
        body+=a[954]
        out=sum(a[:143],[])+body+tail(a)
        # Fixture and production need the added temporary register.
        out[sum(len(x) for x in a[:41])+1]=44
        w[:]=[w[0],len(out)+2,*out]
    return transform_shex(data,patch)


def main():
    data=base64.b64decode((ROOT/'research/spec-performance/spec-resolve.dxbc.b64').read_text())
    build=ROOT/'build';build.mkdir(exist_ok=True);header='#pragma once\n';report={}
    for name,local in [('Local',True),('Other',False)]:
        result=component(data,local)
        (build/f'spec-local-{name.lower()}.dxbc').write_bytes(result)
        header+=f'inline constexpr unsigned char kSpecLocal{name}[]={{\n'+','.join(map(str,result))+'\n};\n'
        report[name]={'sha256':hashlib.sha256(result).hexdigest(),'bytes':len(result)}
        for reference in (False,True):
            (build/f'spec-local-{name.lower()}-{int(reference)}.dxbc').write_bytes(fixture(data,local,reference))
    (build/'spec_local_components.generated.h').write_text(header)
    (build/'spec-local-validation.json').write_text(json.dumps({'diagnostic_only':True,'source_sha256':SOURCE_SHA,'variants':report},indent=2)+'\n')
    print(json.dumps(report))


if __name__=='__main__':main()
