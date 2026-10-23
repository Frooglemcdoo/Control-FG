// Private shader-table translation. The native table remains read-only.
ByteAddressBuffer nativeTable : register(t8);
RWByteAddressBuffer captureTable : register(u2);
RWByteAddressBuffer captureStatus : register(u3);
cbuffer TableLayout : register(b4) {
    uint recordCount, tableBytes, layoutVersion, reserved;
    uint4 nativeOpaque0, nativeOpaque1, nativeAlpha0, nativeAlpha1;
    uint4 captureOpaque0, captureOpaque1, captureAlpha0, captureAlpha1;
}
[numthreads(1,1,1)]
void clearStatus() { captureStatus.Store4(0, uint4(0,0,0,0)); }
[numthreads(64,1,1)]
void translateTable(uint3 thread : SV_DispatchThreadID) {
    uint sourceBytes, destinationBytes, statusBytes;
    nativeTable.GetDimensions(sourceBytes);
    captureTable.GetDimensions(destinationBytes);
    captureStatus.GetDimensions(statusBytes);
    if (statusBytes < 16) return;
    bool valid = recordCount > 0 && recordCount <= 8388608 / 96 &&
        tableBytes == recordCount * 96 && sourceBytes == tableBytes &&
        destinationBytes >= tableBytes && layoutVersion == 1 && reserved == 0;
    valid = valid && any(nativeOpaque0 | nativeOpaque1) && any(nativeAlpha0 | nativeAlpha1) &&
        any(captureOpaque0 | captureOpaque1) && any(captureAlpha0 | captureAlpha1) &&
        (any(nativeOpaque0 != nativeAlpha0) || any(nativeOpaque1 != nativeAlpha1));
    if (!valid) { if (thread.x == 0) captureStatus.Store(12, 1); return; }
    if (thread.x >= recordCount) return;
    uint at = thread.x * 96;
    uint4 first = nativeTable.Load4(at), second = nativeTable.Load4(at + 16);
    bool empty = !any(first | second);
    bool opaque = all(first == nativeOpaque0) && all(second == nativeOpaque1);
    bool alpha = all(first == nativeAlpha0) && all(second == nativeAlpha1);
    uint ignored;
    if (empty) captureStatus.InterlockedAdd(8, 1, ignored);
    else if (opaque || alpha) captureStatus.InterlockedAdd(4, 1, ignored);
    else captureStatus.InterlockedAdd(0, 1, ignored);
    // Unknown records become null, and status prevents any ray traversal.
    captureTable.Store4(at, opaque ? captureOpaque0 : alpha ? captureAlpha0 : uint4(0,0,0,0));
    captureTable.Store4(at + 16, opaque ? captureOpaque1 : alpha ? captureAlpha1 : uint4(0,0,0,0));
    [unroll] for (uint offset = 32; offset < 96; offset += 16)
        captureTable.Store4(at + offset, nativeTable.Load4(at + offset));
}
