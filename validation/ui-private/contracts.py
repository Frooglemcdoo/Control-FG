from pathlib import Path
r=Path(__file__).resolve().parents[2]
s=(r/'src/fg_ui_recomposition.h').read_text()
prepare=s[s.index('static FGUIRecomposeSlot* Prepare'):s.index('static bool FGUIRecomposeRecordHdrConversion')]
final=s[s.index('static bool Finalize'):s.index('// Native Present has flushed')]
native=final[:final.index('list=s.work.list;')]
for forbidden in ('SetDescriptorHeaps','SetPipelineState','SetComputeRoot','Dispatch(', 'slSetTagForFrameApi'):
 assert forbidden not in prepare and forbidden not in native, forbidden
assert prepare.index('s.work.Begin(')<prepare.index('FGUIRecomposeEnsureSlot(')
assert 'hd.NumDescriptors=18' in s and 'descriptorSlot=slotIndex+(s.hdr10?3u:0u)' in final
assert final.index('list=s.work.list;')<final.index('SetDescriptorHeaps')<final.index('slSetTagForFrameApi')<final.rindex('s.work.Finish()')
assert 'CopyResource(s.postHud,finalColor)' in native and 'std::swap(nativeBarrier.Transition.StateBefore,nativeBarrier.Transition.StateAfter)' in native
h=(r/'src/hdr10_bridge.h').read_text()
for name in ('Present','Present1'):
 a=h.index('HRESULT STDMETHODCALLTYPE '+name+'(');b=h.index('const UINT appliedSyncInterval',a);part=h[a:b]
 assert part.index('if (realPresent)')<part.index('SubmitFGUIRecompositionBeforePresent')<part.index('ObserveSLDisplayHdrDomainBeforePresent')<part.index('ConvertCurrentBackBuffer')
 assert part.index('SubmitFGUIRecompositionBeforePresent')<part.index('sdrCorrection_.Apply')
assert 'privateFailure' in s and 'FGUISameDevice(queueDevice,o->device,resolveNative)' in s
print('PASS native bindings untouched, copy state restored, descriptors partitioned, reuse before descriptor writes, private tag recording, both Present variants ordered before HDR/SDR/Streamline')
