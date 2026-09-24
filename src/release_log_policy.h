#pragma once
#include <cstddef>
#include <cstring>

namespace control_fg_release_log {
inline bool ShouldSuppress(const char* body, bool verboseAuditLogging) noexcept {
    if (verboseAuditLogging || !body || !*body) return false;
    const char* end = std::strchr(body, ' ');
    const std::size_t markerLength = end ? static_cast<std::size_t>(end - body) : std::strlen(body);
    static const char* const suppressed[] = {
        "CAMERA_VALUES","CAMERA_ARRAY","CAMERA_SNAPSHOT",
        "SWAPCHAIN_FRAME","ENGINE_HDR_STATE","SWAPCHAIN","SWAPCHAIN_BUFFER","SWAPCHAIN_BUFFER_ALL","PRESENT_PATH",
        "HUD_RTT_TARGET","HUD_RTT_FRAME_SUMMARY","HUD_DISPATCH_ENTER","HUD_DISPATCH_RETURN","HUD_RENDER_ENTER","HUD_RENDER_RETURN",
        "HUD_COMMAND_CONTEXT","HUD_PRE_UI_CANDIDATE","BEGIN_ENTER","BEGIN_RETURN","PRESENT_ENTER","PRESENT_RETURN","COUNTERS","SWAP_TRACE_TRIGGER",
        "RESOURCE","RESOURCE_UNAVAILABLE","NGX_FLOAT","NGX_UINT","NGX_INT","NGX_RESOURCE","NGX_RESOURCE_EMPTY","NGX_POST_EVAL","NGX_FEATURE_FLAGS",
        "SL_FRAME_TOKEN","SL_SIMULATION_MARKER_START","SL_BACKBUFFER_INDEX","SL_DLSSG_STATE_SAMPLE","SL_PRESENT_MANUAL_GATE",
        "SL_CONSTANTS","SL_RESOURCE_TAG","SL_PRESENT_MARKER_START","SL_PRESENT_MARKER_END","SL_PRESENT_FRAME_GATE",
        "HDR10_BRIDGE_QI_DELEGATED","FG_RUNTIME_STATS","FG_UI_HUDLESS_SNAPSHOT","FG_UI_RECOMPOSE_TAG","FG_OVERLAY_CADENCE",
        "SL_RR_OPTIMAL_CANDIDATE","RR_G12_LIVE_STATUS","RR_G12_EVALUATION_ENTRY",
        "RR_NOISY_REFLECTION","RR_NOISY_GI","RR_NOISY_CONTACT_SHADOW","AA_ENTER","AA_RETURN","AA_POST_SAMPLE","AA_TRANSITION"
    };
    for (const char* marker : suppressed) {
        const std::size_t length = std::strlen(marker);
        if (length == markerLength && !std::memcmp(body, marker, length)) return true;
    }
    return false;
}
}
