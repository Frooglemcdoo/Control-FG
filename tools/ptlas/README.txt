CONTROL PTLAS BACKEND VALIDATION - NOT A GAME MOD

Extract the entire folder anywhere and double-click Run-PTLAS-Validation.cmd.
It does not install files, load Control, change game settings or need admin.
Return PTLAS-Validation.txt after the program finishes.

This tests the actual NVIDIA Partitioned TLAS API on your GPU. The older captures
contain no PTLAS capability query or traversal results because that API was not
implemented in those builds. Keep using R392 in Control.

The validator builds a shared triangle BLAS once, traces seven conventional TLAS
scenes, releases each conventional TLAS after GPU completion, and only then
constructs PTLAS. It checks the same ray hits after movement, masking, removal,
slot reuse and an empty scene. Duplicate user IDs are intentional: instance
indices must remain correct. An unchanged PTLAS scene must not rebuild. Native
TLAS builds do not run during the PTLAS phase. GPU waits time out after 30 seconds.

This is a synthetic correctness test, not a benchmark or proof of in-game quality.
It does not yet test Control's material/SBT mapping, alpha-tested geometry,
multiple queues, native AS copy consumers or full native preparation retirement.
Mega Geometry support is queried but clustered geometry is not built by this test.
No in-game replacement is enabled until those remaining integrations are tested.

Requires a compatible NVIDIA GPU/driver and DXR 1.1 for the validation shader.
Unsupported hardware reports FAIL and does not change anything in the game.
NVIDIA NVAPI dependency is pinned to 70d337db9186e968eab622f7e786de7e437faf3d.
