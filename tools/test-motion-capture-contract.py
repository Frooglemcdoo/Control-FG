"""Check motion capture wiring and preservation of the user's active lighting."""
from pathlib import Path
root=Path(__file__).resolve().parents[1]
ui=(root/'src/ui/gi_path_capture.h').read_text()
record=(root/'src/rr_input_capture.h').read_text()
assert 'if(condition!=3&&condition!=4)rt_surface_source::request.store(savedRequest);' in ui
assert 'if(!motion)rt_surface_source::request.store(8);' in ui
assert 'condition=motion?(both?4:3):1;' in ui
assert ui.index('if(condition==3||condition==4){') < ui.index('condition=2;waiting=false')
assert 'rt_surface_source::manualGI.load()!=4||rt_surface_source::request.load()!=0' in ui
assert 'rt_surface_source::request.load()!=savedRequest' in ui
assert 'strncmp(reason,"complete:",9)!=0' in ui
assert 'ExportStride(gi_path_capture::condition.load())' in record
assert 'PairAligned(c->pathCondition,c->pairIndex,c->previousFrame,c->frame)' in record
assert 'if(SUCCEEDED(hr)&&!c->pathCondition)hr=GIBatchCapturePrepare' in record
assert '((c->pathCondition==3||c->pathCondition==4)?1:c->pathCondition?2:4)' in record
print('PASS motion-mode wiring: no settings switch, full-resolution pair, no GI-hit dump, explicit incomplete outcome')
