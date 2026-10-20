"""Check encounter entry points against a locally supplied game (not distributed)."""
import sys, re, hashlib, json
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from pe_tools import PE
p=PE(sys.argv[1]);source=Path(__file__).with_name('no-spawns.cpp').read_text()
assert hashlib.sha256(p.data).hexdigest()==re.search(r'gameHash\[\] = "([a-f0-9]+)"',source)[1]
events={0x328bd0:('OnEncounterStart',0x328bfb),0x32a7f0:('OnEncounterStart',0x32a81b),0x32a860:('OnEncounterStart',0x32a885),0x5313a0:('OnPreplacedEncounterStart',0x531460)}
for rva,expected in re.findall(r'\{0x([a-f0-9]+),"([a-f0-9]+)"\}',source):
 start=int(rva,16);code=bytes.fromhex(expected)
 assert p.data[p.off(start):p.off(start)+len(code)]==code
 end=next(b for a,b in p.functions if a==start)
 name,event=events[start];assert start<event<end
 assembly=p.disasm(start,end)
 assert 'ret' in assembly or 'jmp' in assembly
 # Verify the LEA's exact target is the event string.
 import struct
 target=event+7+struct.unpack_from('<i',p.data,p.off(event)+3)[0]
 assert p.string(target)==name
 print(f'PASS {start:#x} entry bytes / unwind boundary / {name} reference')
print('PASS exact-build static audit; live spawn coverage requires game testing')
