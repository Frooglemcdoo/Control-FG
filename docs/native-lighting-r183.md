# R183: fix component launcher selection

The supplied R182 capture was complete, but recorded `requested=full applied=1`
despite launcher option 2. The old launcher ran SET after CHOICE and before
testing ERRORLEVEL. R183 branches on CHOICE immediately, before changing any
environment variables, and rejects cancellation/error returns.

Windows CI executes the exact launcher selection block for options 1, 2 and 3,
including a stale inherited setting, plus cancellation/error cases. It also
prints the old ordering's resulting ERRORLEVEL for diagnosis.

The reflection shaders are unchanged from R182. This fixes test selection;
it does not claim a quality or performance improvement.

Close Control, install the inner package beside Control_DX12.exe, and run
Start-Spec-Component-Test.cmd. Choose option 2. Before capturing, open F10:
the bottom diagnostic label must read **REFLECTION: SHADING**. If it instead
reads SPEC SHADING: SERIAL, do not repeat the capture; send SAVE LOG. This
check catches both selection failure and any launcher/process handoff that
loses the environment variable.

With the correct label, select NATIVE BLEND, RR on, FG off and SPEC4. Capture
the painting and elevator, then PACKAGE RR and SAVE LOG. Keep all package ZIPs.
Close Control and repeat with option 3; its label must be **REFLECTION: EMISSION**.
Send the packages labeled 2 and 3. No full-baseline or SPEC8/16 repeat is needed.

Option 2 removes reflected material emission, retaining other reflected shading.
Option 3 retains reflected emission only. Missing reflected lighting is expected.
Both retain the original total-radiance basis for the per-hit cap. Later filtering
and exposure are nonlinear, so final captures are not directly additive.
Normal launch or option 1 restores the accepted full-reflection path.
