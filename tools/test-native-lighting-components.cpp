#include "../src/native_lighting_isolation.h"
#include <fstream>
#include <iterator>
#include <iostream>
#include <stdexcept>
static void Require(bool value) { if (!value) throw std::runtime_error("component patch validation failed"); }
int main(int argc, char** argv) try {
    using namespace native_lighting;
    if (argc != 2) throw std::runtime_error("provide captured native direct-light DXBC");
    std::ifstream file(argv[1], std::ios::binary);
    std::vector<unsigned char> source((std::istreambuf_iterator<char>(file)), {}), output;
    Require(source.size() == kDirect.size);
    for (const auto* patch : {&kDirectDiffuse, &kDirectSpecular}) {
        Require(MakeDisabledContribution(*patch,source.data(),source.size(),patch->nativeSha,output));
        Require(output.size()==source.size());
        for(size_t i=0;i<source.size();++i) {
            if(i>=4 && i<20) continue; // Separately validated container checksum.
            bool zero=false;
            for(size_t w=0;w<patch->zeroCount;++w) {
                const size_t offset=patch->instructionOffset+patch->zeroWords[w]*4;
                zero |= i>=offset && i<offset+4;
            }
            Require(output[i] == (zero ? 0 : source[i]));
        }
        Require(!MakeDisabledContribution(*patch,source.data(),source.size(),"unsupported",output));
        auto altered=source; altered[patch->instructionOffset]^=1;
        Require(!MakeDisabledContribution(*patch,altered.data(),altered.size(),patch->nativeSha,output));
    }
    std::cout<<"PASS: component patches change only their two output multiplier words and checksum; invalid identities/instructions rejected\n";
    return 0;
} catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
