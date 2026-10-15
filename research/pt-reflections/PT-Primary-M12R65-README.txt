M12R65 - avoid descriptor gate on non-consuming callbacks

R64 results: Quality PT GPU time averages 1.917 ms across four 120-frame
windows; Performance averages 1.132 ms across two. Actual ray dimensions
are 2560x1440 and 1920x1080. No new descriptor-change rejection or late
admission failure appears. Upload allocation stays at 17 after warmup.
CPU preparation remains about 3 ms; lower resolution does not reduce it.
Performance has a short stable sample, so its regression is not fully localized.

Retain R64's preparation unlock and strict descriptor version recheck.
R65 additionally acquires the descriptor gate in Signalling only after a
matching deferred fence is found, in Selected only for lists with PT
producers/draws, and in Drawing only after view/producer matching finishes.
Actual descriptor snapshots, submission proof validation and admission stay
inside the gate. Queue ordering, GPU waits, shaders, settings and quality
remain unchanged. This targets remaining contention, not a proven complete
explanation of the Performance regression.

Close Control, extract beside dxgi.dll, launch with PT enabled. Keep FG and
caps/VSync off. In one fixed scene test Quality, Performance, then Quality
again for 30 seconds each. This separates the mode switch from time/scene
drift. Close and run Collect-PT-Primary-M12R65.cmd; send its ZIP and average
FPS for each segment. No PT-enable script needed. In-game gain unverified.
