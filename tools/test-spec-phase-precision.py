"""Interpret the inserted phase IR and compare to an independent exact oracle.

No scene-variance or hardware-performance claim. Quantization measurements
use synthetic offsets, not a recovered native blue-noise texture.
"""
import json
import random
import re
import struct
from fractions import Fraction
from pathlib import Path
from spec_phase_precision import ROOT, CONSTANTS, NUMERATORS, edits, transform
from spec_retry_reuse import transform as reuse


def f32(v):
    return struct.unpack('<f', struct.pack('<f', v))[0]


source = (ROOT/'research/native-integration-audit/reflection-000.ll').read_text()
patched = transform(source)
baseline = reuse(source)
restored = patched
for old, new in edits():
    assert new in patched
    restored = restored.replace(new, old)
assert restored == baseline, 'Unexpected edit outside primary phase arithmetic'
assert len(re.findall(r'(?<![\w.])%273\b', patched)) == 1, 'Large float index still used'
for old, _ in edits():
    try:
        transform(source.replace(old, old.replace('fmul fast', 'fadd fast')))
    except AssertionError:
        pass
    else:
        raise AssertionError('Unrecognized phase accepted')

constants = [struct.unpack('>d', bytes.fromhex(h))[0] for h in CONSTANTS]
for c, n in zip(constants, NUMERATORS):
    assert Fraction(c) == Fraction(n, 2**24)
instructions = [line.strip() for _, new in edits() for line in new.splitlines()]


def execute(index):
    env = {'%272': index}
    for line in instructions:
        name, rhs = line.split(' = ')
        if rhs.startswith('mul i32 '):
            a, b = rhs[8:].split(', '); env[name] = (env[a]*int(b)) & 0xFFFFFFFF
        elif rhs.startswith('and i32 '):
            a, b = rhs[8:].split(', '); env[name] = env[a] & int(b)
        elif rhs.startswith('uitofp i32 '):
            env[name] = f32(env[rhs.split()[2]])
        elif rhs.startswith('fmul float '):
            a, b = rhs[11:].split(', ')
            env[name] = f32(env[a]*struct.unpack('>d', bytes.fromhex(b[2:]))[0])
        else:
            raise AssertionError(line)
    return [env[k] for k in ('%.i0255', '%.i1256', '%.i2257')]


rng = random.Random(185)
indices = [0, 1, 2**23-1, 2**23, 2**24-1, 2**24, 2**31-1, 2**32-1]
for frame in (1, 50093, 56810, 1000000, 2**32-1):
    for count in (1, 2, 4, 8, 12, 16):
        indices += [((frame*count)+sample) & 0xFFFFFFFF for sample in range(count)]
indices += [rng.randrange(2**32) for _ in range(10000)]
for index in indices:
    actual = execute(index)
    for value, constant in zip(actual, constants):
        exact = Fraction(index)*Fraction(constant)
        expected = float(exact - (exact.numerator//exact.denominator))
        assert value == expected and 0 <= value < 1, (index, value, expected)
        for shift in (0.0, .5, f32(1-2**-24), f32(rng.random())):
            shifted = f32(value+shift)
            phase = shifted-int(shifted)
            mathematical = (expected+shift) % 1
            error = abs(phase-mathematical)
            assert min(error, 1-error) <= 2**-24

# A bounded phase keeps all tested shift levels distinct at each age. Legacy
# separated MUL+ADD matches the native IR, but a driver could contract them;
# a contracted path still rounds at the large output magnitude before frac.
quantization = []
for frame in (1, 50093, 56810, 1000000):
    row = {'synthetic_temporal_counter': frame, 'spec': 4, 'synthetic_shift_levels': 65536, 'dimensions': []}
    fixed = execute(frame*4)
    for dim, constant in enumerate(constants):
        old, fused, new = set(), set(), set()
        for i in range(65536):
            shift = i/65536
            z = f32(f32(f32(frame*4)*constant)+shift); old.add(z-int(z))
            z = f32(f32(frame*4)*constant+shift); fused.add(z-int(z))
            z = f32(fixed[dim]+shift); new.add(z-int(z))
        assert len(new) == 65536
        row['dimensions'].append({'legacy_unique':len(old), 'contracted_unique':len(fused), 'bounded_unique':len(new)})
    quantization.append(row)
report = {'interpreter_cases':len(indices), 'exact_fraction_oracle':True,
          'synthetic_shift_quantization':quantization, 'gpu_temporal_counter_observed':False, 'scene_quality_measured':False}
out = ROOT/'build'; out.mkdir(exist_ok=True)
(out/'spec-phase-audit.json').write_text(json.dumps(report, indent=2)+'\n')
print(json.dumps(report, indent=2))
