# R232: identify the active archive read path

R231 successfully created the DirectStorage factory and installed the asynchronous read hook, but all 21 reports showed zero calls. It did not serve game data.

R232 adds an observer at runtime RVA 0x1933F1, the synchronous WinFileSystem::readFile ReadFile call. The runtime SHA and six original call bytes are validated before patching. The other three runtime ReadFile sites belong to command-line pipe handling and remain untouched.

Every read still uses native ReadFile with its original arguments, result, error, and implicit file-position behavior. Counters track calls, completed bytes and failures. One in 64 calls measures native ReadFile duration and identifies .rmdp handles after completion. No file paths are logged. The path query adds some diagnostic overhead; sampled times exclude that query and are not full-frame timings. No native pointers are retained. No synchronous data is served by the cache yet.

Run Start-CPU-R232-DirectStorage.cmd from the full extracted package beside Control_DX12.exe. Keep dxgi.dll, dstorage.dll and dstoragecore.dll in that directory. Only one pinned run is needed. The budget remains 8 ms, partitions 16, expensive preparation phase hooks off. Send compact logs after loading and traversing the problem area.

Expect DIRECTSTORAGE_R232 sync_observer=1 and periodic DIRECTSTORAGE_R232_SYNC lines. Nonzero calls plus archive_samples confirm traffic through this boundary. Zero calls would require tracing another module or I/O mechanism; do not infer DirectStorage activity from factory readiness alone. Existing asynchronous R230 status counters remain unchanged.

Validation includes actual synchronous file reads for exact bytes, implicit-position advancement, sample stride, archive classification and failure LastError preservation, plus existing cache, runtime loader and native patch fixtures on Windows CI. Accepted rendering behavior is unchanged. This is a diagnostic build, with no promised speedup.
