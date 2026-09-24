#include <atomic>
#include <cassert>
#include <cstdlib>
#include <string>
using DWORD=unsigned long;
#define _countof(a) (sizeof(a)/sizeof(a[0]))
bool stored=false,ada=false,known=false,supported=false;
DWORD GetEnvironmentVariableW(const wchar_t*,wchar_t* out,size_t){out[0]=L'C';out[1]=0;return 1;}
unsigned GetPrivateProfileIntW(const wchar_t*,const wchar_t*,unsigned,const wchar_t*){return stored?1:0;}
bool IsFGRtx40Series(){return ada;}
bool IsFGDynamicCapabilityKnown(){return known;}
bool IsFGDynamicMFGSupported(){return supported;}
unsigned NormalizeFGMultiplier(unsigned x){return x;}
constexpr unsigned kSLSelectionOff=0,kSLSelectionDynamic=1;
std::atomic<unsigned> slRtx40MfgRequested{0},slMfgExperimentReady{0};
static bool IsRTX40MFGSessionEnabled() noexcept {
    static const bool enabled = []() noexcept {
        wchar_t localPath[32768]{};
        const DWORD count = GetEnvironmentVariableW(L"LOCALAPPDATA", localPath, _countof(localPath));
        bool value = false;
        if (count && count < _countof(localPath)) {
            const std::wstring path = std::wstring(localPath) + L"\\ControlFG\\settings-mfg-test.ini";
            value = GetPrivateProfileIntW(L"Experimental", L"RTX40MultiFG", 0, path.c_str()) == 1;
        }
        slRtx40MfgRequested.store(value ? 1u : 0u);
        return value;
    }();
    return enabled;
}
static bool IsRTX40MFGRequested() noexcept {
    (void)IsRTX40MFGSessionEnabled();
    return slRtx40MfgRequested.load() != 0;
}
static bool IsRTX40MFGAllowedThisSession() noexcept {
    return IsRTX40MFGSessionEnabled() && IsRTX40MFGRequested();
}
static bool IsFGExperimentalMultiplier(unsigned int selection) noexcept {
    return IsFGRtx40Series() && IsRTX40MFGAllowedThisSession() && slMfgExperimentReady.load() != 0u && selection >= 3u && selection <= 6u;
}
static bool IsFGDynamicAllowedByGpuPolicy() noexcept {
    return !IsFGRtx40Series() || (IsRTX40MFGAllowedThisSession() && slMfgExperimentReady.load() != 0u &&
        IsFGDynamicCapabilityKnown() && IsFGDynamicMFGSupported());
}
static bool IsFGSelectionAllowedByGpuPolicy(unsigned int selection) noexcept {
    selection = NormalizeFGMultiplier(selection);
    if (!IsFGRtx40Series()) return true;
    return selection == kSLSelectionOff || selection == 2u || IsFGExperimentalMultiplier(selection) ||
        (selection == kSLSelectionDynamic && IsFGDynamicAllowedByGpuPolicy());
}
int main(int argc,char** argv){assert(argc==2);unsigned bits=static_cast<unsigned>(std::atoi(argv[1]));stored=(bits&1)!=0;ada=(bits&2)!=0;bool ready=(bits&4)!=0;known=(bits&8)!=0;supported=(bits&16)!=0;
assert(IsRTX40MFGSessionEnabled()==stored);slMfgExperimentReady=ready;
for(unsigned requested=0;requested<2;++requested){slRtx40MfgRequested=requested;
assert(IsFGSelectionAllowedByGpuPolicy(0));assert(IsFGSelectionAllowedByGpuPolicy(2));
for(unsigned m=3;m<=6;++m) assert(IsFGSelectionAllowedByGpuPolicy(m)==(!ada||(stored&&requested&&ready)));
assert(IsFGDynamicAllowedByGpuPolicy()==(!ada||(stored&&requested&&ready&&known&&supported)));
assert(IsRTX40MFGSessionEnabled()==stored);
}}
