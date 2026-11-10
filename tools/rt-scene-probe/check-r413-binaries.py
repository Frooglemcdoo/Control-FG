"""Validate matched clean R413 modes and retained native scene selection."""
import sys
from pathlib import Path
native,owned=(Path(p).read_bytes() for p in sys.argv[1:3])
for data in (native,owned):
    for marker in (b'RT_ASSEMBLY_R411 active=',b'RT_SELECTION_R411 active=',b'RT_PREPARE_R411 active=',b'RT_RECORD_R411 active=',b'RT_BIND_R411 calls=',b'RT_SETUP_R411 sampled_calls=',b'RT_ALLOC_R411 class=',b'RT_FRAME_R411 present='):
        assert marker not in data, marker
    assert b'selection_comparison=R412 selection=native selection_hook=0' in data
    assert b'selection_comparison=R412 selection=owned' not in data
    assert b'RT_HEAP_R411_INSTALL ready=0 disabled=1' in data
    for phase in (b'RECORD',b'BIND'):
        assert b'RT_'+phase+b'_R411_INSTALL ready=%u' in data
assert b'preparation_comparison=R413 preparation=native preparation_hooks=0' in native
assert b'preparation_comparison=R413 preparation=owned' not in native
for phase in (b'ASSEMBLY',b'PREPARE'):
    assert b'RT_'+phase+b'_R411_INSTALL ready=0 disabled=1 reason=r413_native_preparation' in native
    assert b'RT_'+phase+b'_R411_INSTALL ready=%u' not in native
    assert b'RT_'+phase+b'_R411_INSTALL ready=%u' in owned
assert b'preparation_comparison=R413 preparation=owned' in owned
assert b'reason=r413_native_preparation' not in owned
assert native!=owned
print('PASS R413: both clean and native selection; native prep skips assembly/loop installs; owned prep retains both; recording/binding installs retained')
