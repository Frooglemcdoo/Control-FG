@echo off
setlocal
set "CONTROLTOH_GI_GEOMETRY=1"
echo R280 - native GI instance submission metadata.
call "%~dp0Start-GI-Ray-Trace.cmd"
endlocal
