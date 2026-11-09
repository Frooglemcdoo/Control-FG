StructuredBuffer<uint4> input : register(t0);
float4 VS(uint id : SV_VertexID) : SV_Position {
 return float4(id==2?3:-1,id==1?3:-1,0,1);
}
uint4 PS() : SV_Target {return input[0];}
