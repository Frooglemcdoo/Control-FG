"""Structural contract; real emitted shader numerical validation runs on WARP."""
import sys,hashlib
from pathlib import Path
from volume_temporal import write_header,generate,ROOT
from dxbc_checksum import checksum
sys.path.insert(0,str(ROOT/'research/native-integration-audit/tools'))
from decode_dxbc import decode
old,new=write_header()
a=decode(ROOT/'build/volume-temporal-native.dxbc');b=decode(ROOT/'build/volume-temporal.dxbc')
assert a['bindings']==b['bindings'] and a['constant_buffers']==b['constant_buffers']
assert new[4:20]==checksum(new)
assert sum(i['name']=='sample_l' for i in b['instructions'])==4 # no added samples
semantic=lambda i:(i['name'],i['operands'])
assert [semantic(i) for i in a['instructions'] if i['name']=='store_uav_typed']==[semantic(i) for i in b['instructions'] if i['name']=='store_uav_typed']
# Original reprojection and near/far decision survive, with only additional
# unclamped coordinate capture; native descriptor/output contracts unchanged.
keep=[semantic(i) for i in a['instructions'] if i['dword']<433 and i['dword']!=48]
actual=[semantic(i) for i in b['instructions']];cursor=0
for item in keep:cursor=actual.index(item,cursor)+1
for data in (old[:-1],old+b'\0',old[:80]+bytes([old[80]^1])+old[81:]):
 try:generate(data);raise AssertionError('unknown shader accepted')
 except ValueError:pass
print('PASS exact source, checksum, bindings, native reprojection, four samples, stores, unknown rejection')
