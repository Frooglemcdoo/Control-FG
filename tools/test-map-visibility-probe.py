"""Independently verify map-probe instruction scope and payload rejection."""
import importlib.util
from pathlib import Path
import sys
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'research/native-integration-audit/tools'))
from decode_dxbc import decode
from rt_spot_selection import map_visibility_probe
original=(root/'build/rt_spot_selector_bent_normal_off.dxbc').read_bytes()
actual=(root/'build/rt_spot_selector_map_probe.dxbc').read_bytes()
assert actual==map_visibility_probe(original)
old=decode(root/'build/rt_spot_selector_bent_normal_off.dxbc')['instructions']
new=decode(root/'build/rt_spot_selector_map_probe.dxbc')['instructions']
# Remove only the five added instructions, then compare every original opcode,
# operand and ordering. This includes atlas loads, winner condition and IDs.
added=[i for i in new if i['operands'] and i['operands'][0].startswith('r[44]')]
assert [(i['name'],i['operands']) for i in added]==[
 ('mov',['r[44].xyzw','l(0,0,0,0)']),
 ('movc',['r[44].x','r[14].wwww','r[43].xxxx','r[44].xxxx']),
 ('utof',['r[44].z','r[4].xxxx']),
 ('add',['r[44].z','r[44].zzzz','l(1)']),
 ('add',['r[44].y','-r[44].xxxx','l(1)'])]
remaining=[i for i in new if i not in added]
assert len(old)==len(remaining)
for a,b in zip(old,remaining):
 assert a['name']==b['name']
 if a['dword']==225:assert b['operands']==['raw:0x2d']
 elif a['dword']==14948:assert b['operands']==['type30[0][0].xyzw','type32.xyyy','r[44].xyzw']
 else:assert a['operands']==b['operands'],(a,b)
assert next(i for i in new if i['dword']==227)['name']=='dcl_thread_group'
assert added[0]['dword']==231
winner_index=new.index(added[1]);assert new[winner_index+1]['operands']==['r[34].xy','r[14].wwww','r[33].xyxx','r[34].xyxx']
# Dither happens before the diagnostic store; the payload bypasses it.
assert new[new.index(added[2])-1]['name']=='mul'
assert new[new.index(added[4])+1]['name']=='store_uav_typed'
try:map_visibility_probe(original[:-1]+bytes([original[-1]^1]))
except ValueError:pass
else:raise AssertionError('Unsupported input accepted')
spec=importlib.util.spec_from_file_location('probe',root/'tools/analyze-map-visibility-probe.py')
probe=importlib.util.module_from_spec(spec);spec.loader.exec_module(probe)
def record(rgb,light=47,rt=.1):
 return dict(valid=True,light_id=light,pixel=[12,34],composition=dict(diffuse_before=rgb+[0],visibility=[rt,1,0,0]))
assert probe.sample(record([1,0,48]))['map_minus_rt']==.9
assert probe.sample(record([.5,.5,48]))['status']=='valid'
assert probe.sample(record([0,1,64],light=63))['status']=='valid'
assert probe.sample(record([0,1,65],light=64))['status']=='id_precision_limit'
assert probe.sample(record([1,0,47]))['status']=='id_mismatch'
assert probe.sample(record([.5,.8,48]))['status']=='payload_changed'
assert probe.sample(record([float('nan'),0,48]))['status']=='missing_payload'
try:probe.analyze('blocker_trace_start mode=default_no_history_rr_off')
except ValueError:pass
else:raise AssertionError('Ordinary lighting accepted as probe data')
print('PASS map probe: exact instruction scope, winner predicate, post-dither payload, precision/provenance rejection')
