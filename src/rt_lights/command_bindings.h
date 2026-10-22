#pragma once
#include "frame_recorder.h"
#include <atomic>
#include <new>

namespace rt_lights {
// Recording state belongs to the command list, not the thread that last used
// it. Allocate once at list creation; the list owns and releases this metadata.
class CommandBindings final : public IUnknown {
    std::atomic<ULONG> references{1};
public:
    inline static const GUID Key = {0x67cad101,0xc118,0x42e4,{0x91,0x30,0x04,0x20,0x26,0x10,0x22,0x03}};
    NativeBindings bindings;
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void** result) override {
        if (!result) return E_POINTER;
        *result = nullptr;
        if (iid != __uuidof(IUnknown)) return E_NOINTERFACE;
        *result = static_cast<IUnknown*>(this); AddRef(); return S_OK;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return ++references; }
    ULONG STDMETHODCALLTYPE Release() override {
        const ULONG remaining = --references;
        if (!remaining) delete this;
        return remaining;
    }
    static CommandBindings* Get(ID3D12GraphicsCommandList* list) noexcept {
        if (!list) return nullptr;
        IUnknown* object = nullptr; UINT bytes = sizeof(object);
        if (FAILED(list->GetPrivateData(Key,&bytes,&object)) || !object) return nullptr;
        auto* state = static_cast<CommandBindings*>(object);
        // GetPrivateData AddRefs interfaces. The live command list retains its
        // own reference throughout the intercepted recording call.
        object->Release();
        return state;
    }
    static bool Attach(ID3D12GraphicsCommandList* list) noexcept {
        if (Get(list)) return true;
        auto* state = new (std::nothrow) CommandBindings;
        if (!state) return false;
        const HRESULT hr = list->SetPrivateDataInterface(Key,state);
        state->Release();
        return SUCCEEDED(hr);
    }
};
}
