#include "../../src/rr_specular_clamp_policy.h"
#include <cassert>
#include <cstdio>
using control_rr_specular_policy::Choose;
using control_rr_specular_policy::Path;
int main(){
  // Native is the default whenever its bind-stage lease was prepared.
  assert(Choose(false,false,true)==Path::NativeClamp);
  assert(Choose(false,true,true)==Path::NativeClamp);
  // A reference selection wins only when it was ready before the native lease
  // was armed. A late reference readiness change never switches mid-pass.
  assert(Choose(true,true,false)==Path::Reference);
  assert(Choose(true,true,true)==Path::NativeClamp);
  assert(Choose(true,false,true)==Path::NativeClamp);
  // Without either proven path, the old mip-0 copy remains the emergency path.
  assert(Choose(false,false,false)==Path::RawCopy);
  assert(Choose(true,false,false)==Path::RawCopy);
  std::puts("PASS: r21x specular signal policy keeps native CameraCut clamp sticky across bind->dispatch, reference only when ready before bind, raw copy only as fallback");
  return 0;
}
