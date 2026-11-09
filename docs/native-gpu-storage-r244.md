# R244 extended texture streaming budgets

R244 adds 10 ms and 12 ms to the Texture Streaming Budget slider, saved setting
normalization, renderer application and process-local override parser. Native,
4 ms, 6 ms and 8 ms remain available. Defaults and existing saved values are
unchanged. Unsupported numeric values still normalize to Native.

Copy the package over R243. Choose one launcher per fresh game process:

- Start-CPU-R244-NativeGPUStorage-4ms.cmd
- Start-CPU-R244-NativeGPUStorage-8ms.cmd
- Start-CPU-R244-NativeGPUStorage-10ms.cmd
- Start-CPU-R244-NativeGPUStorage-12ms.cmd

Each launcher enables the same DirectStorage path and pins 16 partitions.
The launcher budget overrides the saved/menu budget for that process; changing
the menu during that run does not change its effective budget. Use the matching
launcher for each comparison. Overrides do not rewrite saved settings.
`STREAMING_R229_STATE` reports the actual budget and slices. All other launcher
settings are identical. Compare the same route and graphics settings, and collect
compact logs after each run. Watch both frame spikes and texture arrival.

The R243 streaming engine remains: embedded repack shader, shared pipeline,
4,096 bounded descriptors, reusable GPU buffers and fence-gated publication of
newly allocated supported 2D textures. The 128 MiB per-lane arena budget is
unchanged. 10/12 ms changes the native streaming time allowance, not the GPU
memory or DirectStorage staging budget. A larger allowance may improve texture
arrival or worsen frame pacing; these options are for measurement, not a claim
that 12 ms is faster or a new default.

Validation covers 10/12 ms menu application, allowed-value normalization and
exact two-digit override parsing, including rejection of trailing junk. The
schema and embedded menu are regenerated together. Existing GPU mip-content,
arena reuse, scheduling and rendering checks remain required.
