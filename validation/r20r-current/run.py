from pathlib import Path
import hashlib, json, sys, re
root=Path(__file__).resolve().parents[2]
checks=[]
def ck(name, cond):
    checks.append((name,bool(cond)))
    if not cond:
        raise SystemExit('FAIL '+name)
uc=(root/'src/rr_user_control.h').read_text()
nf=(root/'src/rr_native_frame.h').read_text()
ev=(root/'src/rr_evaluation_entry.h').read_text()
ov=(root/'src/fg_overlay.h').read_text()
np=(root/'src/rr_native_preset.h').read_text()
op=(root/'src/rr_options_r20p.h').read_text()
probe=(root/'src/probe.cpp').read_text()
metadata=(root/'Build-Metadata.ps1').read_text()
ck('identity', 'r20r-rr-native-g12' in probe and "Version = '1.0.0-RR-Native-G12-r20r'" in metadata)
ck('modes', all(x in uc for x in ['RRUserMode::Off','RRUserMode::Partial','RRUserMode::Full','RRUserPartialRequested','RRUserAnyRequested']))
ck('partial-no-registered-true', '*value=0' in nf and '*value=1' not in nf and 'RRNativeWriteOption' not in nf)
ck('partial-option-mirror', 'rrNativeFrame.Selected()||RRNativePartialActive()' in nf and 'NativeRenderOption(rrNativeFrameEnabled,rrNativeFrame.Selected()||partial' in nf)
ck('partial-native-feature-off', 'rrNativeResetOriginal(outputWidth,outputHeight,width,height,flag0,flag1,selected,flag3,reset)' in nf and 'partialRequested' in nf)
ck('partial-filter', 'RR_PARTIAL_FILTER' in nf and 'temporal_substitution=0' in nf)
ck('partial-gi', 'RR_PARTIAL_GI' in nf)
ck('partial-evaluation', 'RR_PARTIAL_EVALUATION_FORWARD' in ev and 'guides_overridden=0' in ev and 'distance_overridden=0' in ev)
ck('full-evaluation-still-present', 'RRNativeSetGuides' in ev and 'RR_NATIVE_EVALUATED' in ev)
ck('preset-restart-gate', 'RRUserPresetNeedsRestart' in uc and 'RR_MODE_SELECTION_BLOCKED mode=full reason=preset_changed_restart_required' in ov)
ck('preset-create-confirmation', 'RRUserConfirmPresetCreate' in np and 'native_create_confirmed' in ev)
ck('preset-options', all(x in op for x in ['RR20PApplySelectedPreset','control_rr::RRUserPresetValue','preset=%s']))
ck('overlay-visible', 'kFGOverlayHeight = 970' in ov and 'L"PARTIAL"' in ov and 'L"FULL RR"' in ov and 'v1.0 RR r20r' in ov)
ck('partial-sharpness-available', 'const bool rrRequested=control_rr::RRUserRequested();' in ov)
# frozen files
frozen=json.loads((root/'g3-frozen-files.json').read_text())['Files']
ck('frozen-files', all(hashlib.sha256((root/i['Path']).read_bytes()).hexdigest().upper()==i['SHA256'].upper() for i in frozen))
# simple braces sanity for changed headers
for f in ['src/rr_user_control.h','src/rr_native_frame.h','src/rr_evaluation_entry.h','src/fg_overlay.h','src/rr_native_preset.h','src/rr_options_r20p.h']:
    s=(root/f).read_text()
    ck('braces-'+Path(f).name, s.count('{')==s.count('}'))
result={'status':'LOCAL_PASS_WINDOWS_GPU_REQUIRED','version':'1.0.0-RR-Native-G12-r20r','checks':[n for n,v in checks if v], 'count':len(checks)}
(root/'validation/r20r-current/results.json').write_text(json.dumps(result,indent=2)+'\n')
print('PASS',len(checks),'r20r checks')
