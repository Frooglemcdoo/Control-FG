# R92: equal-area spotlight sampling

R91 RAW was noisier, while available screenshots did not establish restored
missing coverage. Its log reported RR selected at the filter hook. That field
comes from Selected() && !Stopped(); it is not proof of a completed RR evaluation.
We have not ruled out filtering or final reconstruction.

Code inspection found all four R89/R91 rays share one random radial distance.
Reflecting a disk sample across axes spreads angle but leaves radial sampling
fully correlated. An annular occluder can therefore block all four rays or none.

R92 changes only emitter sampling: four cyclic offsets of 1/4 in uniform
squared radius and angle. Each frame covers all four equal-area radial strata
and angular quadrants. Each ray retains a uniform disk marginal under uniform
native random inputs. The estimator still averages four visibility samples.
This is a variance-reduction experiment, not evidence that the missing rails
are caused by sampling. Arbitrary thin silhouettes can still be missed, and
there is no guaranteed variance improvement for every occluder shape.

Reference: PBRT, Stratified Sampler and Improving Efficiency:
https://www.pbr-book.org/4ed/Sampling_and_Reconstruction/Stratified_Sampler
https://www.pbr-book.org/4ed/Monte_Carlo_Integration/Improving_Efficiency

R89 mean-blocker softness, emitter radius, ray lengths, origin bias, native
caster mask, alpha handlers, point shadows, light selection, filters and
pure-RT composition stay unchanged. Four spotlight rays per pixel remain;
there is a small additional sampling arithmetic cost. No map merge, extra
render pass, GPU readback or synchronization is added.

Close Control to install every ZIP file beside Control_DX12.exe. Start with
RT Contact Shadows ON, RR OFF and FG OFF. At the same stationary camera near
the problematic light, use RT FILTERED first. Check missing character/rail
shadow edges, softness, flicker and frame rate. RAW is diagnostic and remains
noisy; it is not the normal playing mode. Then compare RR ON without moving.
F10 switches live; SAVE LOG produces one Lighting-R92 log for the session.

Validation: DXIL assembler/validator, numerical radial coverage and uniform
moments, unchanged captured point branch/hit handlers, package hash checks,
and Windows workflow shader/runtime checks. In-game appearance and GPU cost
still require the user's test.
