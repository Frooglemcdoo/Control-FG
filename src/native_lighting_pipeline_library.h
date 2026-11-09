#pragma once
// Included inside native_lighting after the ordinary PSO observers.
// Cached library loads return native PSOs without calling the public device
// creation hooks. Reuse the same exact-shader observer on those returned PSOs.
using LoadGraphics = HRESULT (STDMETHODCALLTYPE*)(ID3D12PipelineLibrary*, LPCWSTR, const D3D12_GRAPHICS_PIPELINE_STATE_DESC*, REFIID, void**);
using LoadCompute = HRESULT (STDMETHODCALLTYPE*)(ID3D12PipelineLibrary*, LPCWSTR, const D3D12_COMPUTE_PIPELINE_STATE_DESC*, REFIID, void**);
using CreateLibrary = HRESULT (STDMETHODCALLTYPE*)(ID3D12Device1*, const void*, SIZE_T, REFIID, void**);
struct LibraryHooks {
    void** table = nullptr;
    LoadGraphics graphics = nullptr;
    LoadCompute compute = nullptr;
};
inline LibraryHooks libraryHooks[8]{};
inline CreateLibrary originalCreateLibrary = nullptr;
inline std::atomic<unsigned long long> libraryGraphicsSeen{0}, libraryComputeSeen{0};
inline LibraryHooks FindLibraryHooks(ID3D12PipelineLibrary* library) noexcept {
    LibraryHooks result{};
    void** table = *reinterpret_cast<void***>(library);
    AcquireSRWLockShared(&hookLock);
    for (const auto& entry : libraryHooks) if (entry.table == table) { result = entry; break; }
    ReleaseSRWLockShared(&hookLock);
    return result;
}
inline HRESULT STDMETHODCALLTYPE LoadGraphicsHook(ID3D12PipelineLibrary* library, LPCWSTR name,
    const D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc, REFIID iid, void** out) {
    const auto original = FindLibraryHooks(library).graphics;
    if (!original) return E_UNEXPECTED;
    const HRESULT hr = original(library, name, desc, iid, out);
    libraryGraphicsSeen.fetch_add(1);
    if (SUCCEEDED(hr) && out && *out && desc && originalGraphics) {
        ID3D12Device* device = nullptr;
        if (SUCCEEDED(library->GetDevice(IID_PPV_ARGS(&device)))) {
            ObserveGraphics(device, desc, reinterpret_cast<IUnknown*>(*out));
            device->Release();
        }
    }
    return hr;
}
inline HRESULT STDMETHODCALLTYPE LoadComputeHook(ID3D12PipelineLibrary* library, LPCWSTR name,
    const D3D12_COMPUTE_PIPELINE_STATE_DESC* desc, REFIID iid, void** out) {
    const auto original = FindLibraryHooks(library).compute;
    if (!original) return E_UNEXPECTED;
    const HRESULT hr = original(library, name, desc, iid, out);
    libraryComputeSeen.fetch_add(1);
    if (SUCCEEDED(hr) && out && *out && desc) {
        ID3D12Device* device = nullptr;
        if (SUCCEEDED(library->GetDevice(IID_PPV_ARGS(&device)))) {
            // Load the native cache entry first. Replace only a successful exact
            // target load; failure keeps its valid native object and HRESULT.
            void* replacement=nullptr;
            HRESULT replacementHr=E_FAIL;
            if(spec_serial::TryCreate(device,desc,iid,&replacement,
                ptReflectionOriginalCreateComputePipelineState,true,replacementHr)){
                auto* native=reinterpret_cast<IUnknown*>(*out);
                *out=replacement;
                native->Release();
            }
            const auto create = reinterpret_cast<CreateCompute>((*reinterpret_cast<void***>(device))[11]);
            ObserveCompute(device, desc, reinterpret_cast<IUnknown*>(*out), create);
            device->Release();
        }
    }
    return hr;
}
inline void InstallLibrary(ID3D12PipelineLibrary* library) noexcept {
    void** table = *reinterpret_cast<void***>(library);
    AcquireSRWLockExclusive(&hookLock);
    LibraryHooks* available = nullptr;
    for (auto& entry : libraryHooks) {
        if (entry.table == table) { ReleaseSRWLockExclusive(&hookLock); return; }
        if (!entry.table && !available) available = &entry;
    }
    if (available) {
        *available = {table, reinterpret_cast<LoadGraphics>(table[9]), reinterpret_cast<LoadCompute>(table[10])};
        const bool graphics = PatchSlot(&table[9], reinterpret_cast<void*>(available->graphics), reinterpret_cast<void*>(&LoadGraphicsHook));
        const bool compute = PatchSlot(&table[10], reinterpret_cast<void*>(available->compute), reinterpret_cast<void*>(&LoadComputeHook));
        Trace("library_hooks graphics=%u compute=%u", unsigned(graphics), unsigned(compute));
    } else Trace("library_hooks failed=capacity");
    ReleaseSRWLockExclusive(&hookLock);
}
inline void ObserveLibrary(IUnknown* object) noexcept {
    ID3D12PipelineLibrary* library = nullptr;
    if (object && SUCCEEDED(object->QueryInterface(IID_PPV_ARGS(&library)))) {
        InstallLibrary(library); library->Release();
    }
}
inline HRESULT STDMETHODCALLTYPE CreateLibraryHook(ID3D12Device1* device, const void* blob, SIZE_T size, REFIID iid, void** out) {
    if (!originalCreateLibrary) return E_UNEXPECTED;
    const HRESULT hr = originalCreateLibrary(device, blob, size, iid, out);
    if (SUCCEEDED(hr) && out && *out) ObserveLibrary(reinterpret_cast<IUnknown*>(*out));
    return hr;
}
inline void InstallPipelineLibraries(ID3D12Device* device) noexcept {
    ID3D12Device1* extended = nullptr;
    if (FAILED(device->QueryInterface(IID_PPV_ARGS(&extended)))) { Trace("library_device unavailable=1"); return; }
    void** table = *reinterpret_cast<void***>(extended);
    bool first = false;
    AcquireSRWLockExclusive(&hookLock);
    if (!originalCreateLibrary) {
        originalCreateLibrary = reinterpret_cast<CreateLibrary>(table[44]);
        if (!PatchSlot(&table[44], reinterpret_cast<void*>(originalCreateLibrary), reinterpret_cast<void*>(&CreateLibraryHook)))
            originalCreateLibrary = nullptr;
        else first = true;
    }
    ReleaseSRWLockExclusive(&hookLock);
    if (first) {
        // Covers already-created libraries sharing this runtime's implementation.
        ID3D12PipelineLibrary* probe = nullptr;
        const HRESULT hr = originalCreateLibrary(extended, nullptr, 0, IID_PPV_ARGS(&probe));
        if (SUCCEEDED(hr) && probe) { InstallLibrary(probe); probe->Release(); }
        Trace("library_bootstrap hr=0x%08lX", static_cast<unsigned long>(hr));
    }
    extended->Release();
}
