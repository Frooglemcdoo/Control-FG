#include "../src/pt_surface_history.h"
#include <cassert>
#include <cstdio>
struct View {unsigned long long engineFrame=0;};
struct Assets {bool probe=false;};
struct Frame {
    bool producerSubmitted=false,armed=false,cancelled=false;
    std::shared_ptr<Assets> assets=std::make_shared<Assets>();
    std::shared_ptr<int> output=std::make_shared<int>(42);View view;
};
int main(){
    std::shared_ptr<int> history;View view;
    auto f=std::make_shared<Frame>();f->view.engineFrame=1;
    // Normal admission-before-submission.
    f->armed=true;assert(!PTSurfaceHistory::Publish(f,true,history,view));
    f->producerSubmitted=true;assert(PTSurfaceHistory::Publish(f,true,history,view));
    assert(history==f->output&&view.engineFrame==1);
    // Actual R70 regression: submission-before-late-COPY admission.
    auto late=std::make_shared<Frame>();late->view.engineFrame=2;late->producerSubmitted=true;
    assert(!PTSurfaceHistory::Publish(late,true,history,view));
    late->armed=true;assert(PTSurfaceHistory::Publish(late,true,history,view));
    assert(history==late->output&&view.engineFrame==2);
    assert(!PTSurfaceHistory::Publish(f,true,history,view));
    assert(!PTSurfaceHistory::Publish(late,true,history,view));
    auto invalid=std::make_shared<Frame>();invalid->view.engineFrame=3;invalid->armed=invalid->producerSubmitted=true;
    invalid->cancelled=true;assert(!PTSurfaceHistory::Publish(invalid,true,history,view));invalid->cancelled=false;
    invalid->assets->probe=true;assert(!PTSurfaceHistory::Publish(invalid,true,history,view));invalid->assets->probe=false;
    assert(!PTSurfaceHistory::Publish(invalid,false,history,view));
    invalid->assets.reset();assert(!PTSurfaceHistory::Publish(invalid,true,history,view));
    std::weak_ptr<int> retained=late->output;late.reset();assert(!retained.expired());
    puts("PASS: history publication for normal and late COPY admission, probe/cancel/replay rejection and retained lifetime");
}
