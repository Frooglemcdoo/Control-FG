# Control FG v2.1.0

## What's new in v2.1.0

- **Experimental RTX 40-series Multi Frame Generation**, available as an opt-in setting in Options. Disabled by default; restart Control after enabling it.
- A dedicated **Ray Reconstruction settings page**, with a reflection clamp slider from **25 to 75**, default **60**, and a **Reset to default** button.
- **Overlay shortcut rebinding** in Options.

## Fixes and improvements

- Reworked Frame Generation synchronization and resource handling to address flickering, ghosting, and stability problems.
- Improved HDR/SDR transition handling and Frame Generation recovery.
- Improved HUD/UI handling on generated frames.
- Improved GPU command and resource lifetime management during resizing, resource replacement, and shutdown.
- Added safeguards for stale frames, queue changes, and invalid device/resource combinations.
- Improved RR Model F projection validation and hit-distance handling.
- Refined RR reflection clamping to preserve more natural skin and material appearance.
- Cleaned up the overlay and removed the HDR restart footer.

## Known issues

**RR indirect diffuse lighting:** blinds can show crawling/noisy streaks when Ray Reconstruction and **Ray Traced Indirect Diffuse Lighting** are enabled together. Reported on both RTX 40- and 50-series GPUs; Models E and F behave similarly. **Workaround:** disable Ray Traced Indirect Diffuse Lighting in the game, or disable RR in the overlay.

Periodic frame-time spikes with the overlay visible have also been reported on an RTX 4070. Close the overlay during gameplay if affected. If an HDR transition causes corruption, restart Control with the desired HDR setting. See [KNOWN_ISSUES.md](KNOWN_ISSUES.md) for support details.

## Installation

See [INSTALL.md](INSTALL.md). Download the DropIn ZIP from the release assets; the source archive requires compilation.
