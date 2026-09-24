#include <cassert>
#include <cstdio>
using HRESULT=int;
constexpr HRESULT S_OK=0,E_FAIL=-1;
#define FAILED(x) ((x)<0)
#define IID_PPV_ARGS(p) 0,p
struct IUnknown {
 IUnknown* canonical=this;
 int refs=1;
 bool queryFails=false;
 void AddRef(){++refs;} void Release(){--refs;assert(refs>=1);}
 HRESULT QueryInterface(int,IUnknown** out){if(queryFails){*out=nullptr;return E_FAIL;}*out=canonical;canonical->AddRef();return S_OK;}
};
#include "../../src/fg_ui_device_identity.h"
int main(){
 IUnknown native, other, proxy, alias, secondProxy;
 alias.canonical=&native;
 auto resolve=[&](IUnknown* in,IUnknown** out){*out=nullptr;if(in==&proxy||in==&secondProxy){*out=&native;native.AddRef();return true;}return false;};
 assert(FGUISameDevice(&native,&native,resolve));
 assert(FGUISameDevice(&proxy,&native,resolve));
 assert(FGUISameDevice(&native,&proxy,resolve));
 assert(FGUISameDevice(&proxy,&secondProxy,resolve));
 assert(FGUISameDevice(&alias,&native,resolve));
 assert(!FGUISameDevice(&proxy,&other,resolve));
 assert(!FGUISameDevice(nullptr,&native,resolve));
 assert(!FGUISameDevice(nullptr,nullptr,resolve));
 other.queryFails=true;assert(!FGUISameDevice(&other,&other,resolve));
 assert(native.refs==1&&other.refs==1&&proxy.refs==1&&alias.refs==1&&secondProxy.refs==1);
 puts("PASS production device identity: native/proxy in both directions, two wrappers, COM alias, distinct devices rejected, null/QI failures rejected, references balanced");
}
