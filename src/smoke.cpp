#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dxgi.h>
#include <cstdio>

int wmain(int argc, wchar_t** argv) {
    if (argc != 2) { puts("Expected the absolute path to the built proxy DLL."); return 1; }
    HMODULE proxy = LoadLibraryExW(argv[1], nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!proxy) { printf("Proxy load failed: %lu\n", GetLastError()); return 2; }
    using Create = HRESULT(WINAPI*)(REFIID, void**);
    auto create = reinterpret_cast<Create>(GetProcAddress(proxy, "CreateDXGIFactory1"));
    if (!create) { puts("Missing CreateDXGIFactory1 export."); FreeLibrary(proxy); return 3; }
    IDXGIFactory1* factory = nullptr;
    HRESULT hr = create(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&factory));
    if (FAILED(hr) || !factory) {
        printf("DXGI forwarding failed: 0x%08lX\n", static_cast<unsigned long>(hr));
        FreeLibrary(proxy); return 4;
    }
    IDXGIAdapter1* adapter = nullptr;
    hr = factory->EnumAdapters1(0, &adapter);
    if (SUCCEEDED(hr) && adapter) {
        DXGI_ADAPTER_DESC1 desc{};
        if (SUCCEEDED(adapter->GetDesc1(&desc))) wprintf(L"First DXGI adapter: %ls\n", desc.Description);
        adapter->Release();
    }
    factory->Release();
    FreeLibrary(proxy);
    puts("PASS: proxy loaded and forwarded CreateDXGIFactory1 to a working DXGI factory.");
    puts("This smoke test does not load Control or validate its hooks.");
    return 0;
}
