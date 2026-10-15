M12R63 - reduce scene queue-ordering overhead

R62 Performance capture confirms PT dispatch at 1920x1080 and RR Performance.
PT GPU time: Quality 1.921 ms (2 windows), Performance 1.132 ms (18 windows).
CPU preparation: 2.926 ms vs 3.049 ms; these are not whole-frame timings.
The internal resolution switch works. Remaining scaling limits are outside
that resolution switch; earlier whole-frame traces show CPU submission work.

R63 preserves cached queue ancestry when a write, mark or wait is appended.
Signals, observed completion changes, compaction and rebases still invalidate
ancestry. New competing writes are always checked, even against cached cuts.
A failed cyclic traversal is invalidated on append so independent cuts can
be evaluated afresh, matching the previous behavior.

AS selection uses bounded inline maximal-writer storage instead of allocating
a temporary tree per lookup. FIFO permits at most one candidate per clock lane.
All ordering/admission rules and GPU waits remain in place.

Local validation includes 199,816 differential selections against R62 with
writes, waits, late signals, completion observations and cycles, plus scene
catalog and sustained queue-ordering regression tests. Not an in-game FPS claim.

Close Control, extract beside dxgi.dll and launch normally with PT enabled.
Compare Quality and Performance in the same scene for 30 seconds each, FG and
caps/VSync off. Close and run Collect-PT-Primary-M12R63.cmd. Send the ZIP and
average FPS. No PT-enable script needed. In-game gain remains unverified.
