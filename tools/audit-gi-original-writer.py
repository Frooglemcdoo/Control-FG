#!/usr/bin/env python3
"""Exact original GI ray -> scheduled writer ordering, read-only static audit.
Does not establish runtime exclusion or authorize scene/descriptors as immutable.
"""
import argparse, hashlib, json
from pathlib import Path
import pefile
EXPECTED = "cceb99cbd9c019af907c24c44701a53c8d230965c1fb31c325233e4c80214aa5"
WINDOWS = {'0x35701': '498b8020020000488b80a800000048ff00498b9020020000488b82a8000000488b0883e1014c8d348949c1e6044c03f241', '0x35782': '498b7e48803da3140c00007554498b80300200004889442430418b802c020000894424288b866814000089442420458b88280200004d8b8040020000498bd6b9ffffffffe875e2ffff4c8b0556151000498b8020020000488b80a000000048ff00', '0x358c7': '488b0d4ac30d004885d2480f45ca48ff4108488b4920488b01488d55a7ff9060020000', '0x35936': '4963dd48c1e306488d15ec1310004c89741310488b0dd8131000488b814002000048894413188b8128020000894413208b8668140000894413248b812c02000089441328488b81300200004889441330', '0x359bd': '488d156c1310004803d30f104424400f1102488b0d927c0200488b09ff15717c0200'}
def audit(data, check_hash=True):
    if check_hash and hashlib.sha256(data).hexdigest()!=EXPECTED:
        raise ValueError("Unsupported D3D binary")
    pe=pefile.PE(data=data)
    for rva, expected in WINDOWS.items():
        raw=bytes.fromhex(expected)
        if pe.get_data(int(rva,16),len(raw))!=raw:
            raise ValueError("Original writer ordering changed at "+rva)
    return {"sha256":hashlib.sha256(data).hexdigest(),"instruction_windows":len(WINDOWS),
        "original_dispatch_rva":"0x358e4","writer_arguments_rva":"0x35944",
        "writer_schedule_rva":"0x359d9","cpu_snapshot_required_at":"original ray call under acquired scene window",
        "gpu_completion_proven":False,"scene_descriptor_exclusion_proven":False}
def main():
    ap=argparse.ArgumentParser();ap.add_argument("dll");ap.add_argument("--output",required=True)
    args=ap.parse_args();data=Path(args.dll).read_bytes();result=audit(data);pe=pefile.PE(data=data)
    mutations=0
    for rva in WINDOWS:
        changed=bytearray(data);changed[pe.get_offset_from_rva(int(rva,16))]^=1
        try:audit(bytes(changed),False)
        except ValueError:mutations+=1
        else:raise AssertionError("Changed instruction accepted")
    result["instruction_mutations_rejected"]=mutations
    Path(args.output).write_text(json.dumps(result,indent=2)+"\n")
    print("PASS original writer sequence:",len(WINDOWS),"windows;",mutations,"mutations rejected")
if __name__=="__main__":main()
