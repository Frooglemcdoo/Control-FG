#pragma once

#include <windows.h>

#include <cstdint>

// Ada (SM 8.9) temporal correction for NVIDIA's DLSS-G provider.
//
// The stock Ada frame-generation kernel scales motion by a hardcoded 0.5,
// which is correct for 2x (one generated frame at the midpoint) but collapses
// every generated frame onto the same temporal position at 3x and above. This
// rebuilds the provider's SM89 PTX so those 104 midpoint multiplies read the
// kernel's own temporal parameter t (and 1 - t) instead, then republishes the
// rebuilt fatbin through a cloned kernel descriptor.
//
// Every step is fail-closed: the adapter must be Ada, the provider version
// must be known, and the source fatbin, source PTX and rebuilt fatbin must all
// match their expected SHA-256 digests before anything is published.
namespace ada_patch
{
using LogCallback = void (*)(const wchar_t* message);

void SetLogCallback(LogCallback callback) noexcept;

// Verifies the adapter behind a D3D12 device is Ada (SM 8.9) via CUDA.
bool ObserveD3D12Device(void* device) noexcept;

// Same check for Vulkan. A VkCommandBuffer carries no back-pointer to its
// device, so unlike D3D12 the adapter cannot be recovered at CreateFeature
// time -- it has to be captured earlier, from slSetVulkanInfo.
bool ObserveVulkanPhysicalDevice(void* physicalDevice) noexcept;

// Rebuilds and publishes the corrected Ada kernel. Requires ObserveD3D12Device
// to have succeeded first. Idempotent: repeat calls re-validate the published
// descriptor rather than patching again.
bool PatchProvider(HMODULE module, const wchar_t* path) noexcept;

bool AdapterVerified() noexcept;
bool Ready() noexcept;
uint32_t FailureCode() noexcept;

// Human-readable name for a FailureCode(), for logs and bug reports.
const wchar_t* FailureName(uint32_t code) noexcept;
}
