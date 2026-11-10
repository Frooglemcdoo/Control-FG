"""Verify the shipped clean binary lacks RT profiling code's report markers."""
import sys
from pathlib import Path
clean,diagnostic=(Path(p).read_bytes() for p in sys.argv[1:3])
periodic=[b'RT_ASSEMBLY_R411 active=',b'RT_SELECTION_R411 active=',b'RT_PREPARE_R411 active=',b'RT_RECORD_R411 active=',b'RT_BIND_R411 calls=',b'RT_SETUP_R411 sampled_calls=',b'RT_ALLOC_R411 class=',b'RT_FRAME_R411 present=']
for marker in periodic:
    assert marker not in clean,('clean contains diagnostic report',marker)
    assert marker in diagnostic,('diagnostic missing report',marker)
for data in (clean,diagnostic):
    assert b'RT_MODE_R411 diagnostics=' in data
    assert b'RT_HEAP_R411_INSTALL ready=0 disabled=1' in data
    assert b'RT_HEAP_R410 calls=' not in data
    for phase in (b'ASSEMBLY',b'SELECTION',b'PREPARE',b'RECORD',b'BIND'):
        assert b'RT_'+phase+b'_R411_INSTALL ready=' in data
assert clean!=diagnostic
print('PASS R411: clean excludes all RT periodic profiling reports; diagnostic includes all; both retain ownership installs and disable R410 tracking')
