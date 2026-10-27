"""R188: split the R187 environment-specular signal by native source route."""
import base64, hashlib, json
from spec_indirect_components import ROOT, component, fixture as indirect_fixture
from spec_hit_view import op, f
from spec_parallel import instructions, inst, src, dst, imm
from dxbc_transform import transform_shex


def extract(data):
    a=[];transform_shex(data,lambda w:a.extend(instructions(w)));return a


def tag(selected):
    return inst('mov',dst(44,'x'),imm(1 if selected else 0))


def gate():
    return op(55,dst(43,'xyz'),src(44,'xxxx'),src(43,'xyzx'),f(0))


def source_shader(data,volume):
    native=extract(data)
    result=component(data,True) # also validates exact native source SHA
    def patch(w):
        a=instructions(w);out=[]
        for ins in a:
            if ins==native[368]:out+=tag(volume)
            if ins==native[378]:out+=tag(not volume)
            if ins==native[956]:out+=gate()
            out+=([ins[0],45] if ins==[0x02000068,44] else ins)
        w[:]=[w[0],len(out)+2,*out]
    return transform_shex(result,patch)


def fixture(data,volume,reference,miss):
    native=extract(data)
    result=indirect_fixture(data,True,False,miss)
    def patch(w):
        a=instructions(w);out=[]
        for ins in a:
            if ins==native[956]:
                # Alternate source route across pixels, exercising both sides
                # of the native if/else on GPU in each fixture.
                out+=op(1,dst(7,'w'),src(1,'xxxx'),imm(1))
                if reference:
                    out+=op(55,dst(43,'xyz'),src(7,'wwww'),src(43,'xyzx') if volume else f(0),f(0) if volume else src(43,'xyzx'))
                else:
                    out+=native[367]+tag(volume)+native[377]+tag(not volume)+native[403]+gate()
            out+=([ins[0],45] if ins==[0x02000068,44] else ins)
        w[:]=[w[0],len(out)+2,*out]
    return transform_shex(result,patch)


def main():
    data=base64.b64decode((ROOT/'research/spec-performance/spec-resolve.dxbc.b64').read_text())
    build=ROOT/'build';build.mkdir(exist_ok=True);header='#pragma once\n';report={}
    for name,volume in [('Volume',True),('Cube',False)]:
        result=source_shader(data,volume)
        (build/f'spec-environment-{name.lower()}.dxbc').write_bytes(result)
        header+=f'inline constexpr unsigned char kSpecEnvironment{name}[]={{\n'+','.join(map(str,result))+'\n};\n'
        report[name]={'sha256':hashlib.sha256(result).hexdigest(),'bytes':len(result),'diagnostic_gain':8}
        for miss in (False,True):
            for reference in (False,True):
                (build/f'spec-environment-{name.lower()}-{int(miss)}-{int(reference)}.dxbc').write_bytes(fixture(data,volume,reference,miss))
    (build/'spec_environment_sources.generated.h').write_text(header)
    (build/'spec-environment-validation.json').write_text(json.dumps({'diagnostic_only':True,'variants':report},indent=2)+'\n')
    print(json.dumps(report))

if __name__=='__main__':main()
