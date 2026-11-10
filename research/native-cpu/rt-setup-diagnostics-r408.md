# R408: withdraw larger reservations and capture the missing timing evidence

User reports no performance difference with R407 and possibly worse dips.
Read complete R406/R407 collector logs, installation markers and error lines.
R407 probe-20261110-012610-521-5028.log has all active installs ready, no errors.
After the first 16384 applications, 90388 of 98304 targeted allocator calls
were avoided (91.9474%). This is coverage, not a frame-rate improvement.

Warm dynamic CPU cost: R406 0.817549 ms/job, R407 0.917080 ms/job.
Their sampled updates/job are 24.4330 versus 29.1250. Provider CPU us/update
are 31.6263 versus 29.5244. Scene objects/loop are 6717.30 versus 8058.14.
Different workloads prevent claiming either a controlled gain or regression.
The R407 detail cohort reports 16.625 us/application in shaders; ordinary
whole-application samples report 22.9877 us. They are separate populations.
No per-frame intervals or allocator/sampler/initialization split exists in
those collectors. Earlier reviewed logs likewise cannot identify these dips.
No identical repeat capture requested. Larger reservations consume potentially
unused space but heap churn as the dip's cause has not been demonstrated.

## Runtime changes

Remove R407 runtime include, job scope and application scope. Do not install
its allocator call patch; log intentional disablement. Normal reservation
size is restored to 49. Retain R405 current SRV batching and the owned jobs.

Build-lock and byte-check D3D 2F050..2F1B1 and 1BD50..1BDEF; patch only the
call at 2F0A9 -> 1BD50. With no active detailed sample, call original 1BD50.
For the existing one-in-256 application sample, reproduce its exact calls:
1AA00(49) publishes state+30/+40/+50, 1ACF0(0 or 4) publishes +38/+48/+58,
then conditional native CopyDescriptorsSimple(49) from manager+A8 to the
current state+40. Both native allocators ignore the first argument; all
lifetime, lock, heap creation/reuse and retirement logic remains native.
The initialization uses the current device vtable, preserving observers.
A thread-local reentrancy guard forwards nested preparations to native.

Four timing boundaries distinguish allocation, sampler, and initialization.
The same sampled calls feed all three totals. Report clear and nonzero-sampler
counts plus the slowest sampled preparation's QPC span and its phase times.
A short statistics lock is taken after timing, released before logging.
Ordinary whole-application samples continue to exclude detailed samples.
Timing includes instrumentation and possible CPU preemption, not GPU work.

At the existing engine present-entry hook, read QPC once. Aggregate 300
intervals, report counts at 25/33/50/100 ms, and retain the four longest spans
with present indices and begin/end QPC. Log only at window boundaries, not
at every dip. This adds one line per 300 intervals. Its measured reporting cost is attached
to the following interval (and included in each window and top-four record),
so a slow logging operation is visible rather than misattributed. The same QPC clock
allows time correlation with sampled preparation; sampling can miss a stall,
and correlation does not prove causation. Menus/loading/pauses are explicitly
included; these are not generated frame, display, or GPU measurements.

## Validation

Extend the existing 4096-case native 1BD50 oracle to compare the sampled
implementation with the original machine code, including sampler modes,
initialization on/off, exact output fields, callbacks and phase attribution.
Alternate null/enabled timing callbacks. Existing lease oracle retained as
an offline regression test, not enabled in the game.

Add interval-window tests for threshold boundaries, total durations, top-four
ordering, retained QPC spans and reset. Existing binding/GPU/ownership tests
remain required. Full runtime header syntax checked locally before Windows.
