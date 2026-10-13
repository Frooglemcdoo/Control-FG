Control-FG Primary PT M12R20

R19 passed geometry preflight but every attempted PT dispatch remained disarmed: scene_dispatch_wait_pending. GPU coverage was zero. Its 20 FPS report is not proof of PT traversal.
R20 polls actual native fence completion without blocking. Already-satisfied waits lacking an observed queue signal can resolve (including initial values and CPU-signalled fences); known queue signal ancestry is preserved. Future values and unordered scene writes remain rejected. Logs include completed_waits.
Surface hooks now skip camera/descriptor work when the only recorded PT producers have already been rejected at submission.

Replace all files. Check RT/off FPS, enable PT through F10 for 10 seconds and note FPS. Run Collect-PT-Primary-M12R20.cmd and send the ZIP. In-game rendering and performance remain to be verified.
