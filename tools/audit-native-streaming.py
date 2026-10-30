"""Read-only audit of the supplied Control renderer; does not redistribute game code."""
import argparse
import hashlib
import json
import struct
import capstone
import pefile

parser = argparse.ArgumentParser()
parser.add_argument('renderer')
args = parser.parse_args()
pe = pefile.PE(args.renderer)
cs = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
cs.detail = True

def instruction(rva):
    return next(cs.disasm(pe.get_data(rva, 15), rva))

def rip(rva):
    ins = instruction(rva)
    mem = next(o.mem for o in ins.operands
               if o.type == capstone.x86.X86_OP_MEM and o.mem.base == capstone.x86.X86_REG_RIP)
    return ins.address + ins.size + mem.disp

assert rip(0x15AB5B) == 0x9186A0
assert struct.unpack('<f', pe.get_data(rip(0x15AB63), 4))[0] == 1000.0
assert rip(0x169138) == 0x919010
assert instruction(0x1691C5).op_str == 'rdi, r14'
assert instruction(0x169255).op_str == 'qword ptr [rbp + 0x88]'
assert instruction(0x15AC04).op_str == '0x158060'
imports = {i.address - pe.OPTIONAL_HEADER.ImageBase: i.name.decode() if i.name else ''
           for entry in pe.DIRECTORY_ENTRY_IMPORT for i in entry.imports}
assert 'requestFilePartialLoad@Loader' in imports[rip(0x158247)]
assert instruction(0x137608).op_str == 'r12d, 2'
assert rip(0x1377A2) == 0x919010
assert rip(0x13781E) == 0x919010
assert pe.get_data(0x1377A2, 7).hex() == '44892567187e00'
assert pe.get_data(0x13781E, 10).hex() == 'c705e8177e0008000000'
assert struct.unpack('<Q', pe.get_data(0x632518, 8))[0] == pe.OPTIONAL_HEADER.ImageBase + 0x1374B0
points = [0x15AB5B, 0x15AB63, 0x15AB97, 0x15AC04, 0x15AC46, 0x15AC93,
          0x158247, 0x169138, 0x16913F, 0x169148, 0x169198, 0x1691C5, 0x169255,
          0x169360, 0x16936E, 0x137608, 0x137744, 0x1377A2, 0x13781E]
print(json.dumps({
    'renderer_sha256': hashlib.sha256(open(args.renderer, 'rb').read()).hexdigest(),
    'budget_rva': '0x9186a0', 'slice_rva': '0x919010',
    'request_routine': '0x15ab30', 'request_body': '0x158060',
    'pool_scan_routine': '0x168ed0',
    'native_settings_routine': '0x1374b0',
    'native_partition_policy': {'source_field': 'renderer+0x6b0',
                                'value_4': 2, 'values_0_through_3': 8,
                                'other_values': 'no store on this branch',
                                'field_semantics': 'not yet established'},
    'evidence': [{'rva': hex(rva), 'instruction': f'{instruction(rva).mnemonic} {instruction(rva).op_str}'} for rva in points],
    'partial_file_import': imports[rip(0x158247)],
    'limits': ['Static inspection does not measure time or prove which streaming stage limits throughput.',
               'Slice invocation frequency and budget-accumulator reset cadence remain to be traced.',
               'Archive codec, chunk layout, and loader completion ownership are not established.']
}, indent=2))
