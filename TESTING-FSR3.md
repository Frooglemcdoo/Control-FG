# Control FG — FSR3 alpha1 test

This first AMD FSR 3 frame-generation gate targets fixed 2x, synchronous work, and SDR. It keeps Control's existing DLSS Super Resolution path and replaces only frame generation and presentation.

1. Back up and remove the existing Control FG dxgi.dll and ControlFGStreamline folder.
2. Copy this build's dxgi.dll and ControlFGFidelityFX beside Control_DX12.exe.
3. Disable Windows and game HDR for the first run.
4. Launch Control in DX12 mode, load gameplay, and move the camera for at least 30 seconds.
5. Exit, run Collect-ControlFG-Logs.cmd, and upload the resulting ZIP.

Expected markers: FSR3_CORE_READY; FSR3_SWAPCHAIN_CREATE result=0; FSR3_PROVIDER_SELECTED; FSR3_CONTEXT_CREATE result=0; FSR3_PREPARE result=0; FSR3_CONFIGURE enabled=1; FSR3_GENERATION_CALLBACK generated=1 result=0.

The build fails open to Control's native swapchain if AMD's loader or swapchain cannot initialize. FSR 4 is deliberately rejected on this RTX test path. HDR interpolation is disabled in alpha1.
