# R189: no visible improvement

User report: "looks the same."
Inputs: Lighting-R189-20261027-181933-16180-0.log,
image(20261027-232115).png (elevator), and
Screenshot 2026-10-27 182035.png (painting).

The log reports enabled=1, diagnostic_gain=1, direct_created=1,
library_created=0, failures=0 throughout the observed filter-status records.
It contains effective SPEC4 / RR-selected intervals. SAVE LOG occurred with
RR off; that later state does not invalidate the user's tested outcome.
The screenshots show persistent granular/mottled reflection artifacts.
There is no exact cross-run camera match, and no numeric image-quality or
performance improvement is inferred.

Disposition: R189 roughness-dependent volume-direction blend is not an
accepted improvement. Do not require a repeat of this test or carry it into
the next candidate as an established fix. Use the accepted R184/R176 baseline.

The negative result rejects this specific blend as a practical fix; it does
not establish that roughness is irrelevant or that volume lighting is ruled
out. The actual blend weight and hit-surface distribution were not measured.
Next investigation should examine variation between low-count ray hits and
how the accumulated signal reaches RR, rather than simply increasing the
strength of this same blend or repeating sample-count tests.
