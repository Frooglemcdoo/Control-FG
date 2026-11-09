#pragma once

// Streamline wrappers and native COM interfaces can represent the same device
// at different addresses. Unwrap first, then compare canonical IUnknowns.
// The resolver returns an owned reference on success, just like Streamline.
template<class ResolveNative>
static IUnknown* FGUICanonicalIdentity(IUnknown* object,ResolveNative resolve) noexcept {
    if(!object)return nullptr;
    IUnknown* native=nullptr;
    if(!resolve(object,&native)||!native){
        if(native)native->Release();
        native=object;native->AddRef();
    }
    IUnknown* identity=nullptr;
    const HRESULT hr=native->QueryInterface(IID_PPV_ARGS(&identity));
    native->Release();
    if(FAILED(hr)){if(identity)identity->Release();return nullptr;}
    return identity;
}

template<class ResolveNative>
static bool FGUISameDevice(IUnknown* a,IUnknown* b,ResolveNative resolve) noexcept {
    IUnknown* left=FGUICanonicalIdentity(a,resolve);
    IUnknown* right=FGUICanonicalIdentity(b,resolve);
    const bool same=left&&right&&left==right;
    if(left)left->Release();
    if(right)right->Release();
    return same;
}
