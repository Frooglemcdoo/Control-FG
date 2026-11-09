# R63: final packed tile and shadow-path audit

R62 requested larger spotlight tiles but the user reported no image change.
Its log established requests, not the packer's eventual result. R63 retains the
R62 resolution comparison and native shader, and adds bounded CPU observations.

The reviewed PCF packer at renderer RVA 0x1A1B40 takes the pool lock, packs the
requests, writes record X/Y/width/height at +0x34/+0x38/+0x3C/+0x40, binds the
texture wrapper at +8, and invokes virtual slot 6 at 0x1A1C43 before releasing
the lock. The slot originally targets 0x1A1EF0. R63 forwards that clear call
unchanged, then reads active records while the native pool lock is still held.
Only the exact post-pack caller (return RVA 0x1A1C46) is observed.

A bounded registry correlates request records with selection sequence and light
ID. The first matched pack per selection logs requested and final sizes, atlas
bounds, invalid entries, resized entries and registry overflow. Larger mode
cannot report a confirmed visual result solely from a request: matched packed
records must have valid bounds and match their requested dimensions. These
records are CPU packing evidence, not proof of depth rasterization or shader
sampling. Only observed matching records are covered, not every scene light.

VSM request vtable 0x637198, slot 2, target 0x1A01E0 is observed without changing
its six arguments or return value. Spotlight caller 0x18AC64 with dynamic=0 is
the static-map allocation path; caller 0x18ACBA with dynamic=1 is the dynamic VSM
path. The PCF counterpart records dynamic PCF. Logs share light IDs to identify
lights requesting multiple paths. A request does not prove that the shader
sampled that path at the troubled pixel.

All hook targets and the post-pack call bytes are checked before installation.
Unknown contracts remain disabled. Reads use guarded copies, counts are bounded,
and request pointers are dereferenced only against the active locked pool list.
Registry capacity is 2048; output limits are 128 packed tiles and 512 unique
light/path combinations per selection. After confirmation, tile scanning stops.

Test: Native, return focus to game, wait, then Shadow Res 2X, return focus and
wait. Save the single log even if the status remains unconfirmed. No restart
between modes. This is an evidence build, not a claimed noise fix. R62's extra
atlas capacity remains; native PCF filtering and static shadow assets are unchanged.
