// Compile-only gate: MSVC must produce the exact name imported by Control.
// This object is never linked into the probe.
namespace d3d {
class NativeTexture;
class DLSS {
public:
    __declspec(dllimport) static bool doAntiAliasing(
        NativeTexture*, NativeTexture*, NativeTexture*, NativeTexture*,
        NativeTexture*, NativeTexture*, NativeTexture*, NativeTexture*,
        NativeTexture*, bool, double, double, float, float, float);
};
}
auto ControlFGAbiCheck = &d3d::DLSS::doAntiAliasing;
