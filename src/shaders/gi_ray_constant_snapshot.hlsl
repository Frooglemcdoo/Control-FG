// Pinned native GI variant-zero ray constants: 1128-byte b0, 20-byte b1.
// Read through their original CBV bindings, without a resource registry or CPU
// readback. Output slots start at 0 and 1280, both 256-byte CBV aligned.
cbuffer NativeFrame : register(b0) { uint4 frameWords[71]; };
cbuffer NativeGI : register(b1) { uint4 giWords[2]; };
RWByteAddressBuffer snapshot : register(u0);
[numthreads(64,1,1)]
void main(uint3 id : SV_DispatchThreadID) {
    uint word=id.x;
    if(word>=384) return;
    uint value=0;
    if(word<282) value=frameWords[word/4][word%4];
    if(word>=320&&word<325) value=giWords[(word-320)/4][(word-320)%4];
    snapshot.Store(word*4,value);
}
