cmake_minimum_required(VERSION 3.30)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
project(ControlGIDenoiser LANGUAGES CXX)
include(FetchContent)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# The CPU contract target deliberately has no shader binaries. It cannot be
# loaded by the game. The GPU target must enable embedded D3D12 shaders.
option(CONTROL_REBLUR_ONLY "Use REBLUR instead of RELAX" OFF)
set(CONTROL_NRD_SHADER_DEFINES)
if(CONTROL_REBLUR_ONLY)
 add_compile_definitions(CONTROL_REBLUR_ONLY)
 list(APPEND CONTROL_NRD_SHADER_DEFINES -D CONTROL_REBLUR_ONLY=1)
endif()
option(CONTROL_NRD_GPU "Build embedded D3D12 NRD shaders" OFF)
if(CONTROL_NRD_GPU AND NOT WIN32)
  message(FATAL_ERROR "D3D12 validation requires Windows")
endif()
set(NRD_STATIC_LIBRARY ON CACHE BOOL "" FORCE)
set(NRD_NRI OFF CACHE BOOL "" FORCE)
set(NRD_EMBEDS_SPIRV_SHADERS OFF CACHE BOOL "" FORCE)
set(NRD_EMBEDS_DXBC_SHADERS OFF CACHE BOOL "" FORCE)
set(NRD_EMBEDS_DXIL_SHADERS ${CONTROL_NRD_GPU} CACHE BOOL "" FORCE)
# Floating-point world normals avoid conflating Control's material IDs with
# NRD's optional 2-bit material classification.
set(NRD_NORMAL_ENCODING 4 CACHE STRING "" FORCE)
set(NRD_ROUGHNESS_ENCODING 1 CACHE STRING "" FORCE)
FetchContent_Declare(nrd
 GIT_REPOSITORY https://github.com/NVIDIA-RTX/NRD.git
 GIT_TAG c862a54f4ed845378eeebd832491eebdc203c537
 GIT_SHALLOW FALSE
 PATCH_COMMAND ${CMAKE_COMMAND} -DNRD_SOURCE=<SOURCE_DIR>
   -P ${CMAKE_CURRENT_SOURCE_DIR}/patch_nrd.cmake)
FetchContent_MakeAvailable(nrd)
FetchContent_GetProperties(mathlib SOURCE_DIR MATHLIB_SOURCE)
include(${CMAKE_CURRENT_SOURCE_DIR}/patch_mathlib.cmake)
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/nrd_library_$<CONFIG>.txt" CONTENT "$<TARGET_FILE:NRD>")
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/nrd_shader_blob_$<CONFIG>.txt" CONTENT "$<TARGET_FILE:ShaderMakeBlob>")
add_executable(control_nrd_contract test_contract.cpp)
target_link_libraries(control_nrd_contract PRIVATE NRD)
if(MSVC)
 target_compile_options(control_nrd_contract PRIVATE /W4 /WX)
else()
 target_compile_options(control_nrd_contract PRIVATE -Wall -Wextra -Werror)
endif()
enable_testing()
add_test(NAME control_nrd_contract COMMAND control_nrd_contract)
if(CONTROL_NRD_GPU)
 add_library(control_nrd_native_inputs OBJECT compile_native_inputs.cpp)
 target_compile_options(control_nrd_native_inputs PRIVATE /W4)
 add_executable(control_nrd_d3d12 test_d3d12.cpp)
 target_link_libraries(control_nrd_d3d12 PRIVATE NRD d3d12 dxgi)
 target_compile_options(control_nrd_d3d12 PRIVATE /W4)
 add_test(NAME control_nrd_d3d12 COMMAND control_nrd_d3d12)
 set(conversion_headers)
 foreach(shader prepare finish receiver)
  set(header "${CMAKE_CURRENT_BINARY_DIR}/nrd_${shader}.h")
  add_custom_command(OUTPUT "${header}"
   COMMAND "${SHADERMAKE_DXC_PATH}" -T cs_6_0 -HV 2018 -E main -WX ${CONTROL_NRD_SHADER_DEFINES}
    -I "${nrd_SOURCE_DIR}/Shaders"
    -Fh "${header}" -Vn "kNrd_${shader}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../src/shaders/nrd_diffuse_${shader}.hlsl"
   DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../../src/shaders/nrd_diffuse_${shader}.hlsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../src/shaders/gi_evaluator_secondary.generated.hlsl"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../src/shaders/ControlPrimaryGuideDecode.hlsli"
   VERBATIM)
  list(APPEND conversion_headers "${header}")
 endforeach()
 if(CONTROL_REBLUR_ONLY)
  set(specular_header "${CMAKE_CURRENT_BINARY_DIR}/nrd_specular_prepare.h")
  add_custom_command(OUTPUT "${specular_header}"
   COMMAND "${SHADERMAKE_DXC_PATH}" -T cs_6_0 -HV 2018 -E main -WX
    ${CONTROL_NRD_SHADER_DEFINES} -D CONTROL_NRD_SPECULAR=1
    -I "${nrd_SOURCE_DIR}/Shaders"
    -Fh "${specular_header}" -Vn kNrd_specular_prepare
    "${CMAKE_CURRENT_SOURCE_DIR}/../../src/shaders/nrd_diffuse_prepare.hlsl"
   DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../../src/shaders/nrd_diffuse_prepare.hlsl"
   VERBATIM)
  list(APPEND conversion_headers "${specular_header}")
  foreach(spec_shader guides native_prepare)
   set(spec_output "${CMAKE_CURRENT_BINARY_DIR}/nrd_specular_${spec_shader}.h")
   set(spec_source "${CMAKE_CURRENT_SOURCE_DIR}/../../src/shaders/nrd_specular_guides.hlsl")
   set(spec_defines)
   if(spec_shader STREQUAL "native_prepare")
    set(spec_source "${CMAKE_CURRENT_SOURCE_DIR}/../../src/shaders/nrd_diffuse_prepare.hlsl")
    list(APPEND spec_defines -D CONTROL_NRD_SPECULAR=1 -D CONTROL_NRD_HIT_NORMALIZED=1)
   else()
    set(spec_output "${CMAKE_CURRENT_BINARY_DIR}/nrd_specular_guides_shader.h")
   endif()
   add_custom_command(OUTPUT "${spec_output}"
    COMMAND "${SHADERMAKE_DXC_PATH}" -T cs_6_0 -HV 2018 -E main -WX
     ${CONTROL_NRD_SHADER_DEFINES} ${spec_defines} -I "${nrd_SOURCE_DIR}/Shaders"
     -Fh "${spec_output}" -Vn "kNrd_specular_${spec_shader}" "${spec_source}"
    DEPENDS "${spec_source}" "${CMAKE_CURRENT_SOURCE_DIR}/../../src/shaders/ControlPrimaryGuideDecode.hlsli"
     "${CMAKE_CURRENT_SOURCE_DIR}/../../src/shaders/rr_hit_distance_shared.h"
    VERBATIM)
   list(APPEND conversion_headers "${spec_output}")
  endforeach()
 endif()
 add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/nrd_restore_probe.h"
  COMMAND "${SHADERMAKE_DXC_PATH}" -T cs_6_0 -E main -WX
   -Fh "${CMAKE_CURRENT_BINARY_DIR}/nrd_restore_probe.h" -Vn kNrd_restore_probe
   "${CMAKE_CURRENT_SOURCE_DIR}/restore_probe.hlsl"
  DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/restore_probe.hlsl" VERBATIM)
 foreach(stage vs ps)
  string(TOUPPER "${stage}" entry)
  add_custom_command(OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/nrd_graphics_${stage}.h"
   COMMAND "${SHADERMAKE_DXC_PATH}" -T ${stage}_6_0 -E ${entry} -WX
    -Fh "${CMAKE_CURRENT_BINARY_DIR}/nrd_graphics_${stage}.h" -Vn kNrd_graphics_${stage}
    "${CMAKE_CURRENT_SOURCE_DIR}/graphics_restore_probe.hlsl"
   DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/graphics_restore_probe.hlsl" VERBATIM)
  list(APPEND conversion_headers "${CMAKE_CURRENT_BINARY_DIR}/nrd_graphics_${stage}.h")
 endforeach()
 add_executable(control_nrd_pipeline test_d3d12.cpp ${conversion_headers}
  "${CMAKE_CURRENT_BINARY_DIR}/nrd_restore_probe.h")
 target_compile_definitions(control_nrd_pipeline PRIVATE CONTROL_NRD_PIPELINE_TEST)
 target_include_directories(control_nrd_pipeline PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
 target_link_libraries(control_nrd_pipeline PRIVATE NRD d3d12 dxgi)
 target_compile_options(control_nrd_pipeline PRIVATE /W4)
 add_test(NAME control_nrd_pipeline COMMAND control_nrd_pipeline)
 if(CONTROL_REBLUR_ONLY)
  add_test(NAME control_nrd_specular COMMAND control_nrd_d3d12 --specular)
  add_test(NAME control_nrd_pipeline_specular COMMAND control_nrd_pipeline --specular)
 else()
  add_test(NAME control_nrd_pipeline_balanced COMMAND control_nrd_pipeline --balanced)
 endif()
endif()
