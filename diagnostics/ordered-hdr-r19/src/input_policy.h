#pragma once
#include <cstdint>
namespace hdrguard {
struct InputReadiness {
 bool ready{},foreground{},busy{},gpuKnown{},rtx40{},displayValid{};
 uint64_t frames{},frameAge{UINT64_MAX},displayAge{UINT64_MAX};
 uint32_t selection{};
};
inline const char* inputReason(const InputReadiness& r) {
 if(!r.ready)return "controller_not_ready";
 if(!r.foreground)return "game_not_foreground";
 if(r.busy)return "request_already_pending";
 if(!r.frames)return "render_callback_not_seen";
 if(r.frameAge>=500)return "render_heartbeat_stale";
 if(!r.gpuKnown)return "gpu_detection_pending";
 if(r.rtx40)return "rtx40_not_in_this_test";
 if(!r.selection)return "FG_selection_is_Off";
 if(!r.displayValid)return "HDR_output_query_failed";
 if(r.displayAge>=500)return "HDR_output_query_stale";
 return "accepted";
}
inline bool inputAllowed(const InputReadiness& r) {
 return r.ready && r.foreground && !r.busy && r.frames && r.frameAge<500 && r.gpuKnown && !r.rtx40 && r.selection && r.displayValid && r.displayAge<500;
}
inline bool isControllerMessage(uint32_t message,uintptr_t messageId,uint16_t registeredId) {
 return registeredId && message==0x0312u && messageId==registeredId;
}
}
