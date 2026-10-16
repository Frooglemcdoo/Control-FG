M12R93 - complete bounded primary light candidates

R92 is confirmed active in the supplied capture. The recurring camera-rejection
records are gone, but the user reports no restoration of missing lighting.

R93 replaces four random primary candidates with enumeration of every light
when the authenticated list contains 1..64 lights. The supplied run has up to
38 lights (last record: 9 points + 28 spots). Every light is considered for its
actual material response before reservoir selection. One visibility ray is
still traced. Zero/occluded candidates retain their correct normalization;
no post-shadow resampling or brightness multiplier is introduced.

Secondary vertices and larger lists retain four candidates. Primary candidate
shading costs more; shadow-ray count and path depth are unchanged. Performance
in Control needs measurement. Visibility selection, indirect paths and emitter
sampling can still flicker: this targets candidate-omission noise only.

Missing floor reflections and baked lighting are NOT declared fixed. Finite
area-light sampling and complete authored-light contribution remain absent.
Fourth-hit volume lighting is unchanged. This is not a complete lighting fix.

Close Control and replace all package files. Keep your current settings and do
not rerun PT-Enable. Use Collect-PT-Primary-M12R93.cmd after the same-scene test.
