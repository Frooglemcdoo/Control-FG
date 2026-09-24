"""Exact supplied DLL evidence for worker binding behavior, not GPU validation."""
from pathlib import Path
import hashlib,json,sys
from pe_tools import PE
root=Path(__file__).resolve().parents[1]
p=PE(sys.argv[1]);sha=hashlib.sha256(p.data).hexdigest()
assert sha=='cceb99cbd9c019af907c24c44701a53c8d230965c1fb31c325233e4c80214aa5'
checks={0x2eb4d:'4a833c3000',0x2eb52:'752f',0x2eb61:'e8eac00000',0x2eb76:'e8d5c00000',0x2ed63:'498b87a0000000',0x2ed6a:'4c89bbe0090000',0x2ed7a:'488906',0x2ee11:'ff9070010000',0x3ad74:'894720'}
for rva,want in checks.items():assert p.data[p.off(rva):p.off(rva)+len(bytes.fromhex(want))]==bytes.fromhex(want),hex(rva)
d=root/'validation/worker-depth';d.mkdir(exist_ok=True)
for rva in [0x2eb00,0x3ac50]:(d/f'{rva:x}.asm').write_text(p.disasm(rva))
(d/'evidence.json').write_text(json.dumps({'status':'EXACT_BINARY_STATIC_PASS','sha256':sha,'checked_bytes':{hex(k):v for k,v in checks.items()},'findings':['Non-null TLS+8 branches from 0x2eb52 to 0x2eb83, bypassing both color and depth prepareForProducing calls.','Native setRenderTargets loads depth texture+0xA0 into device state+0x40 and depth identity into state+0x9E0 before OMSetRenderTargets.','prepareForProducing writes tracker+0x20 after recording ResourceBarrier; tracker is CPU recording metadata, not a GPU completion observation.'],'runtime_limit':'Static code establishes the native worker binding contract, not the precise inter-thread scheduling that produced r11 state 0xC0.','policy':'Workers must preserve nonzero matching inherited DSV and depth identity; primary retains exact depth-write state. No new barriers.'},indent=2)+'\n')
print('PASS: exact DLL worker transition bypass and inherited DSV binding evidence')
