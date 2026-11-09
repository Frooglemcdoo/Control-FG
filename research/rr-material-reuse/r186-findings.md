# R186 results

The user reports splatter remained in option 2 (other reflected lighting),
while option 3 (secondary point/spot specular only) was too dim to assess.
Do not label option 3 clean or infer zero local-highlight contribution.

Three uploaded Lighting-R186 logs were inspected. PID 41784 ends during startup
without a useful SPEC4/RR-on interval. PIDs 51384 and 63912 record effective
SPEC4 with RR selected during substantial portions of gameplay. Their toggles
also include RR-off/native-sample intervals, which are not the comparison.

These Lighting logs do not contain the SPEC_LOCAL_COMPONENT creation records;
those went to the separate probe log. Thus the visual attribution is conditional
on the selected isolation shader having applied. Do not claim the uploaded
logs prove the modes. R187 adds startup-mode and shader-creation counter reports
to Lighting heartbeat and SAVE LOG to close that evidence gap.

The result prioritizes the remainder (reflected diffuse/environment lighting)
over broadening secondary point/spot highlights. R183 already showed selected
painting emission-only regions were zero, though elevator emission outliers
remain a separate possibility. No user FPS result is available for R186.
R187 separates reflected diffuse from environment specular and boosts the latter
for visibility. This is narrower attribution, not a re-run of emission isolation.
