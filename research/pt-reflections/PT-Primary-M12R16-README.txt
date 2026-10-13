M12R16 - retain scene resources and copy bounded vertex ranges

R15 progressed past the pending scene-order check but logged no PT submission
or GPU coverage. Scene metadata was missing. The logs independently confirm
native resource tracking reached its 1 GiB cap and geometry snapshots reached
their 512 MiB active-copy cap during level loading.

Native resource tracking now allows 4 GiB of existing buffer references. This
does not allocate 4 GiB of copies; native release observation still removes
each registry entry. Private geometry snapshots keep their 512 MiB limit.

Snapshots copy only the union of vertex byte ranges used by each BLAS build,
instead of entire vertex backing buffers. Native index allocations remain
complete for shader-table offsets and padded raw loads. Shared backing ranges
are merged, exact source ownership/state checks remain required, and private
vertex roots are rebased to the copied range. GPU retirement is unchanged.

WARP tests verify the reduced copy budget, merged ranges, nonzero source
offsets, rejected bounds, and bytes surviving later native buffer overwrites.
Missing-scene diagnostics distinguish unavailable TLAS and BLAS data.
The logs do not identify which missing allocation stopped each R15 attempt;
these changes address confirmed capacity failures. PT remains unconfirmed.

Close Control. Extract beside Control_DX12.exe, replacing included files.
Keep existing Streamline files and other settings. Load the entrance save
with native RT enabled. Note FPS with PT Reflections OFF, then use F10 ->
PT Reflections ON for 10 seconds and note FPS. Leave F11 unchanged.
Close the game, run Collect-PT-Primary-M12R16.cmd, and upload the results ZIP.
No screenshot is needed. Stop early if performance becomes unusable.
