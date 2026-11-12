#include "../src/character_support_log.h"
#include "../src/release_log_policy.h"
#include <cassert>

int main() {
    const char* const admitted[] = {
        "RR_CHARACTER_SUPPORT_R418 build=R418",
        "RR_CHARACTER_BASE_ALBEDO_R417 original=%p",
        "RR_HAIR_OPACITY_READY original=%p",
        "PROBE v2.1.2 internal_build=pt-reflections-p0",
        "GI_FROZEN_R428 producer=1",
        "GI_FROZEN_R426 startup=1 requested=1",
        "GI_FROZEN_R426 pipeline_prepared=1",
        "GI_FROZEN_R426 compute_prepared=0 hr=0x80004005",
        "GI_FROZEN_R426 binding_ready=1",
        "GI_TOGGLE_R308 create_failed=1",
        "GI_TOGGLE_R308 ready=1"
    };
    for (const auto* event : admitted) {
        assert(control_character_support::IsLifecycleEvent(event));
        assert(!control_fg_release_log::ShouldSuppress(event, false));
    }
    const char* const denied[] = {
        nullptr, "", "PROBE_EXTRA", "GI_FROZEN_R428_FRAME", "GI_FROZEN_R426_FRAME", "GI_TOGGLE_R3080", "GI_RAY_TRACE", "RR_CHARACTER_GUIDE frame=%llu", "RR_HAIR_RESPONSIVITY frame=%llu",
        "RR_CHARACTER_BASE_ALBEDO_R417_EXTRA", "RR_HAIR_OPACITY_READY_FRAME",
        "RR_CHARACTER_SUPPORT_R4180", "RR_CHARACTER_BASE", "CAMERA_VALUES", "NGX_RESOURCE"
    };
    for (const auto* event : denied) assert(!control_character_support::IsLifecycleEvent(event));
    assert(control_character_support::IsLifecycleEvent("RR_HAIR_OPACITY_READY"));
}
