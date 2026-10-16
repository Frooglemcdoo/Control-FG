M12R90 - shader-specific surface sampler requirement

Corrects the unconditional graphics root-16 requirement. Each companion PSO
checks all active shader stages using D3DReflect. Only shaders proven not to
use space-1 samplers may omit root 16; missing reflection keeps the strict check.
All captured native arguments are still restored. No dummy sampler bindings,
previous-frame camera reuse, or relaxed camera/depth matching is introduced.

Adds specific early-camera failure reasons and per-PSO required-root masks.
The new collector retains the camera diagnostics with the full log.

This is a targeted surface-admission correction, not a completed fix for missing
baked lighting or floor reflections. Terminal GI, lighting and reconstruction
are unchanged. Visual coverage improvements require in-game confirmation.

Close Control and replace all package files. Keep existing settings; do not
rerun PT-Enable. After gameplay run Collect-PT-Primary-M12R90.cmd.
