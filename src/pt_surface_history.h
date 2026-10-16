#pragma once
#include <memory>
namespace PTSurfaceHistory {
// Publish only after BOTH GPU submission and admission. A late COPY signal
// reverses their usual order; callers must invoke this at either transition.
template<class Frame,class Output,class View>
bool Publish(const std::shared_ptr<Frame>& frame,bool enabled,std::shared_ptr<Output>& output,View& view){
    if(!enabled||!frame->producerSubmitted||!frame->armed||frame->cancelled||
       !frame->assets||frame->assets->probe)return false;
    if(output&&frame->view.engineFrame<=view.engineFrame)return false;
    output=frame->output;view=frame->view;return true;
}
}
