M12R83 - PT specular guide uses the actual primary viewing angle

R82's guide used abs(viewNormal.z), treating every pixel's view ray as the camera
forward axis. PT shading uses the actual perspective ray. These disagree off
center, so RR's specular reflectance guide can disagree with the radiance it
reconstructs as the camera moves.

R83 stores saturate(dot(shadingNormal, outgoingPrimaryRay)) in the previously
unused word at raw-packet byte 60. The RR guide reads it only with admitted,
covered, matching-material PT data. It uses that angle for the native EnvBRDF LUT
while preserving the actual native F0 (not PT diffuse albedo). Diffuse and angular
guides share admission/coverage checks. Invalid data retains native guides.
No extra rays, buffers, passes, or CPU/GPU waits. Packet stride stays 64 bytes.
PT-off and uncovered native guide behavior stay unchanged.

GPU test runs the actual guide shader through off/admitted/rejected/re-admitted
states, with an angular LUT and distinct PT angles, checking reflectance values,
black diffuse albedo, invalid-angle fallback and unchanged normals. The shared
transport fixture checks the primary cosine produced by the integrator.

R82 audit correction: PT_PRIMARY_FAULTS already counts primary rejection while
PT_PRIMARY_SECONDARY_TERMINATION separately counts retained-primary termination.
R83 labels the former stage=primary_rejection; no rejection policy is changed.

This does NOT complete all reported problems: per-pixel native fallback remains,
specular hit-distance/motion guidance remains absent, and missing environment/
area/emissive lighting is unresolved. In-game stability needs verification.

Close Control and replace ALL package files while preserving settings.
Do not rerun PT-Enable. Collect with Collect-PT-Primary-M12R83.cmd.
