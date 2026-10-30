# R227 evidence and R228 scope

User supplied Native and Phases logs plus standalone DirectStorage validation on 2026-10-30. The user identified Phases as the stuttering run. This is not a controlled proof of instrumentation overhead: scene workloads and routes differ.

Native session 20261030-135912-686-85684 started with 8 ms / 16 partitions, then logged a mod-menu application with previous budget 8 and previous partitions 2. Static inspection of the supplied renderer confirms native settings stores to 0x919010 at 0x1377A2 (R12D = 2) and 0x13781E (immediate 8). Source field renderer+0x6B0 determines the branch; its quality-setting label is not established. Vtable slot 0x632518 targets the enclosing routine 0x1374B0. R228 pins these two writes only when explicitly enabled; it does not wrap that large routine or assume its return ABI.

Phases session 20261030-140243-076-88096, cumulative differences from present 8460 through 9180, 15 matched samples:

| Preparation interval | Mean ms |
| --- | ---: |
| Before readback | 0 |
| Readback processing | 0 |
| Scene selection and assembly | 3.223527 |
| Prebuild sizing | 0.011580 |
| Buffer management/upload | 0.062787 |
| Total | 3.297893 |

No readback in this short stable interval. Scene bucket is approximately 97.75% of measured preparation and includes initial query work. It includes probe overhead and scheduling; do not interpret all of it as recoverable time. Native sessions have different workloads; do not subtract them from Phases to claim overhead or gains.

## Static RT findings

At A83D9, virtual query call [R10+0x50] fills context+0x10's object array. It precedes optional readback processing. Query implementation and its cost have not yet been separated from subsequent assembly.

The object loop selects LOD, reads streamed BLAS availability and processes static and dynamic paths. A8CE0 appends geometry records, resolves current material pointers/flags, transposes transforms and obtains current BLAS addresses. The native append sites were already tested in R219/R220 without repeatable FPS gains. Replacing those appends again is not a new architectural optimization.

Persistent output reuse requires more than stationary transforms: LOD, streaming readiness, BLAS replacement, material table remapping and destruction can invalidate it. No persistent cache is introduced in R228.

## DirectStorage result

Adapter 10DE:2B85, software=0; support_flags=0x19 = GPU_OPTIMIZED | USES_COMPUTE_QUEUE | USES_COPY_QUEUE. Sixteen chunks, 4,194,304 uncompressed bytes, 2,257,464 stored bytes; file-to-GPU GDeflate byte equality passed; native_game_integration=0. This proves synthetic decode correctness on the user's adapter, not compatibility with native texture payloads or game throughput.

Keep the 8 ms responsiveness goal. Map actual texture metadata/payload layout and loader completion/cancellation/publication before redirecting native requests. No DirectStorage game path is introduced in R228.
