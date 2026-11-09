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
 unsigned aborted=0;
 for(unsigned bits=0;bits<8;++bits){
  const bool owned=(bits&1)!=0,coverage=(bits&2)!=0,prepared=(bits&4)!=0;
  const auto choice=Select(owned,coverage,prepared);
  const auto beforeNative=native,beforeReplacement=replacement,beforeAbort=aborted;
  Record(choice,[&]{++native;},[&]{++replacement;},[&]{++aborted;});
  if(coverage&&prepared){assert(replacement==beforeReplacement+1&&native==beforeNative&&aborted==beforeAbort);}
  else if(owned){assert(aborted==beforeAbort+1&&native==beforeNative&&replacement==beforeReplacement);}
  else{assert(native==beforeNative+1&&aborted==beforeAbort&&replacement==beforeReplacement);}
 }
 for(unsigned mask=0;mask<64;++mask) {
  Coverage c{bool(mask&1),bool(mask&2),bool(mask&4),bool(mask&8),bool(mask&16),bool(mask&32)};
  assert(c.Complete()==(mask==63));
 }
}
