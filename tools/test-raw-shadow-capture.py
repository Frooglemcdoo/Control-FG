"""Verify native binding contract and that the raw observer cannot write native textures."""
import argparse,base64,hashlib,re,subprocess
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--dxc',required=True);a=p.parse_args()
root=Path(__file__).resolve().parents[1]
data=base64.b64decode((root/'assets/rt-spot/rt_spot_full.dxil.b64').read_text())
assert hashlib.sha256(data).hexdigest()=='26688736d38b5ea056ae6bb37dc1a738334d94c784003a67957f3677f789a246'
path=root/'build/rt_spot_raw_contract.dxil';path.write_bytes(data)
native=subprocess.check_output([a.dxc,'-dumpbin',str(path)],text=True)
for name,register in [('g_rwtSpotPointShadowHitT','u0'),('g_rwtSingleSpotShadow','u1')]:
 line=next(l for l in native.splitlines() if re.match(r'; '+name+r'\s+UAV',l))
 assert line.split()[-2]==register,line
source=subprocess.check_output([a.dxc,'-dumpbin',str(root/'build/rt_spot_capture_raw.dxil')],text=True)
assert '@dx.op.textureStore' not in source
assert '@dx.op.traceRay' not in source
assert source.count('call %dx.types.ResRet.f32 @dx.op.textureLoad')==2
assert source.count('call void @dx.op.bufferStore.i32')==2
for name,register in [('nativeHitDistance','u0'),('nativeVisibility','u1'),('captureInputs','u2')]:
 line=next(l for l in source.splitlines() if re.match(r'; '+name+r'\s+UAV',l))
 assert line.split()[-2]==register,line
print('PASS raw observer: verified native u0/u1, two texture reads, private buffer writes only; no ray dispatch')
