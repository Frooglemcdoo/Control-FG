#include "../src/native_lighting_isolation.h"
#include <fstream>
#include <iterator>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 4) { std::cerr << "usage: check direct|gi native.dxbc output.dxbc\n"; return 2; }
    if (std::strcmp(argv[1], "direct") && std::strcmp(argv[1], "gi")) return 2;
    const auto& patch = !std::strcmp(argv[1], "direct") ? native_lighting::kDirect : native_lighting::kBakedGi;
    std::ifstream input(argv[2], std::ios::binary);
    if (!input) return 3;
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(input)), {}), result;
    if (!native_lighting::MakeDisabledContribution(patch, bytes.data(), bytes.size(), patch.nativeSha, result)) return 4;
    std::ofstream output(argv[3], std::ios::binary);
    output.write(reinterpret_cast<const char*>(result.data()), static_cast<std::streamsize>(result.size()));
    if (!output) return 5;
    // Unknown identities, truncated bytecode, and changed target instructions
    // must all fail closed and discard any previous patch result.
    if (native_lighting::MakeDisabledContribution(patch, bytes.data(), bytes.size(), "unknown", result) || !result.empty()) return 6;
    if (native_lighting::MakeDisabledContribution(patch, bytes.data(), bytes.size()-1, patch.nativeSha, result) || !result.empty()) return 7;
    bytes[patch.instructionOffset] ^= 1;
    if (native_lighting::MakeDisabledContribution(patch, bytes.data(), bytes.size(), patch.nativeSha, result) || !result.empty()) return 8;
    std::cout << "PASS: exact instruction guard and rejection contracts\n";
}
