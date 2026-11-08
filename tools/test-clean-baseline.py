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
specular_filter_exceptions = json.loads((root/'tools/fixtures/r103-baseline-exceptions.json').read_text())
clamp_90_exceptions = json.loads((root/'tools/fixtures/r104-baseline-exceptions.json').read_text())
restore_60_exceptions = json.loads((root/'tools/fixtures/r105-baseline-exceptions.json').read_text())
temporal_exceptions = json.loads((root/'tools/fixtures/r111-baseline-exceptions.json').read_text())
diagnostic_exceptions = json.loads((root/'tools/fixtures/r116-baseline-exceptions.json').read_text())
compact_exceptions = json.loads((root/'tools/fixtures/r190-baseline-exceptions.json').read_text())
suite_exceptions = json.loads((root/'tools/fixtures/r191-baseline-exceptions.json').read_text())
camera_exceptions = json.loads((root/'tools/fixtures/r199-baseline-exceptions.json').read_text())
gi_temporal_exceptions=json.loads((root/'tools/fixtures/r208-baseline-exceptions.json').read_text())
motion_exceptions=json.loads((root/'tools/fixtures/r210-baseline-exceptions.json').read_text())
pipeline_exceptions=json.loads((root/'tools/fixtures/r211-baseline-exceptions.json').read_text())
foliage_exceptions=json.loads((root/'tools/fixtures/r265-baseline-exceptions.json').read_text())
clamp_ab_exceptions=json.loads((root/'tools/fixtures/r276-baseline-exceptions.json').read_text())
queue_boundary_exceptions=json.loads((root/'tools/fixtures/r317-baseline-exceptions.json').read_text())
diffuse_source_exceptions=json.loads((root/'tools/fixtures/r361-baseline-exceptions.json').read_text())
source_capture_exceptions=json.loads((root/'tools/fixtures/r362-baseline-exceptions.json').read_text())
performance_exceptions=json.loads((root/'tools/fixtures/r365-baseline-exceptions.json').read_text())
demand_exceptions=json.loads((root/'tools/fixtures/r368-baseline-exceptions.json').read_text())
for name, expected in manifest['files'].items():
    if name == 'src/fg_overlay.h':
        assert not (root / name).exists(), 'Legacy settings popup must remain removed'
        continue # R42 replaces UI-only code with fg_settings.h; renderer hashes remain enforced.
    data = (root / name).read_bytes().replace(b'\r\n', b'\n')
    # R386 adds only gated observation hooks to these baseline files.
    settings_check_hooks = {
        'src/rr_native_frame.h': ' settings_check::RecordGISamples(effective);\n',
        'src/rr_clamp_strength.h': ' settings_check::Record(changed&&strength==60?settings_check::Clamp60:settings_check::ClampOther);\n',
        'src/rr_clamp_strength_policy.h': '#include "settings_check_state.h"\n',
        'src/rr_user_control.h': '#include "settings_check_state.h"\n',
        'src/rr_evaluation_entry.h': '        settings_check::Record(success?settings_check::RRSuccess:settings_check::RRFailure);\n',
    }
    if name in settings_check_hooks:
        hook = settings_check_hooks[name].encode()
        assert data.count(hook) == 1, (name, 'R386 gated observation hook changed')
        data = data.replace(hook, b'', 1)
    if name == 'src/rr_native_frame.h':
        for hook in ['  if(full)settings_check::Record(settings_check::BroadNativeWithRR);\n',
                     ' if(scope.preserveNativeFiltering)settings_check::Record(settings_check::SpecularNativeWithRR);\n']:
            assert data.count(hook.encode()) == 1
            data = data.replace(hook.encode(), b'', 1)
    if name in demand_exceptions:
        text=data.decode()
        for before,after in reversed(demand_exceptions[name]):
            assert text.count(after)==1,(name,'R368 on-demand scope changed')
            text=text.replace(after,before,1)
        data=text.encode()
    if name in source_capture_exceptions:
        text=data.decode()
        for before,after in reversed(source_capture_exceptions[name]):
            assert text.count(after)==1,(name,'R362 active-source capture scope changed')
            text=text.replace(after,before,1)
        data=text.encode()
    if name in diffuse_source_exceptions:
        text=data.decode()
        for before,after in reversed(diffuse_source_exceptions[name]):
            assert text.count(after)==1,(name,'R361 upstream diffuse restoration scope changed')
            text=text.replace(after,before,1)
        data=text.encode()
    if name in performance_exceptions:
        text=data.decode()
        for before,after in reversed(performance_exceptions[name]):
            assert text.count(after)==1,(name,'performance scope changed')
            text=text.replace(after,before,1)
        data=text.encode()
    if name == 'src/rr_clamp_strength_policy.h':
        # R354 user-requested startup selection only; retain the full historical
        # shader/dispatch baseline check after reversing this one exact edit.
        before=b'inline std::atomic<Mode> requestedMode{Mode::Native};'
        after=b'inline std::atomic<Mode> requestedMode{Mode::Override60};'
        assert data.count(after)==1, 'R354 startup clamp must be60'
        data=data.replace(after,before,1)
    if name in queue_boundary_exceptions:
        text=data.decode()
        for before,after in reversed(queue_boundary_exceptions[name]):
            assert text.count(after)==1,(name,'R317 exact game queue observation boundary changed')
            text=text.replace(after,before,1)
        data=text.encode()
    if name in clamp_ab_exceptions:
        text=data.decode()
        for before,after in reversed(clamp_ab_exceptions[name]):
            assert text.count(after)==1,(name,'R276 clamp toggle scope changed')
            text=text.replace(after,before,1)
        data=text.encode()
    if name in foliage_exceptions:
        text=data.decode()
        for before,after in reversed(foliage_exceptions[name]):
            assert text.count(after)==1,(name,'R265 exact foliage evidence guard changed')
            text=text.replace(after,before,1)
        data=text.encode()
    if name in pipeline_exceptions:
        text=data.decode()
        for before,after in reversed(pipeline_exceptions[name]):
            assert text.count(after)==1,(name,'R211 pipeline scope changed')
            text=text.replace(after,before,1)
        data=text.encode()
    if name in motion_exceptions:
        text=data.decode()
        for before,after in reversed(motion_exceptions[name]):
            assert text.count(after)==1,(name,'R210 transport changed')
            text=text.replace(after,before,1)
        data=text.encode()
    if name in gi_temporal_exceptions:
        text=data.decode()
        for before,after in reversed(gi_temporal_exceptions[name]):
            assert text.count(after)==1, (name, 'R208 scoped GI temporal integration changed')
            text=text.replace(after,before,1)
        data=text.encode()
    if name in camera_exceptions:
        text=data.decode()
        for before,after in reversed(camera_exceptions[name]):
            assert text.count(after)==1, (name, 'R199 camera install scope changed or duplicated')
            text=text.replace(after,before,1)
        data=text.encode()
    if name in suite_exceptions:
        text=data.decode()
        for before,after in reversed(suite_exceptions[name]):
            assert text.count(after)==1, (name, 'R191 suite scope changed or duplicated')
            text=text.replace(after,before,1)
        data=text.encode()
    if name in compact_exceptions:
        text=data.decode()
        for before,after in reversed(compact_exceptions[name]):
            assert text.count(after)==1, (name, 'R190 compact distance scope changed or duplicated')
            text=text.replace(after,before,1)
        data=text.encode()
    if name in diagnostic_exceptions:
        text=data.decode()
        for before,after in reversed(diagnostic_exceptions[name]):
            assert text.count(after)==1, (name, 'R116 diagnostic bypass changed or duplicated')
            text=text.replace(after,before,1)
        data=text.encode()
    if name in temporal_exceptions:
        text=data.decode()
        for before,after in reversed(temporal_exceptions[name]):
            assert text.count(after)==1, (name, 'R111 scoped temporal diagnostic changed or duplicated')
            text=text.replace(after,before,1)
        data=text.encode()
    if name in restore_60_exceptions:
        text=data.decode()
        for before,after in reversed(restore_60_exceptions[name]):
            assert text.count(after)==1, (name, 'R105 clamp restore changed or duplicated')
            text=text.replace(after,before,1)
        data=text.encode()
    if name in clamp_90_exceptions:
        text=data.decode()
        for before,after in reversed(clamp_90_exceptions[name]):
            assert text.count(after)==1, (name, 'R104 clamp strength changed or duplicated')
            text=text.replace(after,before,1)
        data=text.encode()
    if name in specular_filter_exceptions:
        text=data.decode()
        for before,after in reversed(specular_filter_exceptions[name]):
            assert text.count(after)==1, (name, 'R103 specular filter integration changed or duplicated')
            text=text.replace(after,before,1)
        data=text.encode()
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
