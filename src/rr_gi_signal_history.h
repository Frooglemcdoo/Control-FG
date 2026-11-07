#pragma once
namespace control_rr {
struct GISignalHistory {
 bool initialized=false,selected=false,owned=false;
 unsigned phase=0,stage=0,lighting=0;
 bool Update(bool nextSelected,bool nextOwned,unsigned nextPhase,unsigned nextStage,unsigned nextLighting) noexcept {
  const bool changed=initialized&&nextSelected&&selected&&
   (owned!=nextOwned||phase!=nextPhase||stage!=nextStage||lighting!=nextLighting);
  initialized=true;selected=nextSelected;owned=nextOwned;phase=nextPhase;stage=nextStage;lighting=nextLighting;
  return changed;
 }
};
}
