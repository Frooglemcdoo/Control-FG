#*********************************************************
#
# Copyright (c) Microsoft. All rights reserved.
# This code is licensed under the MIT License (MIT).
# THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
# ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
# IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
# PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
#
#*********************************************************
#
# Resource Views Sample (Python)
#
# Demonstrates inspecting D3D12 resources accessed by a draw call in a PIX GPU
# capture:
#   1. Create a PIX factory.
#   2. Open a GPU capture document.
#   3. Get analysis, connect, and start analysis.
#   4. Find a draw call event on a graphics queue.
#   5. Gather accessed resources for that event.
#   6. Print resource name, dimensions, format, and type.
#   7. Print associated resource view information.
#   8. Stop analysis and clean up.
#
# Usage: python main.py <path-to-gpu-capture-file>
#
# Requirements:
#   - 64-bit Python 3.9+
#   - pythonnet >= 3.0  (pip install pythonnet)
#   - .NET 10 runtime
#   - PIX installed (Preview or Retail), or PIX_DIR environment variable set.
#

import inspect
import os
import sys


# Shared PIX-install discovery + pythonnet bootstrap. Sample lives at
# api/{preview,retail}/<sample>/python/main.py, so api/_pix_bootstrap.py
# is three directories up.
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "..")))
from _pix_bootstrap import find_pix_bin_directory, initialize_pythonnet

E_PIX_DEVELOPER_MODE_NOT_ENABLED = -1967390720
E_PIX_FEATURE_REQUIRES_DEVELOPER_MODE = -1967390719


def get_string_or_default(value, default_value="(none)"):
    resolved_value = str(value)
    return resolved_value if resolved_value else default_value


def unpack_result(value):
    return value if isinstance(value, tuple) else (value,)


def get_first_result(value):
    return unpack_result(value)[0]


def get_last_result(value):
    return unpack_result(value)[-1]


def print_developer_mode_help_if_needed(hresult):
    if hresult in (
            E_PIX_DEVELOPER_MODE_NOT_ENABLED,
            E_PIX_FEATURE_REQUIRES_DEVELOPER_MODE):
        print("ERROR: Windows Developer Mode is required for this operation.", file=sys.stderr)
        print("Enable it in Settings > Privacy & security > For developers,", file=sys.stderr)
        print(
            'or run: reg add "HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppModelUnlock" '
            '/v AllowDevelopmentWithoutDevLicense /t REG_DWORD /d 1 /f',
            file=sys.stderr)


def is_draw_call_event(event_info):
    event_name = get_string_or_default(event_info.Name, "")
    api_call_data = get_string_or_default(event_info.ApiCallData, "")
    return "draw" in event_name.lower() or "draw" in api_call_data.lower()


def find_first_draw_call(queues):
    import Microsoft.PIX as pix
    import Microsoft.PIX.Extension as pix_extension
    import Microsoft.PIX.Extension.GpuCapture as pix_gpu_capture

    for queue_index in range(queues.GetCount()):
        queue_info = pix_extension.PixApiExtensions.Get[pix.IPixGpuCaptureQueueInfo](queues, queue_index)
        if queue_info.GetType() != pix.PIX_QUEUE_TYPE.PIX_QUEUE_TYPE_GRAPHICS:
            continue

        for event_index in range(queue_info.GetEventCount()):
            event_info = pix_gpu_capture.PixApiExtensionsGpuCapture.GetEvent(queue_info, event_index)
            if not is_draw_call_event(event_info):
                continue

            return {
                "QueueIndex": queue_index,
                "EventIndex": event_index,
                "QueueName": get_string_or_default(queue_info.GetName(), "(unnamed queue)"),
                "AdapterName": get_string_or_default(queue_info.GetAdapterName(), "(unknown adapter)"),
                "EventInfo": event_info,
            }

    raise RuntimeError("No draw call event was found in the capture.")


def get_generic_pipeline(program_state):
    import clr
    import Microsoft.PIX as pix

    generic_pipeline_guid = clr.GetClrType(pix.IPixGenericPipeline).GUID
    generic_pipeline_result = pix._IPixProgramState_Extensions.GetGpuProgram(program_state, generic_pipeline_guid)
    return get_last_result(generic_pipeline_result)


def get_resource_from_view(resource_view):
    import Microsoft.PIX.Extension.GpuCapture.Resources as pix_gpu_capture_resources

    # The typed PixApiExtensionsGpuCaptureResources.GetD3D12Resource overload returns the
    # IPixD3D12Resource directly, avoiding the GUID/out-param dance.
    return pix_gpu_capture_resources.PixApiExtensionsGpuCaptureResources.GetD3D12Resource(resource_view)


def get_resource_views_for_resource(resource):
    import Microsoft.PIX.Extension.GpuCapture.Resources as pix_gpu_capture_resources

    return pix_gpu_capture_resources.PixApiExtensionsGpuCaptureResources.GetD3D12ResourceViews(resource)


def format_nullable(value, formatter):
    """Format a System.Nullable<T> result, falling back to a placeholder when null."""
    if value is None:
        return "(none)"
    if hasattr(value, "HasValue") and not value.HasValue:
        return "(none)"
    inner = value.Value if hasattr(value, "Value") else value
    return formatter(inner)


def describe_view(resource_views, view_index, view_type):
    import Microsoft.PIX as pix
    import Microsoft.PIX.Extension.GpuCapture.Resources as pix_gpu_capture_resources

    ext = pix_gpu_capture_resources.PixApiExtensionsGpuCaptureResources

    if view_type == pix.PIX_RESOURCE_VIEW_TYPE.PIX_RESOURCE_CONSTANT_BUFFER_VIEW:
        constant_buffer_view = ext.GetResourceView[pix.IPixConstantBufferView](resource_views, view_index)
        return format_nullable(
            ext.GetConstantBufferViewDesc(constant_buffer_view),
            lambda desc: (
                f"CBV SizeInBytes={desc.SizeInBytes}, "
                f"Offset={constant_buffer_view.GetBufferLocationOffset()}"))

    if view_type == pix.PIX_RESOURCE_VIEW_TYPE.PIX_RESOURCE_VERTEX_BUFFER_VIEW:
        vertex_buffer_view = ext.GetResourceView[pix.IPixVertexBufferView](resource_views, view_index)
        vertex_buffer_desc = ext.GetVertexBufferViewDesc(vertex_buffer_view)
        return (
            f"VBV StrideInBytes={vertex_buffer_desc.StrideInBytes}, "
            f"SizeInBytes={vertex_buffer_desc.SizeInBytes}, "
            f"Offset={vertex_buffer_view.GetBufferLocationOffset()}")

    if view_type == pix.PIX_RESOURCE_VIEW_TYPE.PIX_RESOURCE_INDEX_BUFFER_VIEW:
        index_buffer_view = ext.GetResourceView[pix.IPixIndexBufferView](resource_views, view_index)
        index_buffer_desc = ext.GetIndexBufferViewDesc(index_buffer_view)
        return (
            f"IBV Format={index_buffer_desc.Format}, "
            f"SizeInBytes={index_buffer_desc.SizeInBytes}, "
            f"Offset={index_buffer_view.GetBufferLocationOffset()}")

    if view_type == pix.PIX_RESOURCE_VIEW_TYPE.PIX_RESOURCE_STREAM_OUTPUT_VIEW:
        stream_output_view = ext.GetResourceView[pix.IPixStreamOutputView](resource_views, view_index)
        stream_output_desc = ext.GetStreamOutputViewDesc(stream_output_view)
        return (
            f"SOV SizeInBytes={stream_output_desc.SizeInBytes}, "
            f"Offset={stream_output_view.GetBufferLocationOffset()}")

    if view_type == pix.PIX_RESOURCE_VIEW_TYPE.PIX_RESOURCE_SHADER_RESOURCE_VIEW:
        shader_resource_view = ext.GetResourceView[pix.IPixShaderResourceView](resource_views, view_index)
        description = format_nullable(
            ext.GetShaderResourceViewDesc(shader_resource_view),
            lambda desc: f"SRV Format={desc.Format}, ViewDimension={desc.ViewDimension}")
        location_offset_nullable = ext.GetLocationOffset(shader_resource_view)
        if location_offset_nullable is not None and getattr(location_offset_nullable, "HasValue", True):
            offset_value = location_offset_nullable.Value if hasattr(location_offset_nullable, "Value") else location_offset_nullable
            description += f", LocationOffset={offset_value}"
        return description

    if view_type == pix.PIX_RESOURCE_VIEW_TYPE.PIX_RESOURCE_RENDER_TARGET_VIEW:
        render_target_view = ext.GetResourceView[pix.IPixRenderTargetView](resource_views, view_index)
        render_target_desc = ext.GetRenderTargetViewDesc(render_target_view)
        return (
            f"RTV Format={render_target_desc.Format}, "
            f"ViewDimension={render_target_desc.ViewDimension}")

    if view_type == pix.PIX_RESOURCE_VIEW_TYPE.PIX_RESOURCE_UNORDERED_ACCESS_VIEW:
        unordered_access_view = ext.GetResourceView[pix.IPixUnorderedAccessView](resource_views, view_index)
        return format_nullable(
            ext.GetUnorderedAccessViewDesc(unordered_access_view),
            lambda desc: f"UAV Format={desc.Format}, ViewDimension={desc.ViewDimension}")

    if view_type == pix.PIX_RESOURCE_VIEW_TYPE.PIX_RESOURCE_DEPTH_STENCIL_VIEW:
        depth_stencil_view = ext.GetResourceView[pix.IPixDepthStencilView](resource_views, view_index)
        depth_stencil_desc = ext.GetDepthStencilViewDesc(depth_stencil_view)
        return (
            f"DSV Format={depth_stencil_desc.Format}, "
            f"ViewDimension={depth_stencil_desc.ViewDimension}")

    if view_type == pix.PIX_RESOURCE_VIEW_TYPE.PIX_RESOURCE_BUFFER:
        buffer_resource_view = ext.GetResourceView[pix.IPixBufferResourceView](resource_views, view_index)
        return (
            f"Buffer SizeInBytes={buffer_resource_view.GetSizeInBytes()}, "
            f"Offset={buffer_resource_view.GetBufferLocationOffset()}")

    if view_type == pix.PIX_RESOURCE_VIEW_TYPE.PIX_RESOURCE_TEXTURE:
        texture_resource_view = ext.GetResourceView[pix.IPixTextureResourceView](resource_views, view_index)
        subresource_range = ext.GetSubresourceRange(texture_resource_view)
        return (
            f"Texture FirstMip={subresource_range.IndexOrFirstMipLevel}, "
            f"NumMips={subresource_range.NumMipLevels}, "
            f"FirstArraySlice={subresource_range.FirstArraySlice}, "
            f"NumArraySlices={subresource_range.NumArraySlices}")

    if view_type == pix.PIX_RESOURCE_VIEW_TYPE.PIX_RESOURCE_SHADING_RATE_IMAGE:
        return "ShadingRateImage"

    return str(view_type)


def print_accessed_resources(resource_views_at_event):
    import Microsoft.PIX as pix
    import Microsoft.PIX.Extension.GpuCapture.Resources as pix_gpu_capture_resources

    ext = pix_gpu_capture_resources.PixApiExtensionsGpuCaptureResources
    resources_by_pointer = {}
    resource_order = []

    print(f"Accessed resource views at the selected draw: {resource_views_at_event.GetCount()}")
    for view_index in range(resource_views_at_event.GetCount()):
        resource_view = ext.GetResourceView[pix.IPixResourceView](resource_views_at_event, view_index)
        view_type = resource_view.GetType()
        view_description = describe_view(resource_views_at_event, view_index, view_type)

        if view_type in (
                pix.PIX_RESOURCE_VIEW_TYPE.PIX_RESOURCE_SAMPLER,
                pix.PIX_RESOURCE_VIEW_TYPE.PIX_RESOURCE_STATIC_SAMPLER,
                pix.PIX_RESOURCE_VIEW_TYPE.PIX_RESOURCE_DESCRIPTOR_RANGE,
                pix.PIX_RESOURCE_VIEW_TYPE.PIX_RESOURCE_ROOT_CONSTANT,
                pix.PIX_RESOURCE_VIEW_TYPE.PIX_RESOURCE_GPU_DESCRIPTOR_HANDLE,
                pix.PIX_RESOURCE_VIEW_TYPE.PIX_RESOURCE_NONE):
            continue

        typed_view = ext.GetResourceView[pix.IPixD3D12ResourceView](resource_views_at_event, view_index)
        resource = get_resource_from_view(typed_view)
        # Group views by the resource's PIX api object id; matches the C# and
        # C++ sample behavior. id(resource) cannot be used because pythonnet
        # may produce a fresh __ComObject wrapper for each access path even
        # when the underlying IUnknown is the same resource.
        resource_key = int(resource.GetApiObjectId())
        if resource_key not in resources_by_pointer:
            resources_by_pointer[resource_key] = {
                "Resource": resource,
                "EventViewDescriptions": [],
            }
            resource_order.append(resource_key)
        resources_by_pointer[resource_key]["EventViewDescriptions"].append(view_description)

    print(f"Unique accessed D3D12 resources: {len(resource_order)}")
    for resource_index, resource_key in enumerate(resource_order):
        resource_info = resources_by_pointer[resource_key]
        resource = resource_info["Resource"]
        resource_desc = ext.GetDesc(resource)

        print()
        print(f"Resource [{resource_index}] 0x{resource_key:X}")
        print(f"    Name: {get_string_or_default(resource.GetName(), '(unnamed)')}")
        print(f"    Type: {get_resource_type_name(resource.GetType())}")
        print(f"    Dimension: {resource_desc.Dimension}")
        print(
            f"    Size: {resource_desc.Width} x {resource_desc.Height} x "
            f"{resource_desc.DepthOrArraySize}")
        print(f"    MipLevels: {resource_desc.MipLevels}")
        print(f"    Format: {resource_desc.Format}")

        if resource_info["EventViewDescriptions"]:
            print("    EventViews:")
            for view_description in resource_info["EventViewDescriptions"]:
                print(f"      - {view_description}")

        associated_views = get_resource_views_for_resource(resource)
        print(f"    AssociatedViews: {associated_views.GetCount()}")
        for associated_view_index in range(associated_views.GetCount()):
            associated_view = ext.GetResourceView[pix.IPixResourceView](associated_views, associated_view_index)
            associated_view_description = describe_view(
                associated_views,
                associated_view_index,
                associated_view.GetType())
            print(f"      - {associated_view_description}")


def get_resource_type_name(resource_type):
    import Microsoft.PIX as pix

    if resource_type == pix.PIX_D3D12_RESOURCE_TYPE.PIX_D3D12_RESOURCE_COMMITTED:
        return "Committed"
    if resource_type == pix.PIX_D3D12_RESOURCE_TYPE.PIX_D3D12_RESOURCE_PLACED:
        return "Placed"
    if resource_type == pix.PIX_D3D12_RESOURCE_TYPE.PIX_D3D12_RESOURCE_RESERVED:
        return "Reserved"
    return "Unknown"


