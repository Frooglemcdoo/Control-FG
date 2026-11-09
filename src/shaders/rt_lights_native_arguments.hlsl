// The first 88 bytes of arguments are an immutable copy of this submission's
// native shader tables with the custom raygen record. This shader changes only
// dispatch dimensions. Never patch a command buffer already submitted to a queue.
StructuredBuffer<uint> allocation : register(t0);
RWTexture2D<uint4> work : register(u0);
RWByteAddressBuffer arguments : register(u1);
cbuffer Limits : register(b0) {
    uint workWidth; uint workHeight; uint taskCapacity; uint reserved;
}
// Dispatch exactly one group after a UAV barrier on batch outputs/counter.
[numthreads(64,1,1)]
void main(uint3 thread : SV_GroupThreadID) {
    uint width,height,size,stride,argumentBytes;
    work.GetDimensions(width,height);allocation.GetDimensions(size,stride);
    arguments.GetDimensions(argumentBytes);
    if(argumentBytes<136) return;
    uint count=0;
    bool valid=width==workWidth && height==workHeight && width>0 && height>0 &&
        width%8==0 && height%8==0 && height<=0xffffffff/max(width,1) &&
        size==1 && stride==4;
    if(valid) {
        count=allocation[0];
        if(count>taskCapacity || count>width*height) count=0;
    }
    uint rows=count==0 ? 0 : (count-1)/max(width,1)+1;
    uint groupsY=(rows+7)/8;
    uint tailEnd=groupsY*8*width;
    // Extraction uses native 8x8 groups and can visit the last partial rows.
    // Disable that bounded tail so stale atlas jobs cannot generate lighting.
    for(uint index=count+thread.x;index<tailEnd;index+=64)
        work[uint2(index%width,index/width)]=0;
    if(thread.x==0) {
        arguments.Store3(88,uint3(count,1,1)); // D3D12_DISPATCH_RAYS_DESC
        arguments.Store(100,0);              // structure padding
        arguments.Store3(112,uint3(valid ? width/8 : 0,groupsY,1));
        arguments.Store3(124,uint3((count+63)/64,1,1));
    }
}
