"""Audit native temporal motion bindings and reprojection from shipped bytecode.
This verifies the native shader contract, not live resource identity or motion.
"""
import base64
import sys
import tempfile
from pathlib import Path

root = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(root / 'research/native-integration-audit/tools'))
from decode_dxbc import decode

with tempfile.TemporaryDirectory() as directory:
    path = Path(directory) / 'native-temporal.dxbc'
    path.write_bytes(base64.b64decode((root / 'research/gi-temporal-r208/diffuse-temporal.dxbc.b64').read_text()))
    shader = decode(path)
assert shader['sha256'] == 'd54d94fdbb783796cc33bc78ab20dde687d90d9cba6f24c8092a6b177eec5d15'
instructions = {i['dword']: (i['name'], i['operands']) for i in shader['instructions']}
# Pixel centers -> native normalized motion coordinates, sample the CURRENT
# motion SRV at t2/s6, then ADD its XY to obtain previous-frame UV coordinates.
expected = {
    96: ('add', ['r[1].xy', 'r[1].xyxx', 'l(0.5,0.5,0,0)']),
    226: ('mul', ['r[4].xy', 'r[1].xyxx', 'cb[0][0][1].zwzz']),
    235: ('sample_l', ['r[4].xy', 'r[4].xyxx', 't[2][2].xyzw', 's[0][6]', 'l(0)']),
    248: ('mad', ['r[4].xy', 'r[1].xyxx', 'cb[0][0][1].zwzz', 'r[4].xyxx']),
    380: ('sample_l', ['r[1].xyz', 'r[4].xyxx', 't[4][4].xyzw', 's[0][6]', 'l(0)']),
}
for offset, value in expected.items():
    assert instructions[offset] == value, (offset, instructions[offset])
# Reflection field names are printed so this audit remains useful independent
# of the decoder's representation of other unused bindings.
motion = [b for b in shader['bindings'] if b['name'] == 'g_tGeometryVelocityTexture']
assert len(motion) == 1 and motion[0]['slot'] == 2 and motion[0]['space'] == 0 and motion[0]['kind'] == 2
print('PASS exact native temporal shader: t2 current geometry motion, normalized XY, previous UV = current UV + motion')
print('Native motion binding:', motion[0])
print('Live admission must still verify same-frame identity and sys[1].zw against the GI viewport; no later RR resource may substitute.')
