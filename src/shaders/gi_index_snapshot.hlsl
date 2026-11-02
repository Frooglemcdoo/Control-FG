// SM 6.2 + native 16-bit operations are required and checked by the host.
// Do not replace the final Load<uint16_t> with a DWORD read beyond source bounds.
ByteAddressBuffer source : register(t0);
RWByteAddressBuffer destination : register(u0);
cbuffer Snapshot : register(b0) { uint elementCount; uint sourcePrefix; uint groupsX; };
[numthreads(64, 1, 1)]
void main(uint3 group : SV_GroupID, uint lane : SV_GroupIndex)
{
    uint word = (group.y * groupsX + group.x) * 64 + lane;
    uint first = word * 2;
    if (first >= elementCount) return;
    uint low = uint(source.Load<uint16_t>(sourcePrefix + first * 2));
    uint high = 0;
    if (first + 1 < elementCount)
        high = uint(source.Load<uint16_t>(sourcePrefix + (first + 1) * 2));
    destination.Store(word * 4, low | (high << 16));
}
