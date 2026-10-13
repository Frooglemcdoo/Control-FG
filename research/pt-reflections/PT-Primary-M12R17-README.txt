M12R17 - validate only instances reachable by the PT ray mask

R16 logged zero resource-budget rejections, untracked destinations and geometry
copy failures. Its final snapshot usage was approximately 47 MiB. PT still
submitted no work: preparation stopped at scene_blas_unavailable.

The private primary and shadow rays both use inclusion mask 1. Scene selection
previously required geometry for every nonzero instance mask, including objects
these rays cannot traverse. Recording and strict submission selection now both
use mask 1. Original instance contributions and SBT row numbers are preserved.
Excluded instances cannot make unused native SBT rows reachable.

Missing BLAS ownership remains frozen as missing. Every included instance still
requires its original owner and exact geometry; registering an owner later
cannot repair a missing captured identity. Tests cover excluded missing builds,
excluded missing owners, objects becoming visible, and later registration.

Rejected build causes now survive queue-history compaction, distinguishing
missing ordered builds, unsupported geometry, missing owners, copies that fail,
and updates deliberately skipped while PT was off. R16 does not identify the
mask of its blocking instance; in-game PT rendering remains unconfirmed.

Close Control. Extract beside Control_DX12.exe, replacing included files.
Keep existing Streamline files and other settings. Load the entrance save
with native RT enabled. Note FPS with PT Reflections OFF, then use F10 ->
PT Reflections ON for 10 seconds and note FPS. Leave F11 unchanged.
Close the game, run Collect-PT-Primary-M12R17.cmd, and upload the results ZIP.
No screenshot is needed. Stop early if performance becomes unusable.
