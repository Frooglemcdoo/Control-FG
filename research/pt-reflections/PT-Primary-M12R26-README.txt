Control-FG Primary PT M12R26

Install: close Control and replace every supplied file in the game folder with
this package. Use F10 > PT Reflections. F11 is unchanged. Run the supplied
Collect-PT-Primary-M12R26.cmd after testing and send its results ZIP.

R25 reached the scene and surface composition, but almost every sampled path
failed light validation. Both captured primary shaders optimize point/spot light
counts and environment intensity out of b2. Those unused lanes are not inputs.
R26 reads these values from authenticated engine providers, checks the native
sun count and current camera/frame, and retains an immutable private b2 buffer.
It restores the native b2 after the private dispatch.

Completed output textures can now be reused only after all recording owners and
the final GPU fence retire. Reuse clears output and releases old scene bindings.
This removes repeated allocation of the 4K RGBA32F texture and its queue objects.
GPU diagnostics now sample every 64 pixels on each axis (16x fewer atomic
updates); probe ray density remains one in 256 pixels. Promotion requires three
healthy probe frames instead of one occasional valid frame.

Light-provider values and separate native-light faults are collected in logs.
Windows/WARP fixtures validate API behavior; game visuals and FPS remain to be
verified on the user's hardware. This build does not claim a measured FPS gain.
