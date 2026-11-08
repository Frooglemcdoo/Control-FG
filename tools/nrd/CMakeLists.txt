cmake_minimum_required(VERSION 3.30)
project(ControlGIDenoiser LANGUAGES CXX)
include(FetchContent)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# The CPU contract target deliberately has no shader binaries. It cannot be
# loaded by the game. The GPU target must enable embedded D3D12 shaders.
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
 add_executable(control_nrd_d3d12 test_d3d12.cpp)
 target_link_libraries(control_nrd_d3d12 PRIVATE NRD d3d12 dxgi)
 target_compile_options(control_nrd_d3d12 PRIVATE /W4)
 add_test(NAME control_nrd_d3d12 COMMAND control_nrd_d3d12)
endif()
