#include <cassert>
#include <cstdio>
#include "../../src/rr_user_control.h"

int main() {
    using namespace control_rr;
    assert(RRUserPresetValue() == RRPresetF);
    assert(RRUserPresetLiveSwitchable(RRPresetF, RRPresetE));
    assert(RRUserPresetLiveSwitchable(RRPresetE, RRPresetF));
    assert(!RRUserPresetLiveSwitchable(RRPresetF, RRPresetK));
    const auto g = RRUserPresetSelectionGeneration();
    RRUserSetPresetValue(RRPresetE);
    assert(RRUserPresetValue() == RRPresetE);
    assert(RRUserPresetSelectionGeneration() == g + 1);
    RRUserSetPresetValue(RRPresetF);
    assert(RRUserPresetValue() == RRPresetF);
    assert(RRUserPresetSelectionGeneration() == g + 2);
    RRUserSetPresetValue(RRPresetF);
    assert(RRUserPresetSelectionGeneration() == g + 2);
    std::puts("PASS r21z preset policy: F default, E/F live switch, repeated selection stable");
}
