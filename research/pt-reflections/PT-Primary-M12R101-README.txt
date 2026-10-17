M12R101 - native finite-radius light highlights

Restores the native point/spot light radius contribution to direct specular
highlights. Previously radius affected attenuation but the highlight used only
material roughness. The correction applies at primary surfaces and reflected
hits, including glossy floors and directly lit objects seen in reflections.
Light selection uses the same broadened lobe as final shading.

R100 authored diffuse hit lighting and earlier fixes remain. Texture/material
roughness, continuation sampling and RR guides are unchanged. This does not
add soft shadows, sampled emitter geometry or resolve all missing reflections.

Close Control and replace package files in the SAME installation. Keep settings;
do not rerun PT-Enable. Compare the affected floor and lights in reflections,
then run Collect-PT-Primary-M12R101.cmd and send the results ZIP.

Validation: numerical reference tests, Windows shader/build and WARP tests.
In-game visual confirmation remains pending.
