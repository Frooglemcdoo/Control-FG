#include "../src/pt_surface_composition.h"
#include <fstream>
#include <iterator>
#include <string>
#include <cassert>
#include <cstdio>
// Private captured shaders are supplied locally; never embed them in this repo.
int main(int argc,char** argv){
    if(argc!=3){std::puts("Usage: test-pt-surface-composition shader-directory output-directory");return 2;}
    for(const auto& profile:PTSurfaceComposition::Profiles){
        std::ifstream stream(std::string(argv[1])+"/"+profile.sha+".dxbc",std::ios::binary);
        const std::vector<unsigned char> original((std::istreambuf_iterator<char>(stream)),{});std::vector<unsigned char> patched;
        assert(PTSurfaceComposition::Patch(original.data(),original.size(),profile.sha,patched));
        const auto checksum=PTDxbc::Checksum(patched.data()+20,static_cast<unsigned>(patched.size()-20));
        assert(!std::memcmp(checksum.data(),patched.data()+4,16));
        std::ofstream output(std::string(argv[2])+"/"+profile.sha+".dxbc",std::ios::binary);assert(output);
        output.write(reinterpret_cast<const char*>(patched.data()),static_cast<std::streamsize>(patched.size()));assert(output);
        auto corrupt=original;corrupt.back()^=1;assert(!PTSurfaceComposition::Patch(corrupt.data(),corrupt.size(),profile.sha,patched)&&patched.empty());
        assert(!PTSurfaceComposition::Patch(original.data(),original.size(),"unreviewed",patched)&&patched.empty());
    }
    std::puts("PASS all reviewed general and character surface variants: complete patch, container checksum, corruption/unknown rejection.");
}
