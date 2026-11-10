#pragma once
// Compile two binaries from identical rendering code; default is clean.
#ifndef CONTROL_RT_DIAGNOSTICS
#define CONTROL_RT_DIAGNOSTICS 0
#endif
#if CONTROL_RT_DIAGNOSTICS != 0 && CONTROL_RT_DIAGNOSTICS != 1
#error CONTROL_RT_DIAGNOSTICS must be 0 or 1
#endif

// R412 user result: native is the baseline; owned selection is opt-in for comparison.
#ifndef CONTROL_RT_NATIVE_SELECTION
#define CONTROL_RT_NATIVE_SELECTION 1
#endif
#if CONTROL_RT_NATIVE_SELECTION != 0 && CONTROL_RT_NATIVE_SELECTION != 1
#error CONTROL_RT_NATIVE_SELECTION must be 0 or 1
#endif

// R413 isolates object preparation and instance assembly together.
#ifndef CONTROL_RT_NATIVE_PREPARATION
#define CONTROL_RT_NATIVE_PREPARATION 0
#endif
#if CONTROL_RT_NATIVE_PREPARATION != 0 && CONTROL_RT_NATIVE_PREPARATION != 1
#error CONTROL_RT_NATIVE_PREPARATION must be 0 or 1
#endif
