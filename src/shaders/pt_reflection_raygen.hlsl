// P3 DXR injection proof. Build trigger after workflow validation.
// This raygen intentionally writes nothing. If it is selected successfully,
// Control's native reflection hit buffers remain untouched for the dispatch,
// making the reflection contribution disappear. P4 replaces this with real rays.
[shader("raygeneration")]
void ptReflectionRayGeneration()
{
}
