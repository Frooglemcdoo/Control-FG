#include "../src/native_menu/bundle.h"
#include <fstream>
#include <iostream>
#include <iterator>
int main(int argc,char** argv) {
    std::string result;
    if(control_native_menu::PatchCommons("unsupported", "{}", result))return 1;
    std::string ambiguous="x-x";
    if(control_native_menu::ReplaceOnce(ambiguous,"x","y") || ambiguous!="x-x")return 2;
    if(argc==3) {
        std::ifstream in(argv[1],std::ios::binary);
        std::string source{std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>()};
        const char* config=R"({"presetE":5,"presetF":6,"rtx40":false,"values":[4,0,144,0,6,60,4,1,0,1,0],"defaults":[4,0,144,0,6,60,4,1,0,1,0],"fgChoices":[0,1,2,3,4,5,6]})";
        if(!control_native_menu::PatchCommons(source,config,result))return 3;
        std::ofstream out(argv[2],std::ios::binary);out<<result;
        source.back()^=1;
        if(control_native_menu::PatchCommons(source,config,result))return 4;
    }
    std::cout<<"PASS native menu bundle guards\n";
}
