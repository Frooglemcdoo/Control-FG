"""Prevent world-renderer changes from entering the clean-baseline guide build."""
import hashlib
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
manifest = json.loads((root / 'tools/fixtures/clean-baseline-world.json').read_text())
perf_exceptions = json.loads((root/'tools/fixtures/r32-baseline-exceptions.json').read_text())
live_exceptions = json.loads((root/'tools/fixtures/r31-baseline-exceptions.json').read_text())
exceptions = json.loads((root/'tools/fixtures/r28-baseline-exceptions.json').read_text())
glass_exceptions = json.loads((root/'tools/fixtures/r34-baseline-exceptions.json').read_text())
menu_exceptions = json.loads((root/'tools/fixtures/r41-baseline-exceptions.json').read_text())
fixed_exceptions = json.loads((root/'tools/fixtures/r46-baseline-exceptions.json').read_text())
provider_exceptions = json.loads((root/'tools/fixtures/r49-baseline-exceptions.json').read_text())
isolation_exceptions = json.loads((root/'tools/fixtures/r54-baseline-exceptions.json').read_text())
shadow_exceptions = json.loads((root/'tools/fixtures/r67-baseline-exceptions.json').read_text())
rr_filter_exceptions = json.loads((root/'tools/fixtures/r98-baseline-exceptions.json').read_text())
rr_path_exceptions = json.loads((root/'tools/fixtures/r99-baseline-exceptions.json').read_text())
gi_filter_exceptions = json.loads((root/'tools/fixtures/r100-baseline-exceptions.json').read_text())
world_albedo_exceptions = json.loads((root/'tools/fixtures/r102-baseline-exceptions.json').read_text())
for name, expected in manifest['files'].items():
    if name == 'src/fg_overlay.h':
        assert not (root / name).exists(), 'Legacy settings popup must remain removed'
        continue # R42 replaces UI-only code with fg_settings.h; renderer hashes remain enforced.
    data = (root / name).read_bytes().replace(b'\r\n', b'\n')
    if name in world_albedo_exceptions:
        text=data.decode()
        for before,after in reversed(world_albedo_exceptions[name]):
            assert text.count(after)==1, (name, 'R102 world guide history changed or duplicated')
            text=text.replace(after,before,1)
        data=text.encode()
    if name in gi_filter_exceptions:
        text=data.decode()
        for before,after in reversed(gi_filter_exceptions[name]):
            assert text.count(after)==1, (name, 'R100 GI filter integration changed or duplicated')
            text=text.replace(after,before,1)
        data=text.encode()
    if name in rr_path_exceptions:
        text=data.decode()
        for before,after in reversed(rr_path_exceptions[name]):
            assert text.count(after)==1, (name, 'R99 path instrumentation changed or duplicated')
            text=text.replace(after,before,1)
        data=text.encode()
    if name in rr_filter_exceptions:
        text=data.decode()
        for before,after in reversed(rr_filter_exceptions[name]):
            assert text.count(after)==1, (name, 'R98 diffuse filter diagnostic changed or duplicated')
            text=text.replace(after,before,1)
        data=text.encode()
    if name in shadow_exceptions:
        text=data.decode()
        for before,after in reversed(shadow_exceptions[name]):
            assert text.count(after)==1, (name, 'R67 shadow filter integration changed or duplicated')
            text=text.replace(after,before,1)
        data=text.encode()
    if name in isolation_exceptions:
        text=data.decode()
        for before,after in reversed(isolation_exceptions[name]):
            assert text.count(after)==1, (name, 'R54 live isolation changed or duplicated')
            text=text.replace(after,before,1)
        data=text.encode()
    if name in provider_exceptions:
        text=data.decode()
        for before,after in reversed(provider_exceptions[name]):
            assert text.count(after)==1, (name, 'R49 provider selection changed or duplicated')
            text=text.replace(after,before,1)
        data=text.encode()
    if name in fixed_exceptions:
        text=data.decode()
        for before,after in reversed(fixed_exceptions[name]):
            assert text.count(after)==1, (name, 'R46 fixed RR policy changed or duplicated')
            text=text.replace(after,before,1)
        data=text.encode()
    if name in menu_exceptions:
        text=data.decode()
        for before,after in reversed(menu_exceptions[name]):
            assert text.count(after)==1, (name, 'R41 menu integration changed or duplicated')
            text=text.replace(after,before,1)
        data=text.encode()
    if name in glass_exceptions:
        text=data.decode()
        for before,after in reversed(glass_exceptions[name]):
            assert text.count(after)==1, (name, 'R34 glass restoration changed or duplicated')
            text=text.replace(after,before,1)
        data=text.encode()
    if name in perf_exceptions:
        text=data.decode()
        for before,after in reversed(perf_exceptions[name]):
            assert text.count(after)==1, (name, 'R32 diagnostic insertion changed or duplicated')
            text=text.replace(after,before,1)
        data=text.encode()
    if name in live_exceptions:
        text=data.decode()
        for before,after in reversed(live_exceptions[name]):
            assert text.count(after)==1, (name, 'R31 live UI insertion changed or duplicated')
            text=text.replace(after,before,1)
        data=text.encode()
    if name in exceptions:
        text=data.decode()
        for before,after in exceptions[name]:
            assert text.count(after)==1, (name, 'R28 capture insertion changed or duplicated')
            text=text.replace(after,before,1)
        data=text.encode()
    assert hashlib.sha256(data).hexdigest() == expected, name
print(f"PASS: {len(manifest['files'])} baseline files verified, allowing exact R28 capture and R31 live UI/reset and R32 profiling changes, against {manifest['commit']}")
