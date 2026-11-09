// Build with the DXC SDK headers and dxcompiler library. DXV signs the result.
#ifdef _WIN32
#include <windows.h>
#endif
#include <dxc/dxcapi.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

template<class T> struct ReleaseCom {
    void operator()(T* value) const noexcept { if (value) value->Release(); }
};
template<class T> using ComOwner = std::unique_ptr<T, ReleaseCom<T>>;
static void Check(HRESULT status, const char* operation) {
    if (FAILED(status)) throw std::runtime_error(operation);
}
int main(int argc, char** argv) {
    if (argc != 3) { std::cerr << "Usage: assemble-dxil input.ll output.dxil\n"; return 2; }
    try {
        std::ifstream input(argv[1], std::ios::binary);
        if (!input) throw std::runtime_error("Cannot open input");
        std::vector<char> data((std::istreambuf_iterator<char>(input)), {});
        if (data.empty() || data.size() > std::numeric_limits<UINT32>::max())
            throw std::runtime_error("Invalid input size");
        IDxcLibrary* rawLibrary = nullptr;
        Check(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&rawLibrary)), "Create DXC library");
        ComOwner<IDxcLibrary> library(rawLibrary);
        IDxcBlobEncoding* rawSource = nullptr;
        Check(library->CreateBlobWithEncodingFromPinned(data.data(), static_cast<UINT32>(data.size()),
            CP_UTF8, &rawSource), "Create source blob");
        ComOwner<IDxcBlobEncoding> source(rawSource);
        IDxcAssembler* rawAssembler = nullptr;
        Check(DxcCreateInstance(CLSID_DxcAssembler, IID_PPV_ARGS(&rawAssembler)), "Create assembler");
        ComOwner<IDxcAssembler> assembler(rawAssembler);
        IDxcOperationResult* rawResult = nullptr;
        Check(assembler->AssembleToContainer(source.get(), &rawResult), "Assemble container");
        ComOwner<IDxcOperationResult> result(rawResult);
        IDxcBlobEncoding* rawErrors = nullptr;
        result->GetErrorBuffer(&rawErrors);
        ComOwner<IDxcBlobEncoding> errors(rawErrors);
        if (errors) std::cerr.write(static_cast<const char*>(errors->GetBufferPointer()),
            static_cast<std::streamsize>(errors->GetBufferSize()));
        HRESULT status = E_FAIL;
        Check(result->GetStatus(&status), "Read assembly status");
        Check(status, "Assembly failed");
        IDxcBlob* rawOutput = nullptr;
        Check(result->GetResult(&rawOutput), "Read assembled container");
        ComOwner<IDxcBlob> output(rawOutput);
        std::ofstream file(argv[2], std::ios::binary);
        file.write(static_cast<const char*>(output->GetBufferPointer()),
            static_cast<std::streamsize>(output->GetBufferSize()));
        if (!file) throw std::runtime_error("Cannot write output");
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n'; return 1;
    }
    return 0;
}
