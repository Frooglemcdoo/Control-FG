#pragma once
struct RREvaluationInputs {
    ID3D12Resource* color = nullptr;
    ID3D12Resource* output = nullptr;
    ID3D12Resource* depth = nullptr;
    ID3D12Resource* motion = nullptr;
    float jitterX = 0, jitterY = 0;
    int reset = 0;
    unsigned int valid = 0;
    unsigned long long frame = 0;
    DWORD fault = 0;
    unsigned int width=0,height=0;
};
