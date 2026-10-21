#include "../src/native_lighting_isolation.h"
#include <fstream>
#include <iterator>
#include <iostream>
#include <stdexcept>
static void Require(bool value) { if (!value) throw std::runtime_error("component patch validation failed"); }
int main(int argc, char** argv) try {
    using namespace native_lighting;
    if (argc < 2 || argc > 3) throw std::runtime_error("provide captured native direct-light DXBC");
    std::ifstream file(argv[1], std::ios::binary);
    std::vector<unsigned char> source((std::istreambuf_iterator<char>(file)), {}), output;
    Require(source.size() == kDirect.size);
    for (const auto* patch : {&kDirectDiffuse, &kDirectSpecular, &kShadowBypass, &kPatternOff, &kScreenShadows, &kSpotShadows, &kPointShadows, &kSpotVarianceLow, &kSpotVarianceHigh, &kSpotPcfCenter, &kSpotPcf32, &kSpotStableSoft, &kSpotStableTight}) {
        Require(MakeDisabledContribution(*patch,source.data(),source.size(),patch->nativeSha,output));
        const size_t insertedBytes = patch->insertedWordCount * 4;
        Require(output.size()==source.size()+insertedBytes);
        if (insertedBytes) {
            Require(std::memcmp(output.data()+patch->insertionOffset, patch->insertedWords, insertedBytes)==0);
            auto invalid=*patch; invalid.insertionOffset=source.size()+4;
            std::vector<unsigned char> rejected;
            Require(!MakeDisabledContribution(invalid,source.data(),source.size(),patch->nativeSha,rejected));
            if (argc == 3) {
                std::ofstream exported(argv[2],std::ios::binary);
                exported.write(reinterpret_cast<const char*>(output.data()), output.size());
                Require(bool(exported));
            }
        }
        for(size_t i=0;i<source.size();++i) {
            if(i>=4 && i<20) continue; // Separately validated container checksum.
            bool zero=false;
            for(size_t w=0;w<patch->zeroCount;++w) {
                const size_t offset=patch->instructionOffset+patch->zeroWords[w]*4;
                zero |= i>=offset && i<offset+4;
            }
            unsigned char expected = zero ? 0 : source[i];
            for (size_t e=0;e<patch->editCount;++e) {
                const auto& edit=patch->edits[e];
                if(i>=edit.offset && i<edit.offset+edit.words*4)
                    expected=reinterpret_cast<const unsigned char*>(edit.replacement)[i-edit.offset];
            }
            Require(output[i + (insertedBytes && i >= patch->insertionOffset ? insertedBytes : 0)] == expected);
        }
        if (patch->editCount) {
            auto wrong=source; wrong[patch->edits[0].offset]^=1;
            Require(!MakeDisabledContribution(*patch,wrong.data(),wrong.size(),patch->nativeSha,output));
        }
        Require(!MakeDisabledContribution(*patch,source.data(),source.size(),"unsupported",output));
        auto altered=source; altered[patch->instructionOffset]^=1;
        Require(!MakeDisabledContribution(*patch,altered.data(),altered.size(),patch->nativeSha,output));
    }
    std::cout<<"PASS: exact output/input patches change only their declared edits and checksum; invalid identities/instructions rejected\n";
    return 0;
} catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
