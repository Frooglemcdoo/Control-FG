# Pinned NRD's no-material branch returns scalar true even for vector callers.
# DXC rejects float3(true)/float4(true). Preserve the argument shape; material
# IDs are finite decoded values. This does not enable material rejection.
set(path "${NRD_SOURCE}/Shaders/Common.hlsli")
file(READ "${path}" source)
set(before "#define CompareMaterials( m0, m, minm )     true")
set(after "#define CompareMaterials( m0, m, minm )     ( ( m0 ) == ( m0 ) )")
string(FIND "${source}" "${before}" found)
if(found EQUAL -1)
 string(FIND "${source}" "${after}" patched)
 if(patched EQUAL -1)
  message(FATAL_ERROR "Pinned NRD material macro changed; review compatibility patch")
 endif()
else()
 string(REPLACE "${before}" "${after}" source "${source}")
 file(WRITE "${path}" "${source}")
endif()
