#include "../src/rt_as_ownership.h"
#include <cassert>
#include <functional>
using namespace rt_as_ownership;
static bool Reject(const std::function<void()>& f){try{f();return false;}catch(const std::exception&){return true;}}
int main(){
 const rt_as_build_route::Coverage full{true,true,true,true,true,true};
 Ledger book;auto resource=std::make_shared<int>(7);std::weak_ptr<int> weak=resource;
 const auto as=book.Register(0x1000,256,resource,true,full,0x9000);resource.reset();
 assert(book.Resolve(as)==0x9000&&book.Select(as,true)==Route::Replacement);
 assert(book.Select(as,false)==Route::Abort); // Cannot feed a PTLAS to native.
 const auto list=book.BeginList();book.Reference(list,as);book.Reference(list,as);book.Close(list);
 assert(Reject([&]{book.Reference(list,as);}));
 const auto first=book.PrepareSubmit({list},1,101,1);book.Executing(first);book.Submitted(first);
 const auto replay=book.PrepareSubmit({list},1,101,2);book.Executing(replay);book.Submitted(replay);
 const auto other=book.PrepareSubmit({list},2,202,1);book.Executing(other);book.Submitted(other);
 assert(Reject([&]{book.PrepareSubmit({list},2,101,3);}));
 book.Retire(as);book.Discard(list);
 assert(Reject([&]{book.Resolve(as);}));
 assert(Reject([&]{book.Register(0x1000,256,std::make_shared<int>(),false,{},0);}));
 book.Complete(1,101,1);assert(!weak.expired()&&book.Pending()==2);
 book.Complete(1,101,2);assert(!weak.expired()&&book.Pending()==1);
 assert(Reject([&]{book.Complete(1,101,1);}));
 book.Complete(2,202,1);assert(weak.expired()&&book.Pending()==0);
 const auto reused=book.Register(0x1000,256,std::make_shared<int>(),true,{},0);
 assert(reused.generation!=as.generation&&book.Select(reused,true)==Route::Native&&book.Resolve(reused)==0x1000);
 assert(Reject([&]{book.Retire(as);}));
 // Reset/discard before execution releases only that recording's references.
 const auto abandoned=book.BeginList();book.Reference(abandoned,reused);book.Close(abandoned);
 const auto cancelled=book.PrepareSubmit({abandoned},1,101,3);book.CancelBeforeExecute(cancelled);book.Discard(abandoned);
 // Signal failure keeps resources quarantined even if another signal advances.
 auto held=std::make_shared<int>();std::weak_ptr<int> quarantined=held;
 const auto bad=book.Register(0x3000,256,held,true,full,0xb000);held.reset();
 const auto failing=book.BeginList();book.Reference(failing,bad);book.Close(failing);
 const auto ticket=book.PrepareSubmit({failing},1,101,4);book.Executing(ticket);assert(Reject([&]{book.CancelBeforeExecute(ticket);}));book.Quarantine(ticket);
 book.Retire(bad);book.Discard(failing);book.Complete(1,101,4);assert(!quarantined.expired()&&book.Pending()==1);
 assert(Reject([&]{book.CancelBeforeExecute(ticket);}));
 // A closed, replayable list must retain its references after GPU completion.
 auto kept=std::make_shared<int>();std::weak_ptr<int> retained=kept;
 const auto k=book.Register(0x5000,256,kept,false,{});kept.reset();
 const auto repeat=book.BeginList();book.Reference(repeat,k);book.Close(repeat);
 auto run=book.PrepareSubmit({repeat},3,303,1);book.Executing(run);book.Submitted(run);book.Retire(k);book.Complete(3,303,1);assert(!retained.expired());
 run=book.PrepareSubmit({repeat},3,303,2);book.Executing(run);book.Submitted(run);book.Discard(repeat);assert(!retained.expired());book.Complete(3,303,2);assert(retained.expired());
}
