#pragma once
// Compile two binaries from identical rendering code; default is clean.
#ifndef CONTROL_RT_DIAGNOSTICS
#define CONTROL_RT_DIAGNOSTICS 0
#endif
#if CONTROL_RT_DIAGNOSTICS != 0 && CONTROL_RT_DIAGNOSTICS != 1
#error CONTROL_RT_DIAGNOSTICS must be 0 or 1
#endif

// R412 comparison: compile out only the scene-selection hook installation.
#ifndef CONTROL_RT_NATIVE_SELECTION
#define CONTROL_RT_NATIVE_SELECTION 0
#endif
#if CONTROL_RT_NATIVE_SELECTION != 0 && CONTROL_RT_NATIVE_SELECTION != 1
#error CONTROL_RT_NATIVE_SELECTION must be 0 or 1
#endif
