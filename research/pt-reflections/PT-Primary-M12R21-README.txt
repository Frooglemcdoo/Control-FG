Control-FG Primary PT M12R21

R20 resolved 26 completed waits but still never armed PT. R21 rejects unresolved scene ordering BEFORE creating the full-resolution output and recording the private dispatch. It retains strict submission revalidation. This removes speculative GPU work on blocked frames; it does not establish that PT is working.
The PT_PRIMARY_PENDING_FENCE log identifies the remaining dependency with its exact required value, last observed queue signal, current real completion and queue lane. Actual fence completion is refreshed at scene preflight as well as submission.

Replace all files. Record RT/off FPS and PT/on FPS via F10 for 10 seconds, then run Collect-PT-Primary-M12R21.cmd and send the results ZIP. Full logs are included. In-game performance and PT rendering remain unverified.
