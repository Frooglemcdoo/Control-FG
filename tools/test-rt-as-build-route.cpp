#include "../src/rt_as_build_route.h"
#include <cassert>
#include <stdexcept>

int main() {
 using namespace rt_as_build_route;
 unsigned native=0,replacement=0;
 Record(false,[&]{++native;},[&]{++replacement;});
 assert(native==1 && replacement==0);
 Record(true,[&]{++native;},[&]{++replacement;});
 assert(native==1 && replacement==1);
 // Even an interrupted replacement must not execute a native retry.
 try {Record(true,[&]{++native;},[&]{++replacement;throw std::runtime_error("recording interrupted");});}
 catch(const std::runtime_error&) {}
 assert(native==1 && replacement==2);
 for(unsigned mask=0;mask<64;++mask) {
  Coverage c{bool(mask&1),bool(mask&2),bool(mask&4),bool(mask&8),bool(mask&16),bool(mask&32)};
  assert(c.Complete()==(mask==63));
 }
}
