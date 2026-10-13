Control-FG Primary PT M12R22 - scheduling diagnostic

R21 identifies a real future fence wait: required N, last observed signal N-1, actual completion N-1. The next signal has not been submitted at PT recording time. This build does not bypass that dependency or claim working PT.
Blocked frames now skip descriptor and private pipeline preparation as well as GPU output allocation. A bounded 64-event queue trace after the first failure (and every 600th) records actual native signal/wait/execute order, queue type and fence identity, allowing the producer queue to be correlated directly. No CPU/GPU drain or blocking wait is added.

Replace all files. Enable PT via F10 for 10 seconds, report off/on FPS, run Collect-PT-Primary-M12R22.cmd and send its ZIP. The trace is in the included full probe log. Rendering and FPS recovery remain unverified.
