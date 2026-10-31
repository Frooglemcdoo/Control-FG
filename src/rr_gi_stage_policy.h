#pragma once
#include <atomic>
namespace control_rr_gi {
enum class StageMode : unsigned {Both,TemporalOnly,SpatialOnly,ClampOnly,BothSingleHistory};
inline std::atomic<StageMode> stageMode{StageMode::ClampOnly};
// Render-thread scoped, never re-read the UI request during a filter invocation.
inline thread_local bool historyDisabled=false;
inline thread_local bool spatialDispatch=false;
inline std::atomic<bool> outlierRequested{true};
inline thread_local bool outlierEnabled=false;
inline bool UseOutlier(StageMode mode,bool requested){return mode==StageMode::ClampOnly&&requested;}
enum class DiagnosticView : unsigned {Normal,Mask,Zero};
inline std::atomic<DiagnosticView> viewRequested{DiagnosticView::Normal};
inline thread_local DiagnosticView activeView=DiagnosticView::Normal;
inline DiagnosticView EffectiveView(StageMode mode,DiagnosticView v){return mode==StageMode::ClampOnly?v:DiagnosticView::Normal;}
inline const char* ViewName(DiagnosticView v){return v==DiagnosticView::Mask?"cleanup_mask":v==DiagnosticView::Zero?"zero_gi":"normal";}
inline const wchar_t* ViewLabel(DiagnosticView v){return v==DiagnosticView::Mask?L"GI VIEW: MASK":v==DiagnosticView::Zero?L"GI VIEW: ZERO":L"GI VIEW: NORMAL";}
inline bool Temporal(StageMode m){return m==StageMode::Both||m==StageMode::BothSingleHistory||m==StageMode::TemporalOnly;}
inline unsigned SpatialPasses(StageMode m,unsigned native){return m==StageMode::Both||m==StageMode::BothSingleHistory||m==StageMode::SpatialOnly?native:0;}
inline const char* StageName(StageMode m){switch(m){case StageMode::BothSingleHistory:return "both_single_history";case StageMode::Both:return "both";case StageMode::TemporalOnly:return "temporal_only";case StageMode::SpatialOnly:return "spatial_only";default:return "clamp_only";}}
inline const wchar_t* StageLabel(StageMode m){switch(m){case StageMode::BothSingleHistory:return L"GI: BOTH TEST";case StageMode::Both:return L"GI: BOTH";case StageMode::TemporalOnly:return L"GI: TEMP ONLY";case StageMode::SpatialOnly:return L"GI: SPATIAL ONLY";default:return L"GI: CLAMP ONLY";}}
inline StageMode NextStage(StageMode m){return static_cast<StageMode>((static_cast<unsigned>(m)+1)%5);}
}
