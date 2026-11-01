@echo off
setlocal
set "CONTROLTOH_GI_GEOMETRY=1"
echo R279 - read-only native ray-tracing geometry descriptor inventory.
call "%~dp0Start-GI-Ray-Trace.cmd"
endlocal
