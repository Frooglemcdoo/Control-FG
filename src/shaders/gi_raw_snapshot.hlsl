// Isolated snapshot kernel. No native lighting filtering or history changes.
ByteAddressBuffer source : register(t0);
RWByteAddressBuffer destination : register(u0);
cbuffer Snapshot : register(b0) { uint wordCount; };
[numthreads(64, 1, 1)]
void main(uint3 thread : SV_DispatchThreadID)
{
    if (thread.x < wordCount)
        destination.Store(thread.x * 4, source.Load(thread.x * 4));
}
