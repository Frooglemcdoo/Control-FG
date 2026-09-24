"""Read-only exact-build binding audit; no live binding authorization implied."""
from pathlib import Path
import argparse, hashlib, json, struct, subprocess

def audit(path):
    data=path.read_bytes()
    expected='ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433'
    actual=hashlib.sha256(data).hexdigest()
    if actual!=expected: raise ValueError('renderer hash mismatch')
    pe=struct.unpack_from('<I',data,0x3c)[0]
    count=struct.unpack_from('<H',data,pe+6)[0]
    optional_size=struct.unpack_from('<H',data,pe+20)[0]
    opt=pe+24
    if struct.unpack_from('<H',data,opt)[0]!=0x20b: raise ValueError('not PE32+')
    base=struct.unpack_from('<Q',data,opt+24)[0]
    sections=[]
    for i in range(count):
        p=opt+optional_size+40*i
        vs,va,size,raw=struct.unpack_from('<IIII',data,p+8)
        sections.append((va,size,raw))
    def off(rva):
        for va,size,raw in sections:
            if va<=rva<va+size: return raw+rva-va
        raise ValueError(hex(rva))
    def string(rva):
        p=off(rva);return data[p:data.index(b'\0',p)].decode('ascii')
    imports={}
    rva=struct.unpack_from('<I',data,opt+120)[0]
    p=off(rva)
    while True:
        original,_,_,name,first=struct.unpack_from('<IIIII',data,p)
        if not any((original,name,first)):break
        q=off(original or first);i=0
        while True:
            v=struct.unpack_from('<Q',data,q+8*i)[0]
            if not v:break
            imports[hex(first+8*i)]=string(name)+'!'+(string(v+2) if not v>>63 else 'ordinal:'+str(v&65535))
            i+=1
        p+=20
    ranges=[(0x2f690,0x2f79a),(0xc92f0,0xc93f5)]
    text=[]
    for start,end in ranges:
        text.append(subprocess.check_output(['objdump','-d','-M','intel',f'--start-address={base+start}',f'--stop-address={base+end}',str(path)],text=True))
    requested=['0x5dfc70','0x5dfc78','0x5dfc80','0x5dfc88','0x5dfc90','0x5dfc98','0x5dfca0','0x5dfd30','0x5dff90']
    return {'renderer_sha256':actual,'strings':{'0x621f00':string(0x621f00),'0x62a118':string(0x62a118)},
        'selected_imports':{key:imports.get(key) for key in requested},
        'limits':['Name registrations are not NativeTexture/NativeBuffer pointers.','Material candidate is read from owner+0xf0; owner provenance and current-frame lifetime require further tracing.','No GBuffer2 specular channel selected; no GPU resources bound.'],
        'disassembly':text}

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('renderer',type=Path);parser.add_argument('--output',type=Path)
    args=parser.parse_args();result=audit(args.renderer)
    serialized=json.dumps(result,indent=2)+'\n'
    if args.output:args.output.write_text(serialized)
    else:print(serialized)
