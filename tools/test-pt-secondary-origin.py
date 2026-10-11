"""Reproduce FP16 endpoint error and bound distance-payload reconstruction."""
from pathlib import Path
import math
import re
import struct

def fp16(x):
    return struct.unpack('e', struct.pack('e', x))[0]

def fp32(x):
    return struct.unpack('f', struct.pack('f', x))[0]

# A receiver at 64.03 m rounds to 64.00 m in the native hit texture.
# The 1 cm bias cannot bridge that 3 cm error.
assert abs(fp16(64.03) - 64.03) > 0.01
worst = 0.0
for i in range(1, 100001):
    distance = i * 0.009973 + 0.000037
    payload = int(fp32(fp32(distance) * 10000.0))
    reconstructed = fp32(fp32(float(payload)) * fp32(0.0001))
    worst = max(worst, abs(reconstructed-distance))
assert worst < 0.0003, worst

root = Path(__file__).resolve().parents[1]
s = (root/'src/shaders/pt_reflection_p2.hlsl').read_text()
helper=s.split('bool PTTracePreserved(',1)[1].split('[shader("raygeneration")]',1)[0]
assert 'origin+normalize(direction)*((float)payload.value*0.0001f)' in helper
raygen=s.split('void reflectionRayGeneration()',1)[1]
assert 'PTTracePreserved(pixel,firstOrigin,firstDirection,first)' in raygen
assert 'first.worldPosition+firstNormal*0.01f' in raygen
assert 'second.worldPosition+secondNormal*0.01f' in raygen
assert 'nativeFirstWorld+' not in raygen
print(f'PASS: FP16 error exceeds bias; payload endpoint error <= {worst:.7f} m; retraced first and later endpoints supply ray origins.')
