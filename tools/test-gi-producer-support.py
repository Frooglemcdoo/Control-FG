"""Audit real pinned DXIL, then ensure support/geometry mutations are rejected."""
import argparse
import base64
from pathlib import Path
import subprocess
import tempfile
from gi_long_sequence import ROOT
from gi_producer_support import audit_source

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--dxc',required=True)
a=p.parse_args()
with tempfile.TemporaryDirectory() as tmp:
    for variant in (0,1):
        path=Path(tmp)/'shader.dxbc'
        path.write_bytes(base64.b64decode((ROOT/f'research/gi-sequence-r264/gi-ray-{variant}.dxbc.b64').read_bytes()))
        source=subprocess.check_output([a.dxc,'-dumpbin',str(path)],text=True)
        assert not audit_source(source,variant)['reuse_ready']
        for old,new in [('0x3FD99999C0000000','0x3FE0000000000000'),
                        ('0xBF847AE140000000','0x0000000000000000'),
                        ('%.i0107 = fmul fast float %99, %15','%.i0107 = fmul fast float %99, %12'),
                        (', i32 0, i32 1, i32 0, i32 2, i32 0, float ', ', i32 1, i32 1, i32 0, i32 2, i32 0, float ')]:
            assert old in source
            try: audit_source(source.replace(old,new),variant)
            except ValueError: pass
            else: raise AssertionError('Accepted changed producer: '+old)
print('PASS both native variants and eight mutated range/position/normal/TraceRay contracts')
