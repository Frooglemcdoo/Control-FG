#include "../src/pt_glass_hook_table.h"
#include <cassert>
#include <cstdio>
int main(){
 PTGlass::HookTable registry;int a=1,b=2,hook=3,later=4;
 void* native[11]{};void* proxy[11]{};native[10]=&a;proxy[10]=&b;unsigned writes=0;
 auto install=[&](void** slot,void* expected,void* value){assert(*slot==expected);*slot=value;++writes;return true;};
 assert(registry.Ensure(native,10,&hook,install));assert(registry.Ensure(proxy,10,&hook,install));
 assert(registry.Find(native)==&a&&registry.Find(proxy)==&b&&writes==2);
 assert(registry.Ensure(native,10,&hook,install)&&writes==2);
 native[10]=&later;assert(registry.Ensure(native,10,&hook,install)&&writes==2&&native[10]==&later);
 assert(registry.Find(native)==&a); // preserve a later interposer's chain
 void* failed[11]{};failed[10]=&a;
 assert(!registry.Ensure(failed,10,&hook,[](void**,void*,void*){return false;}));assert(!registry.Find(failed)&&failed[10]==&a);
 assert(registry.Ensure(failed,10,&hook,install));
 void* unknown[11]{};unknown[10]=&hook;assert(!registry.Ensure(unknown,10,&hook,install));assert(!registry.Find(unknown));
 puts("PASS: distinct vtables retain distinct predecessors, repeated installation and later chaining do not recurse; failures remain retryable");
}
