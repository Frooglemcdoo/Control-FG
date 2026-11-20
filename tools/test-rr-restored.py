from pathlib import Path
import hashlib,json
root=Path(__file__).resolve().parents[1]
m=json.loads((root/'tools/fixtures/r489-approved-rendering.json').read_text())
for name,digest in m['files'].items():
 assert hashlib.sha256((root/name).read_bytes()).hexdigest()==digest,name
ui=(root/'src/ui/rr_controls.h').read_text()
assert 'VK_F10' in ui and 'VK_F7' not in ui and 'VK_CONTROL' not in ui
assert 'control_rr::RRUserLabel()' in ui and 'control_rr_capture_ui::Request()' in ui
assert 'gi_scene_capture::Motion()' in ui and 'PublishCaptureActivity' in ui
assert 'gi_reuse::ownedRequested.load()&&gi_reuse::singleEvaluatorReady.load()' in (root/'src/gi_live_toggle.h').read_text()
assert 'if(!Enabled()||!requested.load()||!native)' in (root/'src/gi_evaluator_runtime.inl').read_text()
assert 'owned_gi_not_selected' in (root/'src/gi_auto_runtime.h').read_text()
schema=json.loads((root/'src/settings_schema.json').read_text())
assert not any(o['key']=='ReblurSharpening' for o in schema['options'])
assert (root/'src/gi_reuse/motion_capture_layout.h').read_text().find('Frames=16,StopCue=4')>=0
print('PASS approved material/hair/clamp/streaming source hashes; shared GI selection gates; slim F10 controls; full 16-frame capture retained')
