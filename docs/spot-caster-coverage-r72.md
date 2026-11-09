# R72: retain nearby off-screen shadow casters

User testing confirms R71 fixed shadow dithering; flicker and larger coverage
pop-in remain. All six R71 spotlight shader assets are byte-identical in R72.
R67 RR filtering and R69 nonblocking diagnostic logging are retained.

## Evidence and change

The verified renderer's two object-collection paths construct a camera convex
volume with `m::View::getVolume`, then offset its side and near planes using the
`rtd:Culling view expansion` or `rtd:Culling view expansion w/o reflections`
option. Defaults are 60 and 1 engine units. The far plane defaults to 150.
Full-distance spotlight rays can need occluders beyond that camera envelope.
This is a concrete coverage limitation; whether it accounts for all reported
flicker/pop-in still requires in-game confirmation.

While RT SPOTLIGHTS is selected and ready, R72 raises both expansion values to
at least the native far-distance value, bounded to 60–200 engine units. Existing
larger expansion values are preserved. The far-distance value is never changed.
The next native collection after selecting NATIVE restores the saved values,
unless the engine/debug controls have since changed them independently.

The shared RT scene may contain more off-screen geometry in this mode, so GPU,
CPU and memory costs can rise. This is a bounded envelope, not unlimited caster
coverage. It does not change geometry masks, triangle culling, light selection,
previous-light history validation, shadow softness or the ray sample count.

## Integration contract

Renderer SHA256: ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433.

- Collection call 0x123f6e -> 0xa81b0, one pointer argument.
- Collection call 0x1b5147 -> 0x1b0fe0, three pointer arguments.
- Both call results are ignored by their callers.
- Expansion loads: 0xa82ad / 0xa82b7 and 0x1b10ca / 0x1b10d4.
- Expansion data: 0x90e790 and 0x90e850; far-distance data: 0x90e910.
- Constructors and option labels confirm the value fields and defaults.

Installation requires the already verified module and exact instruction bytes.
Both hooks must install before the RT replacement is advertised as ready.
Overrides happen before native collection, use aligned atomic writes and a
nonblocking try-lock, and do no render-thread disk I/O. The scalar policy is
shared by both collection hooks. No game files or saved settings are rewritten.

Tests cover bounds, invalid values, a camera-plane example, argument forwarding,
pre-collection application, native restoration, external setting preservation,
and contention returning without waiting. Windows CI also builds the complete
DLL and runs the existing shader/switching/baseline checks.

## Test

Install all R72 package files with Control closed. In the same hallway, enable
RT SPOTLIGHTS and repeat the approach and camera rotation from the R71 video.
Compare NATIVE live via F10. Send one Lighting-R72 log and video, and note FPS.
`rt_spot_culling_hooks ready=1` confirms installation; `rt_spot_culling` records
actual option changes before collection. These CPU logs do not prove which
individual geometry instances reached the GPU scene.
