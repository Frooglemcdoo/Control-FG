# Third-party notices

Control FG's binary package redistributes selected production runtime components from **NVIDIA Streamline 2.14.1**:

- `sl.interposer.dll`
- `sl.common.dll`
- `sl.pcl.dll`
- `sl.reflex.dll`
- `sl.dlss_g.dll`
- `nvngx_dlssg.dll`
- `sl.dlss_d.dll`
- `nvngx_dlssd.dll`

The upstream Streamline license notice is reproduced in `STREAMLINE-LICENSE.txt`.

NVIDIA Streamline also publishes third-party notices for software included in or used by the SDK. The pinned source reference used for Control FG is:

`NVIDIA-RTX/Streamline @ 2122257e0fce486f91b385aa63b9a09b0a34b363`

The **optional r21w/r21x diagnostic reference-clamp add-on** uses a behavior-equivalent implementation derived from the Control-RR specular temporal clamp in RenoDX revision:

`clshortfuse/renodx @ 120663347a89ebe36c37eeb14653ad6b50958bc6`

RenoDX is MIT licensed. Its license is reproduced in `RENODX-LICENSE.txt`.

The diagnostic add-on interoperates with the ReShade add-on API and pins its ABI assumptions to the ReShade revision used as the RenoDX submodule at that commit:

`crosire/reshade @ 4a50d1eddace85734871d91792ff214f13f66c01` (ReShade API 18)

The ReShade license is reproduced in `RESHade-LICENSE.txt`. ReShade itself is **not** redistributed by Control FG; users provide their own full add-on-capable ReShade build only when deliberately running the optional reference diagnostic. The native r21x path does not require ReShade.

Control FG does not redistribute Control game files. *Control* and related trademarks/assets belong to their respective owners.

RTX 40 test only: RTX40MFG-minimal (danzig666), derived from RTX40MFG-Unlock (Michael Robles / dashdogy), MIT. See RTX40MFG-MINIMAL-LICENSE.txt and RTX40MFG-UPSTREAM-LICENSE.txt and CANDIDATE-README.md for pinned revisions.
