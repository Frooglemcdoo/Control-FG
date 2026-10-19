#pragma once
// R24 comparison: only 0 (baseline) or 1 post-temporal spatial pass.
// Read once from beside the proxy; a missing/invalid INI preserves baseline.
static unsigned RRReflectionSpatialTestPasses() {
 static const unsigned passes=[]() -> unsigned {
  auto path=ModulePath(selfModule);
  const auto slash=path.find_last_of(L"\\/");
  if(slash==std::wstring::npos)return 0;
  path.resize(slash+1);path+=L"Control-RR-Reflection-Test.ini";
  const unsigned requested=GetPrivateProfileIntW(L"ReflectionTest",L"SpatialPasses",0,path.c_str());
  const unsigned selected=requested==1?1:0;
  Log("RR_REFLECTION_SPATIAL_R24 passes=%u requested=%u temporal_history=disabled restart_required=1",selected,requested);
  return selected;
 }();
 return passes;
}
