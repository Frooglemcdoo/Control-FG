# R92 evidence and R93 correction

R92 package and runtime markers match. The user reports no visual improvement
and continuing light flicker. Last grid: 2040 samples, 1998 covered; this is
execution evidence, not proof of lighting fidelity. Foliage vertex-camera
pipelines are active, repeated reason-8 camera rejection is absent. Two logged
consumer rejections are missing-producer ordering, not camera rejection.
Root16 missing-argument failures persist (last throttled counter 3000).

The last light list has 9 points and 28 spots, environment scale zero. The
integrator uniformly proposes four lights with replacement at each hit. For a
single dominant light in a 37-light list, omission probability is (36/37)^4,
about 89.6 percent. Material-weighted selection cannot select a light that was
never proposed. This is a concrete variance mechanism, not proof it explains
all observed flicker.

R93 enumerates all primary candidates for lists up to 64. Each candidate keeps
proposal factor 1/N and reservoir count N, yielding denominator target/sum.
Thus the estimate remains unbiased with one deferred visibility test. Tests
cover every bright-light position at counts 1,2,37,64, colored shadowed energy,
and bounded fallback for secondary vertices / larger light lists. No history,
brightness, material fallback or baked-cache behavior is changed.

The substantive baked-light and finite-emitter feature gaps remain unresolved.
Do not describe this build as restoring the missing floor reflections or as
eliminating all flicker. Primary shading cost rises from 4 to N candidate
evaluations; secondary cost and visibility-ray count remain bounded as before.
