#ifndef GI_DISPATCH_GATE_HLSL
#define GI_DISPATCH_GATE_HLSL
// The live host initializes this once on the submission queue, outside the
// recorded game list. The recorded list clears it after its final resolve.
// A replay therefore leaves native lighting untouched without rewriting uploads.
#ifdef GI_OWNED_SINGLE_USE
ByteAddressBuffer giOwnedGate : register(t35);
bool GIOwnedGateOpen(){return giOwnedGate.Load(0)!=0;}
#else
bool GIOwnedGateOpen(){return true;}
#endif
#endif
