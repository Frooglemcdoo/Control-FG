#!/usr/bin/env python3
"""Build-locked native mesh-buffer write routes; does not infer runtime overlap."""
import argparse,hashlib,json
from pathlib import Path
import pefile
EXPECTED='cceb99cbd9c019af907c24c44701a53c8d230965c1fb31c325233e4c80214aa5'
WINDOWS={
 0x26000: '488b4940e917f8ffff',
 0x26010: '488b4940e997fcffff',
 0x26655: '488b6c2438895914488b4740488b5c243089b0e0000000488b4740488b7424404889b8d8000000',
 0x25d30: '448b4b7848897c24204c8d4320488b5308488b8bd8000000e8430c0000',
 0x25dd2: '488b530848897c24204c8bca4c8b8380000000488b8bd8000000e89f0b0000',
 0x26abe: '4d8b4d004c8b4548498b1424488bce41ff5278',
 0x26b84: '4d8b4d004c8b4548498b1424488bce41ff5278',
}
def audit(data):
 p=pefile.PE(data=data)
 for site,expected in WINDOWS.items():
  if p.get_data(site,len(bytes.fromhex(expected)))!=bytes.fromhex(expected):
   raise ValueError(f'Native mesh write route changed at {site:x}')
 return {
  'd3d_sha256':EXPECTED,
  'verified_instruction_windows':{hex(k):v for k,v in WINDOWS.items()},
  'lock':'26000 loads NativeBuffer+40 and tail-calls25820',
  'unlock':'26010 loads NativeBuffer+40 and tail-calls25CB0',
  'control_backpointer':'26675 stores original NativeBuffer at control+D8',
  'write_paths':['25D48 ->26990','25DEC ->26990'],
  'copy_destination':'26990 preserves rcx in r12;26AC6/26B8C load destination resource from [r12]',
  'copy_calls':['26ACD CopyBufferRegion for copy-list route','26B93 CopyBufferRegion after transitions for other lists'],
  'implication':'The NativeBuffer API supports copying into existing resource storage. Same wrapper/resource/GPU address does not prove immutable mesh contents.',
  'not_proven':['Whether a referenced static mesh is actually updated between the GI ray and GI6 in the captured runs.',
                'Whether all relevant callers are excluded by the RT preparation gate.'],
  'scene_lease_proven':False,
 }
def main():
 a=argparse.ArgumentParser(description=__doc__);a.add_argument('d3d',type=Path);a.add_argument('--output',type=Path,required=True);args=a.parse_args()
 data=args.d3d.read_bytes()
 if hashlib.sha256(data).hexdigest()!=EXPECTED:raise ValueError('Unsupported native D3D binary')
 report=audit(data);p=pefile.PE(data=data)
 for site in WINDOWS:
  changed=bytearray(data);changed[p.get_offset_from_rva(site)]^=1
  try:audit(changed)
  except ValueError:continue
  raise ValueError(f'Modified mesh writer accepted {site:x}')
 report['mutated_route_rejections']=len(WINDOWS)
 args.output.write_text(json.dumps(report,indent=2)+'\n')
 print('PASS native buffer map/unmap and existing-resource copy routes; actual GI overlap not established')
if __name__=='__main__':main()
