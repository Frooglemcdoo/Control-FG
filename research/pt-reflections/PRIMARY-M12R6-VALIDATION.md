# M12R6 binding admission

R5 results confirmed ON toggles, healthy scene capture and no primary dispatch
or coverage records. No primary skip/view failures were logged, leaving early
native-job and compute recognition exits unobserved. This evidence does not
establish which early gate was responsible.

Code review found that `Tracker::Change` discarded the first setter on any
command-list interface absent from `aliases`. Vtable interception applies to
all instances sharing that vtable, while creation-time registration only covers
the interfaces actually queried there. Reads also failed for an unregistered
alias even if its canonical IUnknown already owned a recording.

M12R6 registers the first observed caller, joins its canonical identity, and
retains the first update. Existing setter calls still take one mutex and do
not query COM. The WARP fixture checks first-setter retention, a distinct
interface address sharing IUnknown, cross-interface writes and Reset, and no
repeated QueryInterface. It fails against R5's tracker.

Aggregated counters expose native SBT validation, writer scheduling, disabled
callbacks, missing or unmatched compute PSOs, and recognized primary/environment
passes. They do not claim rendering success. Collection retains these records
alongside actual submitted-dispatch and GPU coverage evidence.

R5 performance changes and shader hash gates remain in place. The silent
binding defect is fixed in code; whether it is the only blocker on the user's
machine remains unverified until the integrated run.
