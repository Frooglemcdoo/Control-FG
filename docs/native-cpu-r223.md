# R223: native material dirty-writer attribution

R222's stable 4,908-entry / 2,089-output table showed 3,649 global rebuilds between snapshots separated by 3,968 material updates (~92%). Each output rebuild visits 35 texture slots per output record, or 73,115 slots. Stable table sizes do not prove stable contents. The next question is which native writers keep requesting rebuilds.

## Run

Install over R222. Run `Start-CPU-R223.cmd`, hold the same problem-area view for 30–60 seconds without recording, exit, and run `Collect-ControlFG-Compact-Logs.cmd`. Send the ZIP and FPS range. This is diagnostic, not a performance fix. Both append experiments remain bypassed; R217 visuals, Map Guard OFF and angled reflections remain unchanged.

## Measurements

`NATIVE_CPU_R223_INSTALL ready=1` confirms all eight probes installed. `NATIVE_CPU_R223_WRITER` reports cumulative hit counts for each direct store, at the existing five-second report cadence. Subtract snapshots around steady gameplay. Counts represent dirty-store executions, not clean-to-dirty transitions, unique materials, individual rebuilds, or evidence that an update is unnecessary. Multiple writes can lead to one rebuild. Counters are global across manager instances and threads.

| RVA | Category |
| --- | --- |
| 0xC481C | Manager initialization |
| 0xC4DA5 | Entry insertion |
| 0xC4F8F | Entry removal |
| 0xC7B96 | Changed material hash |
| 0xC7BB0 | Texture change callback |
| 0xC99FE | Texture unbind/destruction |
| 0xC9A7D | Texture binding/listener registration |
| 0xC9A99 | Texture-listener invalidation |

These are all eight matching direct `mov byte [register+0x118],1` instructions found in the verified renderer's executable sections. This does not prove absence of indirect/aliased writes from other code or modules. Texture binding marks dirty even on its same-pointer branch; do not suppress it merely because pointer identity stayed constant.

## Probe behavior and validation

Startup verifies the renderer identity and each complete original store. Each 7/8-byte store becomes a relative call plus NOP padding. A nearby stub executes pushfq, the unchanged native store, a locked increment to a separate RW counter page, popfq, and ret. It changes no general-purpose registers, preserves flags and performs no C++ calls, TLS lookups, allocations or log writes on hits. Stub code is RX; Windows unwind metadata describes its eight-byte flag-save stack allocation and is registered before patching. Partial installation attempts rollback. Stub pages, counter pages and unwind tables remain allocated until process exit so a surviving probe never references freed storage.

Windows tests execute all eight store encodings, verify original writes, carry/zero/sign/overflow/parity flags, LastError, exact concurrent counters, rollback and RtlVirtualUnwind at entry/body/pop/return instruction boundaries. A hit still adds a call and locked counter increment, so do not use R223 to claim a performance improvement or precise overhead-free frame times.

Set CONTROLFG_CPU_DIRTY_WRITERS=0 before launch to omit these probes. The supplied launcher explicitly enables R222 workload and R218 timing diagnostics as well. No dirty-flag suppression, caching or extra worker scheduling is introduced.
