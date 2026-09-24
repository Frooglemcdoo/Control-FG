#include <cassert>
#include <cstdio>
#include "../../src/release_log_policy.h"

int main() {
    using control_fg_release_log::ShouldSuppress;
    assert(ShouldSuppress("CAMERA_VALUES call=1", false));
    assert(ShouldSuppress("HUD_RTT_TARGET ordinal=1", false));
    assert(ShouldSuppress("SWAPCHAIN_FRAME present=1", false));
    assert(ShouldSuppress("NGX_RESOURCE call=1", false));
    assert(ShouldSuppress("RR_NOISY_REFLECTION frame=240", false));
    assert(!ShouldSuppress("RR_FRAME_MODE frame=240 mode=full", false));
    assert(!ShouldSuppress("RR_NATIVE_EVALUATED frame=240 success=1", false));
    assert(!ShouldSuppress("RR_PRESET_LIVE_SWITCH from=5 to=6", false));
    assert(!ShouldSuppress("RR_SPECULAR_SIGNAL frame=240 mode=native_control_energy_clamp", false));
    assert(!ShouldSuppress("FG_HDR_HUDLESS_PIPELINE_READY owner=1", false));
    assert(!ShouldSuppress("FG_HDR_HUDLESS_CONVERT target_present=240", false));
    assert(!ShouldSuppress("FG_UI_RECOMPOSITION_OPTIONS present=240 enabled=1 hudless_format=24 hdr10_bridge=1", false));
    assert(!ShouldSuppress("SL_DLSSG_MODE present=240 mode=on hudless_fmt=24 hdr10_bridge=1", false));
    assert(!ShouldSuppress("CAMERA_VALUES call=1", true));
    assert(!ShouldSuppress("HUD_RTT_TARGET ordinal=1", true));
    assert(!ShouldSuppress(nullptr, false));
    assert(!ShouldSuppress("", false));
    std::puts("PASS release log filter: audit noise suppressed by default; support events retained; verbose restores audit traces");
}
