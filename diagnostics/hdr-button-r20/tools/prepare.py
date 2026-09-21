from pathlib import Path
import sys,struct,hashlib,zipfile,json
BASE_SHA='4b54be1519646362516665764f24f8a1455a322a741910c9c12edb8211579de4'
CORE_SHA='6ab0089c5b67d0f3a61cc72db1e35284db5eeec0f568ab62ed7cdc07aee586f8'
u16=lambda b,p:struct.unpack_from('<H',b,p)[0]
u32=lambda b,p:struct.unpack_from('<I',b,p)[0]
align=lambda n,a:(n+a-1)//a*a
sha=lambda b:hashlib.sha256(b).hexdigest()

def add_import(original):
    b=bytearray(original);nt=u32(b,0x3c);op=nt+24;n=u16(b,nt+6);optsz=u16(b,nt+20);st=op+optsz
    assert b[:2]==b'MZ' and b[nt:nt+4]==b'PE\0\0' and u16(b,nt+4)==0x8664 and u16(b,op)==0x20b
    secs=[]
    for i in range(n):
        p=st+i*40;vs,va,rs,ro=struct.unpack_from('<IIII',b,p+8);secs.append((bytes(b[p:p+8]),vs,va,rs,ro))
    assert st+(n+1)*40<=min(x[4] for x in secs if x[4])
    def raw(rva):
        for _,vs,va,rs,ro in secs:
            if va<=rva<va+rs:return ro+rva-va
        raise ValueError('unmapped RVA')
    imports=u32(b,op+120);ip=raw(imports);descs=[]
    while b[ip:ip+20]!=b'\0'*20:
        assert len(descs)<100;descs.append(bytes(b[ip:ip+20]));ip+=20
    sa,fa=u32(b,op+32),u32(b,op+36)
    va_new=align(max(x[2]+max(x[1],x[3]) for x in secs),sa);ro_new=align(len(b),fa)
    payload=bytearray(20*(len(descs)+2))
    for i,d in enumerate(descs):payload[i*20:i*20+20]=d
    def put(data,a=1):
        payload.extend(b'\0'*(align(len(payload),a)-len(payload)));p=len(payload);payload.extend(data);return va_new+p
    name=put(b'ControlFGHDRButton.dll\0');hint=put(b'\0\0ControlFGHDRButton_Bootstrap\0',2)
    oft=put(struct.pack('<QQ',hint,0),8);iat=put(struct.pack('<QQ',hint,0),8)
    struct.pack_into('<IIIII',payload,20*len(descs),oft,0,0,name,iat)
    vs=len(payload);rs=align(vs,fa)
    b.extend(b'\0'*(ro_new-len(b)));b.extend(payload);b.extend(b'\0'*(rs-vs))
    struct.pack_into('<8sIIIIIIHHI',b,st+n*40,b'.hdrb\0\0\0',vs,va_new,rs,ro_new,0,0,0,0,0xC0000040)
    struct.pack_into('<H',b,nt+6,n+1);struct.pack_into('<I',b,op+56,align(va_new+vs,sa))
    struct.pack_into('<I',b,op+8,u32(b,op+8)+rs);struct.pack_into('<I',b,op+64,0)
    struct.pack_into('<II',b,op+120,va_new,20*(len(descs)+2))
    for name0,_,_,sz,off in secs:assert b[off:off+sz]==original[off:off+sz],name0
    return bytes(b),{'original_sections_byte_identical':True,'new_import':'ControlFGHDRButton.dll!ControlFGHDRButton_Bootstrap','new_section_rva':hex(va_new)}

def main():
    base=Path(sys.argv[1]);out=Path(sys.argv[2]);out.mkdir(parents=True,exist_ok=True)
    blob=base.read_bytes();assert sha(blob)==BASE_SHA,'wrong baseline zip'
    with zipfile.ZipFile(base) as z:
        for info in z.infolist():
            name=info.filename.replace('\\','/')
            if info.is_dir():continue
            assert not name.startswith('/') and '..' not in Path(name).parts
            p=out/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(z.read(info))
    original=(out/'dxgi.dll').read_bytes();assert sha(original)==CORE_SHA
    patched,detail=add_import(original);(out/'dxgi.dll').write_bytes(patched)
    generated=out.parent/'generated';generated.mkdir(exist_ok=True)
    (generated/'expected_core.h').write_text('#pragma once\ninline constexpr const char* kExpectedCoreSha256="'+sha(patched)+'";\n')
    report={'build':'RTX50 HDR Button R26','baseline_zip_sha256':BASE_SHA,'baseline_core_sha256':CORE_SHA,
            'loader_core_sha256':sha(patched),'integration':detail,'nvidia_dlls_modified':False,
            'renderer_sections_modified':False,'hdr_button':'F10 overlay child button read-only N/A; runtime setter scan active','runtime_status':'not_yet_run_in_Control'}
    (out/'HDR-BUTTON-R26-BUILD.json').write_text(json.dumps(report,indent=2))
    collector=out/'Collect-ControlFG-Compact-Logs.ps1'
    text=collector.read_text(encoding='utf-8-sig')
    needle="        if ($env:LOCALAPPDATA) {"
    insert="""        $hdrButtonLogs=@(Get-ChildItem -LiteralPath $LogDirectory -File -Filter 'hdr-button-R26-*.log' -ErrorAction SilentlyContinue | Where-Object { Test-RegularFile $_ } | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 3)
        foreach ($hdrButtonLog in $hdrButtonLogs) { Copy-CompactFile $hdrButtonLog ('hdr-button/'+$hdrButtonLog.Name) 512KB $true }
        $hdrBuild=Get-Item -LiteralPath (Join-Path $ProjectDirectory 'HDR-BUTTON-R26-BUILD.json') -ErrorAction SilentlyContinue
        if (Test-RegularFile $hdrBuild) { Copy-CompactFile $hdrBuild 'package/HDR-BUTTON-R26-BUILD.json' 64KB }
"""
    assert text.count(needle)==1
    collector.write_text(text.replace(needle,insert+needle),encoding='utf-8-sig')
    print(json.dumps(report,indent=2))
if __name__=='__main__':main()
