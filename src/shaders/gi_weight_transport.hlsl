#ifndef GI_WEIGHT_TRANSPORT
#define GI_WEIGHT_TRANSPORT
// Shared byte offset. Capture records retain their existing ABI; weights follow.
uint GIWeightBase(uint width,uint height,uint capacityPacked,uint rays) {
 uint bytes=(capacityPacked&65535u)*64u;
 if(capacityPacked&65536u){
  uint pixels=min(width,512u)*min(height,512u);
  bytes+=pixels*64u+((pixels*(rays+1u)*16u+31u)&~31u)+pixels*192u;
 }
 return bytes;
}
uint GIWeightAddress(uint2 pixel,uint ray,uint width,uint height,uint packed,uint rays){
 return GIWeightBase(width,height,packed,rays)+((pixel.y*width+pixel.x)*rays+ray)*4u;
}
#endif
