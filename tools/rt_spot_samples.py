"""Four symmetric disk samples for the hash-locked R65 spotlight path.

Preserve the disk distribution, alpha handlers, masks and point
branch. Reflect the native uniform disk sample across both axes; each marginal
remains uniform, while a pixel covers all four quadrants in the same frame.
"""
def _four_spot_samples(source):
    # Full-distance shadows need a closest blocker, not traversal-order first hit.
    # The native disk has radius 0.5; use diameter 2*emitterRadius to match the
    # spatial filter, instead of the randomized short-contact radius multiplier.
    radius = '  %215 = fmul fast float %214, %174'
    assert source.count(radius) == 1
    source = source.replace(radius, '  %215 = fmul fast float 2.000000e+00, %174')
    first_ray = 'i32 157, %dx.types.Handle %250, i32 4, i32 1'
    assert source.count(first_ray) == 1
    source = source.replace(first_ray, 'i32 157, %dx.types.Handle %250, i32 0, i32 1')
    anchor='  %251 = load float, float* %249, align 8\n'
    assert source.count(anchor)==1
    lines=[]
    hits=['%251']
    axes=[('x','%.i0234','%.i0278','%.i0284','%.i0275'),
          ('y','%.i1235','%.i1279','%.i1285','%.i1276'),
          ('z','%.i2236','%.i2280','%.i2286','%.i2277')]
    for index,(sx,sy) in enumerate([(-1,1),(1,-1),(-1,-1)],1):
        prefix=f'SpotSample{index}'
        for axis,center,dx,dy,_ in axes:
            lines.append(f'  %{prefix}{axis}A = {"fadd" if sx>0 else "fsub"} fast float {center}, {dx}')
            lines.append(f'  %{prefix}{axis} = {"fadd" if sy>0 else "fsub"} fast float %{prefix}{axis}A, {dy}')
            lines.append(f'  %{prefix}{axis}Squared = fmul fast float %{prefix}{axis}, %{prefix}{axis}')
        lines.extend([
            f'  %{prefix}XY = fadd fast float %{prefix}xSquared, %{prefix}ySquared',
            f'  %{prefix}XYZ = fadd fast float %{prefix}XY, %{prefix}zSquared',
            f'  %{prefix}Length = call float @dx.op.unary.f32(i32 24, float %{prefix}XYZ)'])
        for axis,*_ in axes:
            lines.append(f'  %{prefix}{axis}Direction = fdiv fast float %{prefix}{axis}, %{prefix}Length')
        lines.extend([
            '  store float %SpotFullDistance, float* %249, align 8',
            f'  call void @dx.op.traceRay.struct.ShadowHitData(i32 157, %dx.types.Handle %250, i32 0, i32 1, i32 0, i32 2, i32 0, float %.i0275, float %.i1276, float %.i2277, float 0.000000e+00, float %{prefix}xDirection, float %{prefix}yDirection, float %{prefix}zDirection, float %SpotFullDistance, %struct.ShadowHitData* nonnull %11)',
            f'  %{prefix}Hit = load float, float* %249, align 8'])
        hits.append(f'%{prefix}Hit')
    for index,hit in enumerate(hits):
        lines.extend([f'  %SpotOccluded{index} = fcmp fast olt float {hit}, %SpotFullDistance',
                      f'  %SpotVisible{index} = select i1 %SpotOccluded{index}, float 0.000000e+00, float 1.000000e+00'])
    lines.extend([
        '  %SpotVisibleA = fadd fast float %SpotVisible0, %SpotVisible1',
        '  %SpotVisibleB = fadd fast float %SpotVisible2, %SpotVisible3',
        '  %SpotVisibleSum = fadd fast float %SpotVisibleA, %SpotVisibleB',
        '  %SpotNearestA = call float @dx.op.binary.f32(i32 36, float %251, float %SpotSample1Hit)',
        '  %SpotNearestB = call float @dx.op.binary.f32(i32 36, float %SpotSample2Hit, float %SpotSample3Hit)',
        '  %SpotNearest = call float @dx.op.binary.f32(i32 36, float %SpotNearestA, float %SpotNearestB)'])
    # Estimate softness from actual blockers only. A single near hit must not
    # collapse the entire penumbra; misses never enter the distance average.
    for index, hit in enumerate(hits):
        lines.extend([
            f'  %SpotBlockerDistance{index} = select i1 %SpotOccluded{index}, float {hit}, float 0.000000e+00',
            f'  %SpotBlockerCount{index} = select i1 %SpotOccluded{index}, float 1.000000e+00, float 0.000000e+00'])
    for label in ('Distance', 'Count'):
        lines.extend([
            f'  %SpotBlocker{label}A = fadd fast float %SpotBlocker{label}0, %SpotBlocker{label}1',
            f'  %SpotBlocker{label}B = fadd fast float %SpotBlocker{label}2, %SpotBlocker{label}3',
            f'  %SpotBlocker{label}Sum = fadd fast float %SpotBlocker{label}A, %SpotBlocker{label}B'])
    lines.extend([
        '  %SpotSafeBlockerCount = call float @dx.op.binary.f32(i32 35, float %SpotBlockerCountSum, float 1.000000e+00)',
        '  %SpotMeanBlocker = fdiv fast float %SpotBlockerDistanceSum, %SpotSafeBlockerCount'])
    source=source.replace(anchor,anchor+'\n'.join(lines)+'\n')
    for before,after in [
        ('%252 = fcmp fast olt float %251, %SpotFullDistance','%252 = fcmp fast olt float %SpotNearest, %SpotFullDistance'),
        ('%253 = select i1 %252, float 0.000000e+00, float 1.000000e+00','%253 = fmul fast float %SpotVisibleSum, 2.500000e-01'),
        ('%254 = fcmp fast olt float %251, 5.000000e+04','%254 = fcmp fast olt float %SpotNearest, 5.000000e+04'),
        ('select i1 %255, float %251, float 5.000000e+04','select i1 %255, float %SpotMeanBlocker, float 5.000000e+04')]:
        assert source.count(before)==1,before
        source=source.replace(before,after)
    return source


def replace_once(source, before, after):
    if source.count(before) != 1:
        raise ValueError(f'Native spotlight instruction changed: {before}')
    return source.replace(before, after)


def full_distance_limit(source, single):
    """R71 light range contract: retain the light's near-plane exclusion."""
    distance, radius_step, pointer, payload, hit, test = (
        (207, 212, 249, 11, 251, 252) if single else (210, 233, 213, 12, 257, 258))
    anchor = f'  %{radius_step} = fadd fast float %FMin, -5.000000e-01'
    source = replace_once(source, anchor,
        f'  %SpotFullDistance = call float @dx.op.binary.f32(i32 35, float %{distance}, float 0.000000e+00)\n' + anchor)
    for before, after in (
        (f'store float %FMin, float* %{pointer}', f'store float %SpotFullDistance, float* %{pointer}'),
        (f'float %FMin, %struct.ShadowHitData* nonnull %{payload})',
         f'float %SpotFullDistance, %struct.ShadowHitData* nonnull %{payload})'),
        (f'%{test} = fcmp fast olt float %{hit}, %FMin',
         f'%{test} = fcmp fast olt float %{hit}, %SpotFullDistance')):
        source = replace_once(source, before, after)
    return source


def four_spot_samples(source):
    return _four_spot_samples(full_distance_limit(source, True))
