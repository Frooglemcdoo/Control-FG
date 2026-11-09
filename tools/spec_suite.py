"""R191 unboosted attribution variants. Preserve the R184 full baseline."""
import base64
from spec_hit_view import ROOT,optimize,view_block,op,f
from spec_parallel import instructions,dst,src
from spec_local_components import component as local
from spec_indirect_components import component as indirect
from spec_environment_sources import source_shader
from spec_components import component as emission
from dxbc_transform import transform_shex

def unboost(data):
    def patch(w):
        a=instructions(w);gain=op(56,dst(14,'xyw'),src(14,'xyww'),f(8))
        assert a.count(gain)==1
        out=sum((i for i in a if i!=gain),[]);w[:]=[w[0],len(out)+2,*out]
    return transform_shex(data,patch)
def corrected_emission(data):
    native=[];transform_shex(data,lambda w:native.extend(instructions(w)))
    def patch(w):
        out=[]
        for i in instructions(w):
            if i in (native[444],native[445]):continue
            out+=view_block(native) if i==native[443] else i
        w[:]=[w[0],len(out)+2,*out]
    return transform_shex(emission(data,True),patch)
def variants(data):
    return [optimize(data),local(data,True),unboost(source_shader(data,True)),unboost(source_shader(data,False)),corrected_emission(data),indirect(data,False)]
def main():
    data=base64.b64decode((ROOT/'research/spec-performance/spec-resolve.dxbc.b64').read_text())
    build=ROOT/'build';build.mkdir(exist_ok=True);out='#pragma once\n'
    for n,v in enumerate(variants(data)):
        (build/f'spec-suite-{n}.dxbc').write_bytes(v)
        out+=f'inline constexpr unsigned char kSpecSuite{n}[]={{'+','.join(map(str,v))+'};\n'
    out+='inline constexpr D3D12_SHADER_BYTECODE kSpecSuiteShaders[]={'+','.join('{kSpecSuite%d,sizeof(kSpecSuite%d)}'%(n,n) for n in range(6))+'};\n'
    (build/'spec_suite.generated.h').write_text(out)
if __name__=='__main__':main()
