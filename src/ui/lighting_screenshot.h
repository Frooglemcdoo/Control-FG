#pragma once
#include <wincodec.h>
#include <wrl/client.h>
#pragma comment(lib,"windowscodecs.lib")
#pragma comment(lib,"ole32.lib")

namespace native_lighting::screenshots {
// UI thread only. Captures the displayed game client, never the desktop bounds.
// Display-referred PNG, not a linear/HDR render-target dump.
inline HRESULT Save(HWND game,const wchar_t* path) noexcept {
    using Microsoft::WRL::ComPtr;
    if(!game || GetForegroundWindow()!=game || IsIconic(game))return E_ABORT;
    RECT rect{};if(!GetClientRect(game,&rect))return E_FAIL;
    const int width=rect.right,height=rect.bottom;
    if(width<=0 || height<=0 || width>16384 || height>16384)return E_INVALIDARG;
    const UINT stride=(UINT(width)*3+3)&~3u;
    if(UINT64(stride)*height>512ull*1024*1024)return E_OUTOFMEMORY;
    BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth=width;info.bmiHeader.biHeight=-height;
    info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=24;info.bmiHeader.biCompression=BI_RGB;
    HDC screen=GetDC(game),memory=screen?CreateCompatibleDC(screen):nullptr;
    void* pixels=nullptr;HBITMAP bitmap=memory?CreateDIBSection(screen,&info,DIB_RGB_COLORS,&pixels,nullptr,0):nullptr;
    HGDIOBJ old=bitmap?SelectObject(memory,bitmap):nullptr;
    HRESULT hr=E_FAIL;
    if(bitmap && BitBlt(memory,0,0,width,height,screen,0,0,SRCCOPY|CAPTUREBLT) && GdiFlush() && GetForegroundWindow()==game) {
        ComPtr<IWICImagingFactory> factory;ComPtr<IWICStream> stream;
        ComPtr<IWICBitmapEncoder> encoder;ComPtr<IWICBitmapFrameEncode> frame;
        hr=CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory));
        if(SUCCEEDED(hr))hr=factory->CreateStream(&stream);
        if(SUCCEEDED(hr))hr=stream->InitializeFromFilename(path,GENERIC_WRITE);
        if(SUCCEEDED(hr))hr=factory->CreateEncoder(GUID_ContainerFormatPng,nullptr,&encoder);
        if(SUCCEEDED(hr))hr=encoder->Initialize(stream.Get(),WICBitmapEncoderNoCache);
        if(SUCCEEDED(hr))hr=encoder->CreateNewFrame(&frame,nullptr);
        if(SUCCEEDED(hr))hr=frame->Initialize(nullptr);
        if(SUCCEEDED(hr))hr=frame->SetSize(width,height);
        WICPixelFormatGUID format=GUID_WICPixelFormat24bppBGR;
        if(SUCCEEDED(hr))hr=frame->SetPixelFormat(&format);
        if(SUCCEEDED(hr) && format!=GUID_WICPixelFormat24bppBGR)hr=E_FAIL;
        if(SUCCEEDED(hr))hr=frame->WritePixels(height,stride,stride*height,static_cast<BYTE*>(pixels));
        if(SUCCEEDED(hr))hr=frame->Commit();
        if(SUCCEEDED(hr))hr=encoder->Commit();
    }
    if(old)SelectObject(memory,old);if(bitmap)DeleteObject(bitmap);
    if(memory)DeleteDC(memory);if(screen)ReleaseDC(game,screen);
    if(FAILED(hr))DeleteFileW(path);
    return hr;
}
}
