#pragma once
// Compile two binaries from identical rendering code; default is clean.
#ifndef CONTROL_RT_DIAGNOSTICS
#define CONTROL_RT_DIAGNOSTICS 0
#endif
#if CONTROL_RT_DIAGNOSTICS != 0 && CONTROL_RT_DIAGNOSTICS != 1
#error CONTROL_RT_DIAGNOSTICS must be 0 or 1
#endif
