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
 # CPU uploads stage in chunks, then enter the ranged GPU-copy helper.
 0x26871: '498be94d8bf0488bf24c8be141bd00004000',
 0x2693f: '448b8c249800000048897c24204c8d442440488bd6498bcce834000000',
 # move writes the temporary buffer AND the final destination via 26990.
 0x255b7: '4c8bce4d8bc6490f42dc498bcf33d2488bc349f7f5482bda33d248895c2420e8b5130000',
 0x255db: '4533c948895c24204d8bc7488bd5498bcee89f130000',
 # Whole-resource DX12 copy does NOT enter 26990: command-list slot17.
 0x27cdd: '4c8bf24c8bf9',
 0x27d54: '488b4918488b014d8b06498b17ff9088000000',
 # Control type zero maps/unmaps the original resource directly. A GPU copy
 # observer alone cannot establish content versions for this CPU write route.
 0x2586b: '418d41ffa9fdffffff0f94c2885110418d41fe83f8020f96c0884111',
 0x25d04: '85c90f843a010000',
 0x25c29: '48897da048897da884d27408498d0438488945a8488b83d8000000488b08488b014c8d4b184c8d45a033d2ff5040',
 0x25e46: '488b43084889442448488944245040386b1174164885d27406482b5318eb03498bd04803c24889442450488b83d8000000488b08488b014c8d44244833d2ff5048',
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
  'cpu_upload_route':{'entry':'0x26840','chunk_limit':4194304,'ranged_copy':'0x26957 -> 0x26990',
                      'destination':'entry RCX','destination_offset':'entry RDX','bytes':'entry R9'},
  'move_route':{'entry':'0x25570','temporary_write':'0x255d6 -> 0x26990',
                'destination_write':'0x255ec -> 0x26990',
                'implication':'Both writes must enter the ordered write journal; a move is not just allocation retirement.'},
  'whole_resource_route':{'entry':'0x27cc0','call':'0x27d61','vtable_slot':17,
                          'operation':'CopyResource','destination_resource':'[entry RCX]',
                          'source_resource':'[entry RDX]','bypasses_ranged_copy_helper':True},
  'direct_cpu_mapping':{'control_type':0,'map_call':'0x25c54','unmap_call':'0x25e84',
                        'resource':'[control+D8] then [wrapper]',
                        'write_flag':'control+11 is set for MapType numeric values 2, 3, 4',
                        'range':'offset at control+8; full length at control+0 or optional end-pointer delta',
                        'bypasses_gpu_copy_hooks':True,
                        'requirement':'Version and protect the actual CPU-write interval; Unmap range is not a fence and post-write invalidation cannot protect an already executing reader.'},
  'observation_boundary':'Record both CopyBufferRegion and CopyResource at the existing command-list hooks, then consume in actual Execute/list order. Recording order alone is insufficient.',
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
