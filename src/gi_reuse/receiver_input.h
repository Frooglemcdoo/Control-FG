#ifndef CONTROL_GI_RECEIVER_INPUT_H
#define CONTROL_GI_RECEIVER_INPUT_H
#include "reuse_candidate.h"
#ifdef __cplusplus
namespace gi_reuse {
#endif
struct GIReceiverInput {
    GIConnection connection;
    GIInitialInput receiver;
    float sourceRange; float receiverRange; float sourceCosine; float receiverCosine;
    uint producerVerified; uint radianceVerified; uint sceneRevision; uint frameTag;
};
#ifdef __cplusplus
static_assert(sizeof(GIReceiverInput)==192);
}
#endif
#endif
