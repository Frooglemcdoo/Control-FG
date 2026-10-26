// Assemble audited LLVM IR and require DXIL validation before emitting a shader.
#define NOMINMAX
#include <windows.h>
#include <dxcapi.h>
#include <wrl/client.h>
#include <fstream>
#include <iterator>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <limits>
using Microsoft::WRL::ComPtr;
static void Check(HRESULT status,const char* message){if(FAILED(status))throw std::runtime_error(message);}
int wmain(int argc,wchar_t** argv){
 if(argc!=4){std::cerr<<"Usage: assemble-validated-dxil input.ll output.dxil dxcompiler.dll\n";return 2;}
 try{
  HMODULE module=LoadLibraryExW(argv[3],nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
  if(!module)throw std::runtime_error("Cannot load selected DXC library");
  auto create=reinterpret_cast<DxcCreateInstanceProc>(GetProcAddress(module,"DxcCreateInstance"));
  if(!create)throw std::runtime_error("DXC factory unavailable");
  std::ifstream input(argv[1],std::ios::binary);
  std::vector<char> bytes((std::istreambuf_iterator<char>(input)),{});
  if(bytes.empty()||bytes.size()>std::numeric_limits<UINT32>::max())throw std::runtime_error("Invalid IR input");
  ComPtr<IDxcLibrary> library;Check(create(CLSID_DxcLibrary,IID_PPV_ARGS(&library)),"Create library");
  ComPtr<IDxcBlobEncoding> source;Check(library->CreateBlobWithEncodingFromPinned(bytes.data(),static_cast<UINT32>(bytes.size()),CP_UTF8,&source),"Create source");
  ComPtr<IDxcAssembler> assembler;Check(create(CLSID_DxcAssembler,IID_PPV_ARGS(&assembler)),"Create assembler");
  ComPtr<IDxcOperationResult> assembled;Check(assembler->AssembleToContainer(source.Get(),&assembled),"Assemble IR");
  HRESULT status=E_FAIL;Check(assembled->GetStatus(&status),"Get assembler status");
  ComPtr<IDxcBlobEncoding> errors;assembled->GetErrorBuffer(&errors);
  if(errors)std::cerr.write(static_cast<const char*>(errors->GetBufferPointer()),static_cast<std::streamsize>(errors->GetBufferSize()));
  Check(status,"Assembly failed");
  ComPtr<IDxcBlob> container;Check(assembled->GetResult(&container),"Read container");
  ComPtr<IDxcValidator> validator;Check(create(CLSID_DxcValidator,IID_PPV_ARGS(&validator)),"Create validator");
  ComPtr<IDxcOperationResult> validated;Check(validator->Validate(container.Get(),DxcValidatorFlags_InPlaceEdit,&validated),"Validate DXIL");
  Check(validated->GetStatus(&status),"Get validation status");errors.Reset();validated->GetErrorBuffer(&errors);
  if(errors)std::cerr.write(static_cast<const char*>(errors->GetBufferPointer()),static_cast<std::streamsize>(errors->GetBufferSize()));
  Check(status,"DXIL validation failed");
  ComPtr<IDxcBlob> result;Check(validated->GetResult(&result),"Read validated shader");
  std::ofstream output(argv[2],std::ios::binary);
  output.write(static_cast<const char*>(result->GetBufferPointer()),static_cast<std::streamsize>(result->GetBufferSize()));
  if(!output)throw std::runtime_error("Cannot write shader");
  // Keep the DLL loaded until process exit so COM objects release safely.
 }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
 return 0;
}
