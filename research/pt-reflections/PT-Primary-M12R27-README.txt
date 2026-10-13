Control-FG Primary PT M12R27

Close Control and replace all supplied files. Use F10 > PT Reflections.
Run Collect-PT-Primary-M12R27.cmd after testing and send its results ZIP.

R26 results: full PT dispatches produce valid output at 1975/2005 sampled
eligible pixels in the last record (98.5%). Light-validation faults are zero.
The remaining sampled faults are unsupported BRDFs, which retain native output.
This proves GPU output coverage, not visual correctness. A PT frame still took
64.4 ms, versus 15.3 ms in a later native frame; these are individual samples,
not a controlled benchmark.

R27 starts throughput-weighted Russian roulette after the first surface instead
of the third. It reduces low-contribution secondary paths while preserving the
expected radiance through reciprocal survival weighting. Four hits remain
reachable and rendering stays full resolution. Fewer surviving paths can increase
noise; this is a performance/variance tradeoff, not a measured FPS guarantee.

A retained GPU timestamp pair now measures the private dispatch, without CPU
waiting. The collector includes PT_PRIMARY_TIMING and uses the correct 64x64
coverage sampling description. The timer excludes other rendering work.

Validation: CPU statistical energy/work regression plus Windows compilation and
WARP timestamp/lifetime tests. In-game performance and image quality pending.
