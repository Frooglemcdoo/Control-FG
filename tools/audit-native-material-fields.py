"""Recover native material field names from the verified renderer (no DLL redistribution)."""
import argparse
import json
import pefile
import capstone

parser = argparse.ArgumentParser()
parser.add_argument('renderer')
args = parser.parse_args()
pe = pefile.PE(args.renderer)
cs = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
cs.detail = True
instructions = list(cs.disasm(pe.get_data(0xC51D0, 0x27D8), 0xC51D0))

def rip_target(insn):
    assert insn.mnemonic == 'lea' and insn.operands[1].mem.base == capstone.x86.X86_REG_RIP
    return insn.address + insn.size + insn.operands[1].mem.disp

names = {}
for n, insn in enumerate(instructions):
    if insn.mnemonic == 'call' and insn.op_str == '0x210270':
        string, destination = instructions[n-2:n]
        names[rip_target(destination)] = pe.get_data(rip_target(string), 128).split(b'\0')[0].decode('ascii')
fields = []
for n, insn in enumerate(instructions):
    if not (0xC6200 < insn.address < 0xC6800 and insn.mnemonic == 'lea' and insn.op_str.startswith('r8, [rbp')):
        continue
    source = next((i for i in reversed(instructions[max(0, n-8):n]) if i.mnemonic == 'lea' and i.op_str.startswith('rdx, [rip')), None)
    if source is None:
        continue
    offset = insn.operands[1].mem.disp + 0x80
    if 0 <= offset < 0x150:
        fields.append(dict(offset=hex(offset), parameter=names[rip_target(source)], destination_instruction=hex(insn.address)))
expected = {'0x30': 'g_fEmissionIntensity', **{hex(0xC0+i*16): f'g_vWrinkleWeights{i}' for i in range(6)}}
actual = {f['offset']: f['parameter'] for f in fields}
assert all(actual.get(k) == v for k, v in expected.items())
assert pe.get_data(0xC68BE, 4).hex() == '488d4580'  # source record starts at rbp-0x80
print(json.dumps({'record_bytes': 336, 'record_stack_base': 'rbp-0x80', 'verified_fields': fields}, indent=2))
