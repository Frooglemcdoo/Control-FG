# R231: DirectStorage runtime loading repair

Copy the full package into the game folder. **dxgi.dll, dstorage.dll and dstoragecore.dll must all be beside Control_DX12.exe.** Having the runtime DLLs only in the DirectStorage subfolder is insufficient for this build. Keep the supplied license notices with the package.

Fully exit Control, then run **Start-CPU-R231-DirectStorage.cmd**. Use the same problem area and traversal, then send compact logs and texture-arrival, stutter and FPS observations. No Reference run or standalone validator is needed.

## Why this repair

The R230 session 20261030-171025-018-68128 reported `DIRECTSTORAGE_R230 disabled=factory` and `ready=0`, with no DirectStorage status reports. The pin held 8 ms / 16 partitions across 33 snapshots. No archive reads were served by DirectStorage, so that run cannot establish its performance impact.

Inspection of the pinned SDK wrapper shows that core loading can use System32 and an application-EXE-relative fallback. R230 loaded the wrapper from a subfolder but omitted the runtime pair from the EXE directory. The earlier standalone test had both DLLs beside its EXE, so it did not exercise the game deployment layout. This is a confirmed packaging discrepancy; the old failure log did not capture an HRESULT, so it cannot conclusively identify which factory failure occurred on the user's machine.

R231 ships and loads the runtime pair beside the game EXE. It adds separate export/load/factory errors with HRESULT and Win32 codes. A new test uses the production dynamic-loader helper, has no static DirectStorage import, runs from a different working directory, and checks factory/cache-queue creation with the EXE-adjacent pair.

## Expected logs

```
DIRECTSTORAGE_R231 factory_ready=1 deployment=exe_directory hr=0x00000000
DIRECTSTORAGE_R230_INSTALL ready=1
DIRECTSTORAGE_R230_STATUS ... calls=... served=... native_fallback=... served_bytes=...
STREAMING_R229_STATE ... pin_installed=1 budget_ms=8.000 slices=16
```

R230 labels remain on the unchanged archive-cache implementation. Installation success alone is insufficient: increasing `served` counters prove real game reads consumed DirectStorage data. If initialization fails again, the new error code will distinguish loader, export and factory failures.

The 32 MiB read-ahead cache, native fallback, native decoding and texture publication are unchanged. GPU decompression is not integrated. Phase hooks remain off, and the 8 ms / 16-partition configuration is unchanged. No performance or stutter improvement is claimed before a successful game run.
