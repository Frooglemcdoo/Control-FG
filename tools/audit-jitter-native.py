from pathlib import Path
import sys,hashlib,json,struct,subprocess
from pe_tools import PE
root=Path(__file__).resolve().parents[1]
p=PE(sys.argv[1]);assert hashlib.sha256(p.data).hexdigest()=='ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433'
here=root/'validation/jitter-native';here.mkdir(exist_ok=True)
a,b=0x181c0,0x185f4
code=p.data[p.off(a):p.off(b)]
asm=p.disasm(a,b);assert 'call ' not in asm and '[rip' not in asm
assert code[-1]==0xc3
(here/'native-multiply.h').write_text('static const unsigned char nativeMultiply[]={'+','.join('0x%02x'%v for v in code)+'};\n')
(here/'native-multiply.asm').write_text(asm)
checks={0x11dea7:'f20f11b780040000',0x11deaf:'f2440f118788040000',0x11dfa4:'f20f1175c0',0x11dfa9:'f2440f1145c8',0x11e19f:'e89c270600',0x1809cc:'e8ef77e9ff',0x1300a1:'410f108580040000',0x1300a9:'0f11442450',0x13f541:'410f108e80040000',0x13f54f:'0f114c2450'}
for rva,want in checks.items():assert p.data[p.off(rva):p.off(rva)+len(bytes.fromhex(want))]==bytes.fromhex(want),hex(rva)
for rva,want in [(0x67f4b0,-.5),(0x67eeb8,.5)]:assert struct.unpack_from('<f',p.data,p.off(rva))[0]==want
for name,start,end in [('jitter-translation',0x11dc50,0x11e264),('apply-projection',0x180940,0x180b30),('aa-input',0x13007f,0x1300e6)]:
 (here/(name+'.asm')).write_text(p.disasm(start,end))
(here/'evidence.json').write_text(json.dumps({'status':'EXACT_BINARY_STATIC_PASS','renderer_sha256':hashlib.sha256(p.data).hexdigest(),'multiply_rva':hex(a),'multiply_bytes':len(code),'multiply_sha256':hashlib.sha256(code).hexdigest(),'checks':{hex(k):v for k,v in checks.items()},'derivation':['Jitter doubles are stored at renderer+0x480/+0x488 and in translation-matrix elements 12/13 (rbp=rsp+0x100, matrix starts rsp+0x60, stores rbp-0x40/-0x38).','View-copy helper 0x180940 postmultiplies source projection by that translation via 0x181C0.','Both AA caller sites copy renderer+0x480/+0x488 into the double jitter arguments.','Native NGX conversion is width*d1*0.5 and height*d2*(-0.5), correlated with prior logs.','Row-vector posttranslation adds d1/d2 to normalized clip coordinates; inverse raster ray subtracts native NGX jitter from the pixel center.'],'limit':'Native helper execution validates arithmetic, not the live GPU images or material coverage.'},indent=2)+'\n')
print('PASS: native jitter translation, AA argument linkage and multiplier bytes')
