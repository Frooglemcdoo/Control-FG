"""Execute captured native character remap bytecode; no game or GPU required."""
import ast, base64, hashlib, json, re, tempfile
from pathlib import Path
import numpy as np
from character_surface import root, decode, words
from character_diffuse import opnames

# Extend the existing instruction interpreter for native remap operations only.
path = Path(__file__).with_name('test-character-diffuse.py')
node = next(n for n in ast.parse(path.read_text()).body if isinstance(n, ast.FunctionDef) and n.name == 'execute')
source = ast.unparse(node)
needle = "else:\n                raise AssertionError(name)"
extension = """elif name == 'itof':
                v = args[0].view(np.int32).astype(np.float32)
            elif name == 'ishr':
                v = (args[0].view(np.int32) >> (args[1].view(np.uint32) & 31)).astype(np.int32).view(np.float32)
            elif name == 'ubfe':
                width, offset, value = [a.view(np.uint32) for a in args]
                v = ((value >> (offset & 31)) & ((np.uint32(1) << width) - np.uint32(1))).view(np.float32)
            elif name == 'bfi':
                width, offset, value, base = [a.view(np.uint32) for a in args]
                bitmask = ((np.uint32(1) << width) - np.uint32(1)) << offset
                v = ((base & ~bitmask) | ((value << offset) & bitmask)).view(np.float32)
            else:
                raise AssertionError(name)"""
assert source.count(needle) == 1
source = source.replace('v = -v', "v = (-v.view(np.int32)).view(np.float32) if name == 'iadd' else -v")
source = source.replace('sw = token >> 4 & 255', 'sw = (token >> 4 & 3) * 85 if (token >> 2 & 3) == 2 else token >> 4 & 255')
namespace = {'np': np, 'opnames': opnames}
exec(compile(source.replace(needle, extension), str(path), 'exec'), namespace)
execute = namespace['execute']
rng = np.random.default_rng(420)
checks = 0
with tempfile.TemporaryDirectory() as temporary:
    for family in json.loads((root/'research/character-surface-r346/families.json').read_text()):
        data = base64.b64decode((root/'research/character-surface-r346'/f"{family['name']}.dxbc.b64").read_bytes())
        assert hashlib.sha256(data).hexdigest() == family['sha']
        shader = Path(temporary)/'native.dxbc'; shader.write_bytes(data)
        instructions = decode(shader)['instructions']; w = words(data)
        load_index = next(i for i, x in enumerate(instructions) if x['name'] == 'ld' and x['operands'][-1] == 't[3][3].xyzw')
        start_index = max(i for i in range(load_index) if instructions[i]['name'] == 'ishr') - 1
        start = instructions[start_index]['dword']; load = instructions[load_index]
        code = w[start:load['dword']+(w[load['dword']]>>24)]
        coverage = instructions[start_index-1]
        assert coverage['name'] == 'firstbit_lo'
        register, lane = re.fullmatch(r'r\[(\d+)\]\.([xyzw])', coverage['operands'][0]).groups()
        for sample_index in (0, 0xffffffff):
            for parity in range(4):
                x, y = 4+(parity&1), 4+(parity>>1)
                for case in range(64):
                    packed = int(rng.integers(0, 2**32, dtype=np.uint32))
                    resources = {11:np.full((5,5,4),np.uint32(packed),np.uint32).view(np.float32), 3:np.zeros((10,10,4),np.float32)}
                    regs = np.zeros((128,4),np.float32)
                    regs[int(register),'xyzw'.index(lane)] = np.uint32(sample_index).view(np.float32)
                    _, reads = execute(code, regs, (x+.5,y+.5), resources, 0, 1)
                    shift = (2*sample_index+(x&1)*8+(y&1)*16) & 31
                    selected = (packed >> shift) & 3
                    expected = (3,(x&~1)+(selected&1),(y&~1)+(selected>>1))
                    assert reads[-1] == expected, (family['name'], reads[-1], expected)
                    checks += 1
print(f'PASS {checks} native remap cases: 12 pinned families, all pixel parities, randomized map words, sample zero and zero-coverage sentinel')
print('Scope: coordinate arithmetic only; does not prove selected source surface matches destination depth/motion.')

# Deterministic divergence: valid sample-zero selector is 0, sentinel selector is 3.
for parity in range(4):
    valid_shift = (parity&1)*8+(parity>>1)*16
    invalid_shift = (valid_shift-2)&31
    packed = 3 << invalid_shift
    assert ((packed>>valid_shift)&3) == 0 and ((packed>>invalid_shift)&3) == 3
print('PASS zero-coverage sentinel selects different map bits in all four parities; sample count alone does not prove selector zero')
