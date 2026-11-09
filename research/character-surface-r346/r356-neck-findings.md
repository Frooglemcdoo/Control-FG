# R356: neck input exposure bracket

R355 capture Character-Surface-20261106-114518-30132-0 completed all six phases with skin/cloth/hair recorded replacement binds and effective clamp60. Near-history recorded application advanced. These counters are not GPU draw execution proof.

The neck grain is visible with the specular buffer removed. Diffuse removal darkens the character, and the gain4 diffuse input display is too bright for reliable fine-grain attribution. Neither establishes a confirmed upstream cause. Pose changes prevent pixelwise subtraction; JPEGs and downstream exposure/reconstruction remain confounders.

Audited skin-live consumer loads g_sGBufferReconstruction at half-resolution integer screen coordinates. Packed selectors and coverage index choose the source within a 2x2 footprint before both diffuse/specular loads. The diffuse load is multiplied by the character material and added to specular. Neighboring lighting texels cannot be assumed to belong to the same surface or to be valid selected samples. R356 preserves this instruction sequence exactly.

R356 replaces the four completed isolation phases with diffuse input gains1/64,1/16,1/4 and specular gain1/16. Mapping is gain*max(input,0)/(1+gain*max(input,0)). All are fixed, non-adaptive brackets,4–8 stops lower input gain than R355. Postprocessing still applies, so these are not rawHDR measurements or quantitative cross-exposure comparisons.

Gameplay HQ/R351 shader bytes, near-volume history, clamp60 and phase selection/restoration remain unchanged. Twelve full-screen JPEGs in one automatic TEST JESSE run. This is a capture correction, not a claimed rendering fix.

Validation: reversible bytecode edits on12 families, native output alpha/depth/coverage and bindings retained;192 emitted display arithmetic cases. Existing HQ spatial and near-history checks remain required; WARP PSO validation is shader validity, not real-game visual validation.
