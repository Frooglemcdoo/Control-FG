from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'research/native-integration-audit/tools'))
from decode_dxbc import decode
root = Path(__file__).resolve().parents[1]
d = decode(root / 'validation/primary-variants/single-spot.dxbc')
assert d['sha256'] == 'a9e5b544d5ec48e9b439b36aafc5346645f0261d3b1df65dc09e6cce290c164f'
i = {x['dword']: (x['name'], x['operands']) for x in d['instructions']}
assert i[11323] == ('ld_structured', ['r[12].w', 'r[31].y', 'l(156)', 't[10][10].xxxx'])
assert i[11476] == ('and', ['r[31].zw', 'r[12].wwww', 'l(0,0,4,2)'])
assert i[11536] == ('if', ['r[31].w'])
assert i[5961] == ('ld_structured', ['r[26].w', 'r[33].y', 'l(304)', 't[15][15].xxxx'])
assert i[5986] == ('and', ['r[39].xy', 'r[26].wwww', 'l(2,768,0,0)'])
assert i[6141] == ('ieq', ['r[39].zw', 'r[39].yyyy', 'l(0,0,256,512)'])
assert i[7106] == ('ieq', ['r[30].w', 'r[39].y', 'l(512)'])
assert i[7242] == ('mov', ['r[29].w', 'l(1)'])
s = (root / 'src/shaders/pt_primary_path.hlsl').read_text()
a = s.index('if(native.active==0)return false;')
assert 'if(native.castsShadow==0)return true;' not in s
assert a < s.index('visibility=0;return true;', a) < s.index('TraceRay(PTScene,RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH', a)
print('PASS: native flag decoding documented; production traces every light regardless of raster shadow enable')
