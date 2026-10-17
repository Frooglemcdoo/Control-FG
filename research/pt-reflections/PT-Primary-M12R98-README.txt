M12R98 - guarded global sampler replay for PT surface composition

R97's GI miss fallback is contributing in the supplied capture. Its last sparse
sample contains 176 miss requests and 176 positive cache results. R97 also logs
at least 4,200 rejected surface draws with missing global sampler parameter16,
including the general surface shader. These draws cannot consume the PT output.

R98 can explicitly replay the global sampler table on a reviewed PT draw when
its address was set on this exact command list, the root has not changed, and
the SAME sampler heap has remained continuously bound across resource-heap
changes. API table validity is not assumed: the actual setter is called before
the PT draw and again when restoring the native root. The table must pass full
64-descriptor bounds/alignment checks. A sampler heap change/unbind, root change,
Reset, absent setter or invalid address still rejects the draw. Ordinary compute
and graphics reads remain strict; unrelated draws are unchanged.

The new collector reports actual replayed PT draw bindings. The R97 logs do not
prove how many rejected draws meet these stricter recovery conditions, or that
this is the only cause of missing floor reflections. Windows checks cover both
eligible replay and the rejection boundaries. In-game verification is required.
R96 material and R97 GI corrections remain. Reflection lighting completeness,
RR motion inputs and remaining texture flicker are not declared fixed.

Close Control and replace package files in the same installation. Keep settings;
do not rerun PT-Enable. Compare the affected floor from the same camera with PT
OFF and ON, then run Collect-PT-Primary-M12R98.cmd and send the results ZIP.
