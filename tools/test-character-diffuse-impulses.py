"""R422 emitted character filter: bright impulse mixing and conserved flat-surface energy."""
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
for neighbor in [0.1, 0.49, 0.8, 1.]:
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
    expected = (4. + 12.*neighbor)/16.
    assert np.allclose(value,expected,atol=2e-5), (value,expected)
    print(f'PASS center=1 neighbors={neighbor:g} -> {value[0]:.6f}; eight neighbors read')
print('PASS bright impulses mix with compatible receivers; constant light remains constant.')

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

# Independent flat periodic surface: summing emitted outputs must preserve the
# input energy, even for very bright impulses. No native-map ambiguity here:
# every 2x2 tile explicitly selects its top-left sample, including padded tiles.
for peak in [1.,30.,173.]:
    signal=np.full((6,6),.01,np.float32);signal[2,2]=peak
    resources={
      0:np.tile(np.array([.5,.5,.5,128/255],np.float32),(16,16,1)),
      1:np.tile(np.array([.2,.3,10/255,20/255],np.float32),(16,16,1)),
      3:np.zeros((16,16,4),np.float32),11:np.zeros((8,8,4),np.float32)}
    for y in range(16):
      for x in range(16):resources[3][y,x,:3]=signal[(y//2-1)%6,(x//2-1)%6]
    outputs=[]
    for y in range(6):
      for x in range(6):
        p=(2+2*x,2+2*y);regs=np.zeros((64,4),np.float32)
        regs[0,:3]=signal[y,x];regs[1]=np.array([*p,0,0],np.uint32).view(np.float32)
        out,_=execute(code,regs,p,resources,0,1);outputs.append(out[0,0])
    assert np.isclose(sum(outputs),signal.sum(),rtol=2e-5)
    assert min(outputs)>=signal.min()-1e-6 and max(outputs)<peak
    print(f'PASS periodic emitted energy peak={peak:g}: {signal.sum():.6f} -> {sum(outputs):.6f}')
