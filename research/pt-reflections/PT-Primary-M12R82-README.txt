M12R82 - material-aware direct-light sampling

R81 capture confirms PT guide admission now works: 20 sampled submissions
accepted with guard_ok=1, no reconstruction setup failures or consumer rejections.
RR evaluations succeed. This does not prove all pixel coverage or image stability.

R82 corrects a variance weakness in the four-candidate light reservoir: selection
used incident luminance times cosine, ignoring material and outgoing direction.
Candidates now use unshadowed reflected luminance from the same diffuse+GGX BRDF
as final shading, in the same local frame. The selected light still receives one
visibility ray. Four candidates, path depth and shadow-ray counts are unchanged.
Visibility is not used to discard/retry candidates. Zero proposals still count,
and the estimator uses its selected target and complete proposal probability.

The shared-math regression compares both estimators with the exact light sum:
500,000 trials, glossy 16-light fixture, variance 7992 -> 2733 (~66% lower),
means 27.458/27.330 against exact 27.317. This is a controlled fixture, not a
prediction of in-game improvement. Existing sparse and occluded-light tests pass.
Windows compilation and GPU fixtures validate integration; game performance and
remaining flicker still need verification. Additional BRDF arithmetic may cost GPU
time even though ray counts are unchanged.

Missing lighting is not fixed: R81 still reports environment=0; finite-area and
emissive-light next-event sampling remain unsupported. Specular hit distance and
moving-object motion guidance also remain incomplete. No brightness multiplier,
new accumulation, weakened coverage gate or settings reset is introduced.

Close Control and replace ALL package files while preserving settings.
Do not rerun PT-Enable. Collect with Collect-PT-Primary-M12R82.cmd.
