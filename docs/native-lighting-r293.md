# R293 — diagnostic retention headroom and read-only inspection

Extract beside Control_DX12.exe preserving folders. Run
ControlTOH/Diagnostics/Start-GI-Metadata-Test.cmd, keep RT indirect lighting on,
move around the same area for one minute, exit normally, and send the diagnostic
ZIPs. No hotkeys or fixed camera. Normal launch leaves the observer off.

R292 recorded 740826 successful build-time range lookups and 1581 missing vertex
lookups; index/transform/instance misses remained zero. All 128 distinct retained
miss sample keys matched historically evicted rank-2 shader-candidate ranges.
The inventory repeatedly hit its 512 MiB logical extent budget. Historical
address matches support cache pressure as the cause but cannot prove current
resource identity after address reuse. No tracking fault occurred across 3349
submissions; the final metadata fence completed.

The final completed plan included 7978 retained BLAS records and one TLAS, 24797
ranges, 22011 resolved ranges and 2786 missing ranges. All resolved ranges were
in default heaps. Retained records can be historical, so these are not active
scene counts. GPU byte capture remains blocked on lifetime/state/dependency
validation, not just heap type or descriptor submission completion.

R293 raises the diagnostic owner extent budget from 512 MiB to 1 GiB, retaining
8192 slots and the R291 priority policy. This is a bounded coverage experiment,
not a permanent production memory requirement. It can keep resources alive longer
and increase memory use during this diagnostic run; logical extents are not a
measurement of actual physical VRAM. Nothing is reserved upfront by this limit.

Historical plan inspection now resolves without promoting or refreshing owners.
Actual build-time lookups continue protecting geometry. Unique inspection owners
are held temporarily until that inspection finishes; they do not become permanent
cache pins. GI_PLAN_COVERAGE_R293 adds per-kind missing counts and inspection_ms.
The 65536-range plan and 65536-event journals remain bounded. Replacement GI stays
disabled; no GPU copies, Map, barriers, queue waits or rendering changes are added.
