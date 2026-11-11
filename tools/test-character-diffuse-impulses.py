"""Character filter limitation reproduction; no claim about captured game pixels."""
import ast
from pathlib import Path
import numpy as np
from character_diffuse import block, opnames
from character_surface import src

# Reuse the instruction interpreter without executing its build-fixture suite.
source = Path(__file__).with_name('test-character-diffuse.py')
module = ast.parse(source.read_text())
function = next(n for n in module.body if isinstance(n, ast.FunctionDef) and n.name == 'execute')
namespace = {'np': np, 'opnames': opnames}
exec(compile(ast.Module(body=[function], type_ignores=[]), str(source), 'exec'), namespace)
execute = namespace['execute']
code = block(16, 0, src(1), [0x101e46, 0], 0)
for neighbor, expected_unchanged in [(0.1, True), (0.49, True), (0.8, False), (1., True)]:
    resources = {
        0: np.tile(np.array([.5, .5, .5, 128/255], np.float32), (10, 10, 1)),
        1: np.tile(np.array([.2, .3, 10/255, 20/255], np.float32), (10, 10, 1)),
        3: np.full((10, 10, 4), neighbor, np.float32),
        11: np.zeros((5, 5, 4), np.float32),
    }
    # All sampled positions are even/even: map=0 reconstructs them unchanged.
    resources[3][4, 4, :3] = 1.
    regs = np.zeros((64, 4), np.float32)
    regs[0, :3] = 1.
    regs[1] = np.array([4, 4, 0, 0], np.uint32).view(np.float32)
    result, reads = execute(code, regs, (4, 4), resources, 0, 1)
    value = result[0, :3]
    assert len([r for r in reads if r[0] == 3]) == 8
    if expected_unchanged:
        assert np.allclose(value, 1., atol=1e-6), value
    else:
        assert np.all(value < .95) and np.all(value > neighbor), value
    print(f'PASS center=1 neighbors={neighbor:g} -> {value[0]:.6f}; eight neighbors read')
print('LIMITATION: bright isolated samples can be preserved by radiance rejection; not proof of in-game cause.')

# A remapped center can differ from the destination's guides. Demonstrate that
# changing ONLY destination guides cannot affect this source-anchored filter.
resources = {
    0: np.tile(np.array([.5,.5,.5,128/255],np.float32),(10,10,1)),
    1: np.tile(np.array([.2,.3,10/255,20/255],np.float32),(10,10,1)),
    3: np.full((10,10,4),.8,np.float32),
    11: np.zeros((5,5,4),np.float32),
}
resources[3][4,4,:3] = 1.
regs = np.zeros((64,4),np.float32)
regs[0,:3] = 1.
regs[1] = np.array([4,4,0,0],np.uint32).view(np.float32)
before, reads = execute(code,regs,(5.5,5.5),resources,0,1)
resources[0][5,5] = [.9,.1,.5,129/255]
resources[1][5,5,2:] = [99/255,100/255]
after, changed_reads = execute(code,regs,(5.5,5.5),resources,0,1)
assert np.array_equal(before[0],after[0])
assert reads == changed_reads and not any(slot in (0,1) and x==5 and y==5 for slot,x,y in reads)
print('PASS destination guide mismatch remains unobserved: output unchanged, destination guides never read')
print('Scope: missing destination validation is proven; actual wrong-surface sampling in the game is not.')
