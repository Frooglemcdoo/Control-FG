ByteAddressBuffer inputBytes : register(t0); RWByteAddressBuffer outputBytes : register(u0);
cbuffer Layout : register(b0) { uint srcOffset; uint dstOffset; uint rowWords; uint rows; uint pitchWords; };
[numthreads(64,1,1)] void main(uint3 id:SV_DispatchThreadID) {
 if(id.x<rowWords && id.y<rows) outputBytes.Store(dstOffset+4*(id.y*pitchWords+id.x),inputBytes.Load(srcOffset+4*(id.y*rowWords+id.x)));
}
