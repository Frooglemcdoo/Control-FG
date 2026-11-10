# R409: separate allocator wait from protected work

R408 capture probe-20261110-021827-456-54124.log: after the first 64 setup
samples, 512 samples averaged 20.7871 us allocation, 2.8496 us sampler and
0.0391 us initialization (zero actual clears). Allocation was 87.799% of
measured setup. The 6,000 middle present intervals averaged 12.241409 ms:
five >=25 ms, one >=33.333 ms, none >=50 ms, maximum 40.507 ms.
Reported cumulative setup peak spans did not overlap the top intervals.
Sampling prevents excluding allocation as a dip cause.

Native D3D 1AA00 uses a recursive SRW lock at manager+18. It either advances
the current heap cursor or finds/creates a heap and refreshes its prefix.
R409 guards the full allocator bytes and installed D3D hash, then changes
only FF15 call sites 1AA67 (AcquireSRWLockExclusive) and 1AC9F
(ReleaseSRWLockExclusive) to E8 relay calls plus NOP. Import targets are
verified against kernel32 exports. Both relays are prepared before patching;
partial installation rolls back changed sites.

Only the existing 1/256 setup sample activates timing. Other calls forward
to the original Windows APIs without QPC reads. Before/after snapshots read
heap, generation, cursor and pool count while the allocator lock is held.
Counters distinguish normal cursor advancement, refill/change and other.
Recursive calls bypassing physical acquisition remain unobserved.
Wait/hold peak spans and engine interval timestamps use the same QPC clock.

No allocations are skipped, no heap lifetime changes, no larger reservations.
No FPS improvement is claimed. Wait includes scheduling delays; hold includes
sampled observer work; setup allocation time includes aggregate bookkeeping.
A pool-count increase is an observation, not proof of a driver allocation
duration. Cumulative peaks and sparse samples can miss the worst frame.

Validation covers portable phase timing, preserved API error state, protected
snapshots, passthrough and signed-relative-call bounds. Windows validation
executes original and patched six-byte call sites with real SRW locks and a
contending worker, checking lock release and the Win64 relay ABI.
