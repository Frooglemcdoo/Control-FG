#include <windows.h>
#include <d3dcompiler.h>
#include <d3d11shader.h>
#include <cstdio>
#include <vector>
#include <string>

struct Item { const wchar_t* path; const char* symbol; };

static bool CompileOne(const Item& item, std::vector<unsigned char>& bytes) {
    ID3DBlob* code=nullptr; ID3DBlob* errors=nullptr;
    HRESULT hr=D3DCompileFromFile(item.path,nullptr,D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "main","cs_5_1",D3DCOMPILE_OPTIMIZATION_LEVEL3|D3DCOMPILE_WARNINGS_ARE_ERRORS,0,&code,&errors);
    if(errors){std::fwrite(errors->GetBufferPointer(),1,errors->GetBufferSize(),stderr);errors->Release();}
    if(FAILED(hr)||!code){if(code)code->Release();return false;}
    ID3D11ShaderReflection* r=nullptr;
    hr=D3DReflect(code->GetBufferPointer(),code->GetBufferSize(),__uuidof(ID3D11ShaderReflection),reinterpret_cast<void**>(&r));
    if(FAILED(hr)||!r){code->Release();return false;}
    UINT x=0,y=0,z=0;r->GetThreadGroupSize(&x,&y,&z);
    D3D11_SHADER_DESC d{};const bool okay=SUCCEEDED(r->GetDesc(&d))&&x==8&&y==8&&z==1&&d.BoundResources>=2;
    r->Release();
    if(!okay){code->Release();std::fprintf(stderr,"FAIL: GI29 shader ABI %s\n",item.symbol);return false;}
    auto* p=static_cast<const unsigned char*>(code->GetBufferPointer());
    bytes.assign(p,p+code->GetBufferSize());code->Release();return true;
}

static bool Emit(FILE* f,const char* symbol,const std::vector<unsigned char>& bytes){
    if(std::fprintf(f,"static const unsigned char %s[]={\n",symbol)<0)return false;
    for(size_t i=0;i<bytes.size();++i)
        if(std::fprintf(f,"0x%02x,%s",bytes[i],i%16==15?"\n":"")<0)return false;
    return std::fprintf(f,"\n};\nstatic constexpr size_t %sSize=sizeof(%s);\n",symbol,symbol)>=0;
}

int main(){
    const Item items[]={
      {L"src/shaders/gi29_dlf_taa_diffuse.hlsl","kGI29DlfDiffuseTemporal"},
      {L"src/shaders/gi29_dlf_spatial_diffuse_x.hlsl","kGI29DlfDiffuseSpatialX"},
      {L"src/shaders/gi29_dlf_spatial_diffuse_y.hlsl","kGI29DlfDiffuseSpatialY"},
      {L"src/shaders/gi29_dlf_spatial_spec_x.hlsl","kGI29DlfSpecularSpatialX"},
      {L"src/shaders/gi29_dlf_spatial_spec_y.hlsl","kGI29DlfSpecularSpatialY"},
    };
    std::vector<unsigned char> code[5];
    for(unsigned i=0;i<5;++i)if(!CompileOne(items[i],code[i]))return 1+i;
    FILE* f=nullptr;if(fopen_s(&f,"build/rr_dlf_parity_compiled.h","wb")||!f)return 10;
    std::fprintf(f,"#pragma once\n#include <cstddef>\n");
    for(unsigned i=0;i<5;++i)if(!Emit(f,items[i].symbol,code[i])){std::fclose(f);return 11;}
    const bool good=!std::ferror(f);const int closed=std::fclose(f);
    if(!good||closed)return 12;
    std::puts("PASS GI29 direct DLF shaders: diffuse temporal + diffuse/specular spatial X/Y compiled cs_5_1 8x8x1");
    return 0;
}
