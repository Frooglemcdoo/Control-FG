#pragma once
#include <d3dcompiler.h>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include <cstring>
// The legacy compiler's standard include object does not resolve nested local
// includes relative to these repository shaders consistently. Use explicit test
// search roots; production sidecars are still built by DXC with -I src/shaders.
class PTTestShaderIncludes : public ID3DInclude {
public:
    HRESULT STDMETHODCALLTYPE Open(D3D_INCLUDE_TYPE,LPCSTR name,LPCVOID,LPCVOID* data,UINT* bytes) override {
        if(!name||!data||!bytes)return E_INVALIDARG;
        for(const auto& root:{std::string("tools/"),std::string("src/shaders/"),std::string()}){
            std::ifstream file(root+name,std::ios::binary);if(!file)continue;
            const std::vector<char> content((std::istreambuf_iterator<char>(file)),{});
            if(content.empty()||content.size()>1024*1024)return E_FAIL;
            auto* storage=new char[content.size()];std::memcpy(storage,content.data(),content.size());*data=storage;*bytes=static_cast<UINT>(content.size());return S_OK;
        }return E_FAIL;
    }
    HRESULT STDMETHODCALLTYPE Close(LPCVOID data) override {delete[] static_cast<const char*>(data);return S_OK;}
};
