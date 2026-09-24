"""Audit the exact native shader-table layout used by eye selection."""
from pathlib import Path
import sys,json,hashlib
from pe_tools import PE
p=PE(sys.argv[1]);root=Path(__file__).resolve().parents[1]
h=hashlib.sha256(p.data).hexdigest()
assert h=='ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433'
sites={0x1dd3fd:'8b9140020000',0x1dd40d:'4c8b9938020000',0x1dd41d:'4c69c9a8000000',0x1dd427:'453b5104'}
for address,expected in sites.items():
 b=bytes.fromhex(expected);off=p.off(address);assert p.data[off:off+len(b)]==b
out=root/'validation/eye-variants'
(out/'native-getshader.asm').write_text(p.disasm(0x1dd3f0,0x1dd444))
(out/'native-table.json').write_text(json.dumps({'status':'PASS','renderer_sha256':h,'table_offset':568,'count_offset':576,'record_stride':168,'key_offset':4,'instruction_bytes':{hex(k):v for k,v in sites.items()},'scope':'Read-only table layout; not eye variant semantics or runtime admission.'},indent=2)+'\n')
print('PASS: exact native getShader table/count/record/key layout')
