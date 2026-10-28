"""Exact IR scope plus bounded index coverage; not an image quality test."""
from spec_long_sequence import ROOT, OLD, NEW, transform
from spec_retry_reuse import transform as reuse
source=(ROOT/'research/native-integration-audit/reflection-000.ll').read_text()
patched=transform(source)
assert patched.replace(NEW,OLD)==reuse(source)
# Original retry still consumes %270, which still extracts register 69.y.
assert '%270 = extractvalue %dx.types.CBufRet.i32 %269, 1' in patched
assert '%293 = uitofp i32 %270 to float' in patched
for changed in (source.replace(OLD,OLD.replace('%139','%138')),source.replace('i32 1112, i32 7','i32 1116, i32 7')):
 try: transform(changed)
 except AssertionError: pass
 else: raise AssertionError('Changed source admitted')
for count in (1,2,4,8,12,16):
 for start in (0,1,255,256,50093,0xffffff80):
  indices=[(((start+frame)&0xffffffff)&255)*count+sample for frame in range(256) for sample in range(count)]
  assert len(set(indices))==256*count
  assert min(indices)==0 and max(indices)==256*count-1
  assert all((((start+frame)&255)*count+sample)==(((start+frame+256)&255)*count+sample) for frame in range(256) for sample in range(count))
assert 255*16+15==4095
print('PASS: exact primary-index edit, unchanged retries, source rejection, full 256-frame coverage and uint32 wrap')
