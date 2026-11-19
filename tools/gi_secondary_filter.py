"""Opt-in compiled-copy patch: secondary base color only, pinned native source intact."""
import hashlib
from pathlib import Path
SOURCE=Path(__file__).resolve().parents[1]/'src/shaders/gi_evaluator_secondary.generated.hlsl'
# The generated translation remains audited by test-gi-secondary.py.
def patch(source,paired=False):
    lines=source.splitlines();hits=0
    for i,line in enumerate(lines):
        if line.endswith('// native dword 2338'):
            needle='asfloat(uint4(0x0u,0x0u,0x0u,0x0u)).x'
            if 'g_sMaterialTextureArray' not in line or 'g_sLinearWrap' not in line or line.count(needle)!=1:
                raise ValueError('Secondary base-color sample contract changed')
            lines[i]=line.replace(needle,'state.textureLod' if paired else 'GIMaterialTextureLod()');hits+=1
    if hits!=1:raise ValueError('Expected exactly one secondary base-color lookup')
    result='\n'.join(lines)+'\n'
    if paired:
        needle='struct GIShadingRegisters {'
        if result.count(needle)!=1:raise ValueError('Secondary register contract changed')
        result=result.replace(needle,needle+' float textureLod;')
    return result
