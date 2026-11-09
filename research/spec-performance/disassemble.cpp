#include <windows.h>
#include <d3dcompiler.h>
#include <fstream>
#include <iterator>
#include <vector>
int main(int argc, char** argv) {
    if (argc != 3) return 1;
    std::ifstream input(argv[1], std::ios::binary);
    std::vector<char> bytes((std::istreambuf_iterator<char>(input)), {});
    if (bytes.empty()) return 2;
    ID3DBlob* assembly = nullptr;
    HRESULT result = D3DDisassemble(bytes.data(), bytes.size(), 0, nullptr, &assembly);
    if (FAILED(result)) return 3;
    std::ofstream output(argv[2], std::ios::binary);
    output.write(static_cast<const char*>(assembly->GetBufferPointer()), assembly->GetBufferSize());
    assembly->Release();
    return output.good() ? 0 : 4;
}
