"""Exact captured world RGB, with private replay alpha tagged as -4."""
from pathlib import Path
import struct, json, hashlib
from dxbc_transform import transform_shex
root=Path(__file__).resolve().parents[1]
fixtures=root/'tools/fixtures/rr-world-albedo'
def patch(words):
    # All four audited shaders have one unconditional final ret. Preserve every
    # original instruction and append mov o0.w, l(-4.0) immediately before it.
    assert words[-1]==0x0100003e
    words[-1:-1]=[0x05000036,0x00102082,0,0x00004001,0xc0800000]
lines=['#pragma once','// Generated from audited R101 Standard material bytecode.']
for entry in json.loads((fixtures/'manifest.json').read_text()):
    i=entry['variant']; native=bytes.fromhex((fixtures/f'variant-{i}.hex').read_text())
    assert len(native)==entry['bytes'] and hashlib.sha256(native).hexdigest()==entry['sha256']
    patched=transform_shex(native,patch)
    for name,data in [('Native',native),('Replay',patched)]:
        lines.append(f'inline constexpr unsigned char kRRWorld{name}{i}[]={{'+','.join(str(b) for b in data)+'};')
lines.append('struct RRWorldVariant {const unsigned char* native; size_t nativeSize; const unsigned char* replay; size_t replaySize;};')
lines.append('inline constexpr RRWorldVariant kRRWorldVariants[]={')
for entry in json.loads((fixtures/'manifest.json').read_text()):
    i=entry['variant'];lines.append(f'{{kRRWorldNative{i},sizeof(kRRWorldNative{i}),kRRWorldReplay{i},sizeof(kRRWorldReplay{i})}},')
lines.append('};')
(root/'build').mkdir(exist_ok=True)
(root/'build/rr_world_albedo_compiled.h').write_text('\n'.join(lines)+'\n')
print('Generated four exact-match world albedo variants; RGB untouched, alpha=-4')
