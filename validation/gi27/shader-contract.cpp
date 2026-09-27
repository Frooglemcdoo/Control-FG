#include <cassert>
#include <fstream>
#include <string>
#include <cstdio>
int main(){
 std::ifstream f("src/shaders/rr_specular_mv.hlsl");
 std::string s((std::istreambuf_iterator<char>(f)),{});
 for(const char* token:{"Texture2D<float2> GameMV","Texture2DArray<uint> MatId","Texture2DArray<float4> HitPos","RWTexture2D<float2> SpecMV","effective_t = hit_t * gloss * gloss","virtual_pos = surface_pos + view_dir * effective_t","NdcDelta(virtual_pos) - NdcDelta(surface_pos)","SpecMV[tid.xy] = game_mv + scaled"})
  assert(s.find(token)!=std::string::npos);
 assert(s.find("reflect(view_dir")==std::string::npos);
 std::puts("PASS GI27 shader source contract: reflected-image MV correction, game-MV inheritance, hit-array distance, roughness fade");
}
