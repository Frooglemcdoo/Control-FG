cbuffer Parameters:register(b0){uint4 constants[16];}
RWStructuredBuffer<uint4> result:register(u0);
[numthreads(8,1,1)] void main(uint3 id:SV_DispatchThreadID){
 uint row=id.x;if(row>=17)return;
 uint4 value=constants[0];
 // Zero iterations, one ray, and every count through sixteen; conditional/nested paths.
 [loop]for(uint i=0;i<row;++i){
  value=(value*1664525u+constants[1])^constants[2];
  if((i+row)&1)value+=constants[3];
  [loop]for(uint j=0;j<row;++j)value^=constants[4]+j;
  value+=constants[(i&7)+8]; // Dynamic register indices must stay inside the loop.
 }
 result[row]=value+constants[0];
}
