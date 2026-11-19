#include "../src/gi_reuse/scene_capture_grid.h"
#include <cassert>
#include <algorithm>
#include <iostream>
int main(){for(unsigned extent: {1u,19u,511u,512u,513u,1920u,2160u,2560u,3840u,4096u,16384u}){
 unsigned count=std::min(extent,512u),found=0;
 for(unsigned pixel=0;pixel<extent;++pixel){auto cell=gi_reuse::GISceneCell(pixel,extent,count);if(cell!=0xffffffffu){assert(cell==found++);assert(gi_reuse::GISceneCoordinate(cell,extent,count)==pixel);}}
 assert(found==count);
 }assert(gi_reuse::GISceneCell(1,0,0)==0xffffffffu);std::cout<<"PASS unique whole-screen grid mapping including odd extents.\n";}
