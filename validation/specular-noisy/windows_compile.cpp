#include <windows.h>
#include <d3d12.h>
#include "../../src/rr_specular_noisy_windows.h"
// Compile in the next combined Windows gate. Not executed or wired to gameplay.
int main(){
 control_rr_specular::NativeCopyApi api;
 control_rr_specular::NativeHistoryReset reset;
 return api.Initialize(nullptr)||reset.Initialize(nullptr)||reset.ResetBeforeFilter()?1:0;
}
