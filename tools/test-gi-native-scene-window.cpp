#include "../src/gi_reuse/native_scene_window.h"
#include <cassert>
#include <chrono>
#include <cstdio>
#include <future>
#include <thread>
int main(){
 using gi_reuse::NativeSceneWindow;
 NativeSceneWindow window;
 assert(!window.BeforeJoin()&&!window.TryAcquire(1,1)&&!window.LeavePreparation());
 window.EnterPreparation();assert(!window.BeforeJoin()&&!window.TryAcquire(1,1));
 assert(window.LeavePreparation());const auto joined=window.BeforeJoin();assert(joined==1);
 assert(window.TryAcquire(10,joined)&&!window.TryAcquire(11,joined));
 std::promise<void> started,entered,finish;auto done=finish.get_future();
 auto thread=std::thread([&]{started.set_value();window.EnterPreparation();entered.set_value();done.wait();assert(window.LeavePreparation());});
 started.get_future().wait();auto admission=entered.get_future();
 // Wait for the actual gate waiter, not merely thread creation. No unbounded
 // loop: a broken fixture fails instead of hanging the build worker.
 const auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(3);
 while(!window.Inspect().waiting&&std::chrono::steady_clock::now()<deadline)std::this_thread::yield();
 assert(window.Inspect().waiting==1&&window.Inspect().preparations==0);
 assert(admission.wait_for(std::chrono::milliseconds(10))==std::future_status::timeout);
 assert(!window.Release(11)&&window.Inspect().reader==10);
 assert(window.Release(10));assert(admission.wait_for(std::chrono::seconds(3))==std::future_status::ready);
 assert(window.Inspect().preparations==1&&!window.TryAcquire(12,joined));
 finish.set_value();thread.join();
 assert(window.BeforeJoin()==2&&!window.TryAcquire(12,joined));
 // A preparation fully between join and acquire still invalidates the old
 // epoch. A callback return is not proof its asynchronously launched jobs ended.
 const auto second=window.BeforeJoin();window.EnterPreparation();assert(window.LeavePreparation());
 assert(!window.TryAcquire(12,second));assert(window.TryAcquire(12,window.BeforeJoin()));
 assert(window.Release(12)&&!window.Release(12));
 // The gate does not add serialization among concurrent native preparations.
 window.EnterPreparation();window.EnterPreparation();assert(window.Inspect().preparations==2);
 assert(window.LeavePreparation()&&!window.BeforeJoin());assert(window.LeavePreparation());
 assert(window.TryAcquire(13,window.BeforeJoin())&&window.Release(13));
 std::puts("PASS native preparation gate: waits before admission, exact reader release, stale join rejection and concurrent native writers.");
}
