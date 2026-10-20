// Offline contract check against a captured shader. No GPU or game execution.
#include "../src/native_light_dither.h"
#include <fstream>
#include <iterator>
#include <iostream>
int main(int argc,char** argv) {
    if(argc!=3)return 2;
    std::ifstream file(argv[1],std::ios::binary);
    if(!file)return 3;
    std::vector<unsigned char> original((std::istreambuf_iterator<char>(file)),{}),patched;
    using namespace control_lighting;
    if(!MakeNoDitherShader(original.data(),original.size(),kDeferredShaderSha256,patched))return 4;
    for(size_t i=0;i<original.size();++i){
        const bool checksum=i>=4&&i<20;
        const bool amplitudes=i>=kDitherInstructionOffset+24&&i<kDitherInstructionOffset+32;
        if(!checksum&&!amplitudes&&original[i]!=patched[i])return 5;
    }
    std::ofstream out(argv[2],std::ios::binary);
    out.write(reinterpret_cast<const char*>(patched.data()),patched.size());
    if(!out)return 6;
    if(MakeNoDitherShader(original.data(),original.size(),"unsupported",patched)||!patched.empty())return 7;
    if(MakeNoDitherShader(original.data(),original.size()-1,kDeferredShaderSha256,patched))return 8;
    original[kDitherInstructionOffset]^=1;
    if(MakeNoDitherShader(original.data(),original.size(),kDeferredShaderSha256,patched))return 9;
    std::cout<<"PASS: patch scope, unsupported identity, truncation and instruction mismatch\n";
}
