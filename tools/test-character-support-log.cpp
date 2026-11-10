#include "../src/character_support_log.h"
#include "../src/release_log_policy.h"
#include <cassert>

int main() {
    const char* const admitted[] = {
        "RR_CHARACTER_SUPPORT_R418 build=R418",
        "RR_CHARACTER_BASE_ALBEDO_R417 original=%p",
        "RR_HAIR_OPACITY_READY original=%p"
    };
    for (const auto* event : admitted) {
        assert(control_character_support::IsLifecycleEvent(event));
        assert(!control_fg_release_log::ShouldSuppress(event, false));
    }
    const char* const denied[] = {
        nullptr, "", "RR_CHARACTER_GUIDE frame=%llu", "RR_HAIR_RESPONSIVITY frame=%llu",
        "RR_CHARACTER_BASE_ALBEDO_R417_EXTRA", "RR_HAIR_OPACITY_READY_FRAME",
        "RR_CHARACTER_SUPPORT_R4180", "RR_CHARACTER_BASE", "CAMERA_VALUES", "NGX_RESOURCE"
    };
    for (const auto* event : denied) assert(!control_character_support::IsLifecycleEvent(event));
    assert(control_character_support::IsLifecycleEvent("RR_HAIR_OPACITY_READY"));
}
