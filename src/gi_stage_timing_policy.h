#pragma once
#include <cstdint>
namespace gi_stage_timing {
enum class Stage : unsigned { Evaluator, TemporalDispatch, SpatialDispatch, Reconstruction, Count };
inline const char* Name(Stage stage) noexcept {
 constexpr const char* names[]{"gi_evaluator","gi_temporal_or_raw_copy","gi_spatial","rr_evaluation"};
 return unsigned(stage)<unsigned(Stage::Count)?names[unsigned(stage)]:"invalid";
}
inline constexpr unsigned capacity=128;
struct Ledger {
 std::uint64_t frame=0,present=0;
 unsigned count=0,ended=0,seen[unsigned(Stage::Count)]{};
 unsigned expectedBatches=0,observedBatches=0;bool batchInvalid=false;
 bool overflow=false;
 unsigned Begin(std::uint64_t f,std::uint64_t p,Stage stage) noexcept {
  if(!f||unsigned(stage)>=unsigned(Stage::Count)|| (frame&&frame!=f))return ~0u;
  if(count==capacity){overflow=true;return ~0u;}
  frame=f;present=p;++seen[unsigned(stage)];return count++;
 }
 void Batch(unsigned batch,unsigned total,bool valid) noexcept {
  if(!valid||!total||batch!=observedBatches||batch>=total||(expectedBatches&&total!=expectedBatches)){batchInvalid=true;return;}
  expectedBatches=total;++observedBatches;
 }
 bool Complete(std::uint64_t f)const noexcept {
  return frame==f&&!overflow&&count==ended&&seen[0]>0&&seen[1]==1&&seen[3]==1&&!batchInvalid&&expectedBatches&&observedBatches==expectedBatches&&seen[0]==expectedBatches;
 }
};
inline bool Milliseconds(std::uint64_t a,std::uint64_t b,std::uint64_t frequency,double& ms) noexcept {
 if(!frequency||b<a)return false;
 ms=1000.0*double(b-a)/double(frequency);return true;
}
}
