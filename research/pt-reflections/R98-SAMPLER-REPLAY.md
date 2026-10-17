# R98: explicit sampler replay at reviewed PT draws

R97 capture PT-Primary-M12R97-Results-20261017-075024-505.zip is internally
consistent with no collection warnings. Last sample frame6211: 2040 primary hits,
2021 integrated, 1958 covered, 63 primary rejects. GI miss requests176,
positive176; all terminal/miss GI positive501. No secondary terminations logged.
RR selected at3840x2160. Primary light snapshot10 point/28 spot/environment0.

15 throttled binding-failure records reach count4200. All parameter16 with
heap_change origin. Twelve logged records identify general surface7518cf8...,
three foliage68b09510.... These are logged records, not the full per-shader draw
counts. The prior snapshot discards every table when either heap changes.
The logs do not identify which heap changed, so eligibility of this correction
must be checked with R98 replay counters; no blanket recovery is attempted.

Microsoft requires explicit table definitions after heap changes:
https://learn.microsoft.com/en-us/windows/win32/direct3d12/setting-descriptor-heaps
https://microsoft.github.io/DirectX-Specs/d3d/ResourceBinding.html
R98 preserves that validity rule. It separately retains a replay candidate only
for graphics parameter16, only across calls keeping the exact same non-null
sampler heap continuously bound and the same root. The candidate comes from an
explicit setter in this recording; no default handle, heap-base inference,
cross-list copy, cross-frame cache or previous-address diagnostic is used.
Sampler change/unbind (including A->B->A), root change, invalid heap arguments,
Reset, and explicit replacement clear it. Same-root/same-heap redundant calls
follow existing semantics. Candidate handles undergo normal table bounds checks.

Only Drawing's reviewed-surface Read opts in. Ordinary reads still reject API
invalid state, and planning a replay never mutates the tracker. Successful
snapshot arguments feed existing BindGraphics, which sets the composed root and
all captured arguments, including an explicit SetGraphicsRootDescriptorTable(16).
Drawn restores the native root and explicitly replays again. Native heap/IA
state is unchanged. Failed downstream camera/descriptor/admission checks never
perform the candidate replay. A log is emitted only after the explicit binding,
not on candidate discovery. Submission guards continue to control PT ownership.

WARP tracker regression uses actual hooked command lists: strict read rejection,
opt-in plan, no planner mutation, same sampler across repeated resource changes,
interface alias, actual setter restores strict validity, compute/resource tables
remain rejected, sampler unbind/change-and-return, invalid bounds, root change,
and Reset. Existing composition GPU readback tests cover entry/exit root replay.
The build also runs the retained GI and material tests. This corrects an eligible
composition gap; it is not evidence that all missing reflections are repaired.
