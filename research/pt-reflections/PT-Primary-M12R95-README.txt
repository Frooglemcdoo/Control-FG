M12R95 - unit-color diffuse reconstruction correction

Corrects the custom reconstruction path used with Ray Reconstruction OFF.
Primary diffuse lighting already excludes texture color; reconstruction was
incorrectly dividing it by ray-hit texture albedo before filtering, then
multiplying that albedo back. That can introduce texture-shaped shading errors.
Diffuse now stays in its existing unit-color space throughout reconstruction.
The native raster material still applies its own final color once.

Regression coverage uses constant/noisy lighting with colored, black and
frame-varying ray-hit albedo. RR-on fresh input remains unchanged. R94 binding
and rejected-material diagnostics remain available.

This is NOT a verified fix for missing floor reflections, missing baked lighting,
or all texture flicker. RR-on guide/composition ownership remains unresolved.
Unsupported native shading models are not silently treated as generic opaque.

Close Control, replace package files, keep settings, do not rerun PT-Enable.
If evaluating this correction, use PT with RR OFF; RR ON bypasses this filter.
Collect results with Collect-PT-Primary-M12R95.cmd.
