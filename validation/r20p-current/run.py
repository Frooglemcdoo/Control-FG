from pathlib import Path
import hashlib, json, re
root=Path(__file__).resolve().parents[2]

def text(rel): return (root/rel).read_text(encoding='utf-8-sig')
def sha(rel): return hashlib.sha256((root/rel).read_bytes()).hexdigest()
checks={}

def ck(name, value):
    checks[name]=bool(value)
    if not value: raise AssertionError(name)

probe=text('src/probe.cpp')
overlay=text('src/fg_overlay.h')
frame=text('src/streamline_frame.h')
options=text('src/rr_options_r20p.h')
skin=text('src/rr_skin_diagnostic.h')
entry=text('src/rr_evaluation_entry.h')
user=text('src/rr_user_control.h')
meta=text('Build-Metadata.ps1')
doc=text('RR_NATIVE_G12_R20P.md')

ck('identity', 'v1.0.0-RR-Native-G12-r20p' in probe and 'source_revision=r20p-rr-native-g12' in probe and "Version = '1.0.0-RR-Native-G12-r20p'" in meta)
ck('visible_build_id', 'L"v1.0 RR r20p"' in overlay and 'RR P1' not in overlay)
height=int(re.search(r'kFGOverlayHeight\s*=\s*(\d+)',overlay).group(1))
ck('overlay_height', height>=820)
ck('sharpness_visible', 'AA SHARPNESS  (SR / DLAA)' in overlay and 'PaintFGPercentSlider' in overlay and 'FGOverlayAASharpnessSliderFromPoint' in overlay)
ck('sharpness_bounds', all(v < height for v in [738,784,732,792,741,781,818]))
ck('sharpness_persistence', 'L"AntiAliasing", L"SharpnessOverride"' in overlay and 'L"AntiAliasing", L"SharpnessPercent"' in overlay and 'schema=3' in overlay)
ck('sharpness_scope_honest', 'RR ignores the DLSS sharpness control' in overlay and 'options.sharpness=0.0f' in options and 'rr_sharpness_ignored_by_api=1' in options)
ck('native_aa_override', 'AA_SHARPNESS_OVERRIDE' in probe and 'appliedF3' in probe and '!control_rr::RRUserRequested()' in probe)
ck('explicit_rr_eoff', 'off.mode=sl::DLSSMode::eOff' in options and 'slDLSSDSetOptionsApi(slFgViewport,off)' in options and 'SL_RR_OPTIONS_DISABLED' in options and 'rr_user_disabled_explicit_eoff' in options)
ck('mode_rebuild', 'SL_RR_OPTIONS_REBUILD' in options and 'aa_mode_or_extent_changed' in options and 'ConfigureRR20POptionsForAA' in probe)
ck('skin_stages', all(s in entry for s in ['"rr_pre"','"rr_post"','"sr_pre"','"sr_post"']))
ck('skin_inputs', all(s in skin for s in ['DLSSD.ScreenSpaceSubsurfaceScatteringGuide','DLSSD.ColorBeforeScreenSpaceSubsurfaceScattering','DLSSD.ColorAfterScreenSpaceSubsurfaceScattering','GBuffer.Subsurface','DLSSD.ResponsivityMask']))
ck('skin_shape_validation', 'DXGI_FORMAT_R16_FLOAT' in skin and 'native_pair_valid' in skin and 'guide_shape_valid' in skin)
ck('skin_no_fake_guide', 'sss_injection=0' in skin and 'character_mask_injection=0' in skin and 'next_mutation=disabled_this_build' in skin)
ck('skin_future_paths', all(s in skin for s in ['native_sss_guide','derive_true_sss_guide_from_native_pair','character_mask_then_bypass_or_responsivity_experiment']))
ck('docs', 'RR-bypass/composite' in doc and 'RR_SKIN_PATH' in doc and 'AA SHARPNESS (SR / DLAA)' in doc)

files=['src/probe.cpp','src/fg_overlay.h','src/streamline_frame.h','src/rr_options_r20p.h','src/rr_skin_diagnostic.h','src/rr_evaluation_entry.h','src/rr_user_control.h','Build-Metadata.ps1','RR_NATIVE_G12_R20P.md']
out={
 'status':'PASS',
 'checks':checks,
 'tested_sha256':{f:sha(f) for f in files},
 'limitations':['Static/portable validation only; Windows/MSVC and GPU execution remain required','Skin/SSS path is observation-only in r20p; no bypass/composite mutation is enabled']
}
Path(__file__).with_name('results.json').write_text(json.dumps(out,indent=2)+'\n')
print('PASS',len(checks),'r20p checks')
