import importlib.util
from pathlib import Path
spec=importlib.util.spec_from_file_location('scene',Path(__file__).with_name('analyze-gi-scene-check.py'))
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
rows=[]
def add(kind,data):
    seq=len(rows)+1;rows.append(dict(seq=seq,kind=kind,data=data+[0]*(12-len(data))));return seq
add(25,[427,1,0,4])
for point in (1,2,3,8):
    parent=add(point,[100,10,1,1,1,1,0,0,444,555])
    sample=add(22,[parent,100,point,2,2,0,0,0,63,0,7,1])
    add(23,[sample,1,2,3,4,5,6,7,8,9,10,11]);add(24,[sample,0,333,444,8,10,49])
rows.append(dict(status=True,stopped=1,reserved=len(rows),written=len(rows),dropped=0))
r=m.analyze(rows);assert not r['warnings'] and len(r['frames'])==1
assert r['frames'][0]['ray']['phase_heap_matches'] and not r['frames'][0]['writer_serial_changed']
assert not r['scene_lease_authorized'] and not r['native_suppression_authorized']
shade=next(e for e in rows if e.get('kind')==22 and e['data'][2]==2)
shade['data'][3]=3;shade['data'][4]=4;shade['data'][5]=1
r=m.analyze(rows);assert r['frames'][0]['writer_serial_changed'] and r['frames'][0]['shade']['concurrent_change'] and r['frames'][0]['active_writer_at_either']
shade['data'][9]=5;assert 'native_metadata_read_fault' in m.analyze(rows)['warnings']
rows[-1]['dropped']=1;assert 'trace_incomplete_or_overflowed' in m.analyze(rows)['warnings']
assert 'missing_or_duplicate_sequence' in m.analyze(rows[:2]+rows[3:])['warnings']
assert 'scene_hooks_not_confirmed' in m.analyze([])['warnings']
print('PASS scene observation parser: writer overlap, serial changes, native identity comparison, missing/faulted/overflowed evidence, no false lease')
