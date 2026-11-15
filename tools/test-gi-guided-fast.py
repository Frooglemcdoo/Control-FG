"""Check the emitted R441 branch and exactly recover the R434 instruction stream."""
from pathlib import Path
import re,sys
from gi_live_sampling import transform as live
from gi_single_producer import owned_raygen
from gi_guided_live import transform as reference
from gi_guided_fast import transform
source=owned_raygen(live(Path(sys.argv[1]).read_text(),0),0)
old=reference(source);new=transform(source)
assert new.count('  %G441OneRay = icmp eq i32 %226, 1')==1
assert new.count('  br i1 %G441OneRay, label %G441Guide, label %G441Trace')==1
oldops=re.findall(r'^  %G434_\d+ = .*$',old,re.M)
newops=re.findall(r'^  %G434_\d+ = .*$',new,re.M)
assert oldops==newops
assert new.index('G441Guide:')<new.index(oldops[0])<new.index(oldops[-1])<new.index('G441Trace:')
phis=re.findall(r'^  %G441(Dir[012]|Mask) = phi (float|i32) \[ (%G434_\d+), %G441Guide \], \[ ([^,]+), (%"[^"\n]+") \]$',new,re.M)
assert len(phis)==4
predecessor=phis[0][4]
oldtrace=re.search(r'^  call void @dx.op.traceRay.struct.HitData\(.*$',old,re.M)[0]
newtrace=re.search(r'^  call void @dx.op.traceRay.struct.HitData\(.*$',new,re.M)[0]
restored=new
for name,typ,value,fallback,pred in phis:
 assert pred==predecessor
 assert fallback==('1' if name=='Mask' else '%GISingle'+name)
 # The old shader selects its original multi-ray direction/mask for this same condition.
 expr=next(line for line in oldops if line.startswith('  '+value+' = '))
 assert re.search(r'select i1 (%G434_\d+), '+typ+r' [^,]+, '+typ+' '+re.escape(fallback)+'$',expr)
 condition=re.search(r'select i1 (%G434_\d+)',expr)[1]
 assert '  '+condition+' = icmp eq i32 %226, 1' in oldops
 newtrace=newtrace.replace('%G441'+name,value)
assert oldtrace==newtrace
# Reversibility proves all original arithmetic, ray arguments, counts, stores,
# hit/miss exports and resource/metadata declarations survived byte for byte.
lines=[]
for line in new.splitlines():
 if re.match(r'  %G441|G441(?:Guide|Trace):|  br (?:i1 %G441OneRay|label %G441Trace)',line):continue
 if ' = phi ' in line:line=line.replace('%G441Trace',predecessor)
 if line.startswith('  call void @dx.op.traceRay.struct.HitData('):line=oldtrace
 lines.append(line)
assert '\n'.join(lines)+'\n'==old
# Full assembly/validation runs in gi_live_sampling.py before this test in CI.
print('PASS: one-ray guide unchanged; counts 2..16 bypass',len(oldops),'unused guide instructions per ray; exact reversible CFG edit')
