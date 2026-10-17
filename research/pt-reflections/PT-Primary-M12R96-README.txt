M12R96 - unified secondary material admission

Corrects a material rejection in the PT path, with RR either ON or OFF.
The captured native reflected-hit shader uses a common material approximation
without rejecting primary BRDF mode/scattering flags. Our path had applied
primary exclusions to secondary hits too, ending reflected/indirect paths.
Secondary hits now retain the common diffuse/specular continuation. Primary
specialized materials still fall back, and all value/buffer checks remain.

Windows GPU regression covers illuminated secondary hits for modes 0-3 and
nonzero translucency metadata, plus rejection of the corresponding unsupported
primary materials. Earlier emission/invalid-data cases remain covered.

This correction does not yet resolve GI-backed misses, secondary light-list
coverage, RR guide ownership, sampler binding failures or wall decal artifacts.
Missing floor reflections/baked lighting and flicker are NOT verified fixed.

Close Control and replace package files in the same installation. Keep settings;
do not rerun PT-Enable. Capture the affected floor using the R96 collector.
