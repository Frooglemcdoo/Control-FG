"""Check native input extraction against the authenticated centered shader IR."""
import argparse
import base64
import hashlib
import json
import re
import subprocess
from pathlib import Path
from rt_spot_capture_inputs import transform, EXPORT, remove_payload_setup, PAYLOAD_SETUP
from rt_spot_blocker import BASE_SHA
root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser();parser.add_argument('--dxc');args=parser.parse_args()
manifest=json.loads((root/'assets/rt-spot-capture-inputs/manifest.json').read_text())
data=base64.b64decode((root/'assets/rt-spot-capture-inputs/rt_spot_capture_inputs.dxil.b64').read_text())
assert hashlib.sha256(data).hexdigest()==manifest['sha256']
assert manifest['baseline_sha256']==BASE_SHA and manifest['record_bytes']==48 and not manifest['traces_rays']
for size in (1,2,63,64,65,1920,2560,3840,16384):
    samples=[(2*i+1)*size//128 for i in range(64)]
    assert all(0<=x<size for x in samples)
    assert samples==sorted(samples)
    if size>=64:assert len(set(samples))==64
if args.dxc:
    baseline=base64.b64decode((root/'assets/rt-spot/rt_spot_full.dxil.b64').read_text())
    assert hashlib.sha256(baseline).hexdigest()==BASE_SHA
    path=root/'build/capture-input-baseline.dxil';path.write_bytes(baseline)
    source=subprocess.check_output([args.dxc,'-dumpbin',str(path)],text=True)
    result=transform(source)
    start=source.index('define void @"'+EXPORT+'"()')
    original=source[source.index('\n',start)+1:source.index('  call void @dx.op.traceRay.',start)]
    original=remove_payload_setup(original)
    expected=re.sub(r'; <label>:(\d+)[^\n]*',r'native\1:',original)
    expected=re.sub(r'%(\d+)\b',r'%native\1',expected).replace('label %.loopexit32','label %captureEnd')
    expected=re.sub(r'  %DispatchRaysIndex(?:164)? = call i32 @dx.op.dispatchRaysIndex.i32[^\n]*\n','',expected)
    assert expected in result, 'Native ray-construction instruction changed'
    body=result[result.index('define void @"captureNativeInputs"'):]
    body=body[:body.index('\n}')]
    assert 'call void @dx.op.traceRay' not in body and 'textureStore' not in body
    assert body.count('call void @dx.op.rawBufferStore.i32')==6
    assert 'i32 %native147' in body and 'i32 %captureTag' in body
    assert 'float %SpotSegment0Max' in body
    assert 'alloca' not in body and 'ShadowHitData' not in body
    # R131 differs from R128 extraction only by four unused payload instructions.
    legacy=transform(source,remove_payload=False)
    for line in PAYLOAD_SETUP:
        renamed=re.sub(r'%(\d+)\b',r'%native\1',line)
        assert legacy.count(renamed)==1
        legacy=legacy.replace(renamed,'',1)
    assert legacy==result, 'Ray construction changed during payload cleanup'
    prepared=root/'build/rt_spot_capture_inputs.dxil';prepared.write_bytes(data)
    linked=subprocess.check_output([args.dxc,'-dumpbin',str(prepared)],text=True)
    assert len(re.findall(r'^define ',linked,re.M))==1
    for forbidden in ('alloca','ShadowHitData','traceRay','g_rtScene','g_rwtSingleSpotShadow','g_rwtSpotPointShadowHitT','g_cbRaytracingHit','shadowAlphaTestAnyHit'):
        assert forbidden not in linked, forbidden
    assert linked.count('call void @dx.op.rawBufferStore.i32')==6
    assert manifest['payload_setup_removed'] and manifest['linked_export_only']
    for before,after in [('label %.loopexit32','label %other'),('float %SpotSegment0Max, %struct.ShadowHitData','float %wrong, %struct.ShadowHitData')]:
        try:transform(source.replace(before,after))
        except ValueError:pass
        else:raise AssertionError('Changed native shader accepted')
print('PASS payload-free single-export library, unchanged R128 ray calculations, native input shader hash, bounded grid mapping, exact centered instruction extraction, no native output stores or ray traversal')
