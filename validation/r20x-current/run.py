from pathlib import Path
import hashlib, json, subprocess, tempfile
root=Path(__file__).resolve().parents[2]
src=Path(__file__).with_name('mode-test.cpp')
out=Path(tempfile.gettempdir())/'control_rr_r20x_mode_test'
subprocess.run(['g++','-std=c++17','-O2',str(src),'-o',str(out)],check=True)
subprocess.run([str(out)],check=True)
files=['src/rr_user_control.h','src/rr_native_frame.h','src/rr_native_preset.h','src/rr_options_r20p.h','src/fg_overlay.h','validation/r20x-current/mode-test.cpp']
sha={f:hashlib.sha256((root/f).read_bytes()).hexdigest().upper() for f in files}
required={
 'src/rr_native_frame.h':['RR_PRESET_EPOCH_BEGIN','RR_PRESET_EPOCH_OPTIONS_WINDOW','RR_PRESET_EPOCH_ACTIVATE','RR20POptionsReadyForPreset'],
 'src/rr_native_preset.h':['RR_PRESET_PARAMETER_SYNC','evaluation_parameter_sync'],
 'src/rr_options_r20p.h':['RRUserPresetOptionsWindow','allowPresetRebuild'],
 'src/fg_overlay.h':['preset_epoch_requested','RRUserSetPresetTransitionPaused(true)']
}
for f,markers in required.items():
    text=(root/f).read_text(errors='ignore')
    for m in markers:
        assert m in text,(f,m)
result={'status':'PASS','checks':len(files)+sum(map(len,required.values())),'tested_sha256':sha,
        'contract':'E/F transition pauses before preset publication, drains old RR-consumption leases, rebuilds Streamline options while auxiliary work remains paused, resynchronizes shared NGX preset parameters, then activates fresh history.'}
Path(__file__).with_name('results.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
