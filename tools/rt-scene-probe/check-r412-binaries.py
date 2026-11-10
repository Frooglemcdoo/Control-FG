"""Check R412 uses clean binaries and identifies the isolated selection mode."""
import sys
from pathlib import Path
native,owned=(Path(p).read_bytes() for p in sys.argv[1:3])
for data in (native,owned):
    for marker in (b'RT_ASSEMBLY_R411 active=',b'RT_SELECTION_R411 active=',b'RT_PREPARE_R411 active=',b'RT_RECORD_R411 active=',b'RT_BIND_R411 calls=',b'RT_SETUP_R411 sampled_calls=',b'RT_ALLOC_R411 class=',b'RT_FRAME_R411 present='):
        assert marker not in data, marker
    assert b'RT_HEAP_R411_INSTALL ready=0 disabled=1' in data
    for phase in (b'ASSEMBLY',b'PREPARE',b'RECORD',b'BIND'):
        assert b'RT_'+phase+b'_R411_INSTALL ready=%u' in data
assert b'selection_comparison=R412 selection=native selection_hook=0' in native
assert b'selection_comparison=R412 selection=owned selection_hook=%u' not in native
assert b'reason=r412_native_selection' in native
assert b'selection_comparison=R412 selection=owned selection_hook=%u' in owned
assert b'reason=r412_native_selection' not in owned
assert native!=owned
print('PASS R412: both clean; native skips selection hook; owned retains selection install; other ownership markers retained')
