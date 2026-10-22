"""Four equal-area radial strata for the hash-locked spotlight path.

Each ray is uniform over the disk across frames. Within a frame the four rays
cover separate equal-area annuli and angular quadrants. Native frame/pixel
random values drive both dimensions; ray count and filter contract stay fixed.
"""

def stratified_disk_samples(source):
    lines = []
    for index in range(4):
        prefix = f'SpotDisk{index}'
        offset = f'{index / 4:.6e}'
        lines.extend([
            f'  %{prefix}RadialShift = fadd fast float %Frc, {offset}',
            f'  %{prefix}Area = call float @dx.op.unary.f32(i32 22, float %{prefix}RadialShift)',
            f'  %{prefix}Radius = call float @dx.op.unary.f32(i32 24, float %{prefix}Area)',
            f'  %{prefix}AngularShift = fadd fast float %Frc48, {offset}',
            f'  %{prefix}Turn = call float @dx.op.unary.f32(i32 22, float %{prefix}AngularShift)',
            f'  %{prefix}Angle = fmul fast float %{prefix}Turn, 0x401921FB60000000',
            f'  %{prefix}Cos = call float @dx.op.unary.f32(i32 12, float %{prefix}Angle)',
            f'  %{prefix}Sin = call float @dx.op.unary.f32(i32 13, float %{prefix}Angle)',
            f'  %{prefix}ScaledRadius = fmul fast float %{prefix}Radius, %174',
            f'  %{prefix}X = fmul fast float %{prefix}ScaledRadius, %{prefix}Cos',
            f'  %{prefix}Y = fmul fast float %{prefix}ScaledRadius, %{prefix}Sin'])
    anchor = '  %.i0258 = fmul fast float %.030.i0, %215'
    source = replace_once(source, anchor, '\n'.join(lines) + '\n' +
                          '  %.i0258 = fmul fast float %SpotDisk0X, 1.000000e+00')
    return replace_once(source, '  %.i1259 = fmul fast float %.030.i1, %215',
                        '  %.i1259 = fmul fast float %SpotDisk0Y, 1.000000e+00')


def _four_spot_samples(source):
    # Full-distance shadows need a closest blocker, not traversal-order first hit.
    # Retain the R89 native IR anchor; stratified_disk_samples replaces its
    # consumers with unit-disk samples scaled directly by emitterRadius.
    radius = '  %215 = fmul fast float %214, %174'
    assert source.count(radius) == 1
    source = source.replace(radius, '  %215 = fmul fast float 2.000000e+00, %174')
    first_ray = 'i32 157, %dx.types.Handle %250, i32 4, i32 1'
    assert source.count(first_ray) == 1
    source = source.replace(first_ray, 'i32 157, %dx.types.Handle %250, i32 0, i32 1')
    anchor='  %251 = load float, float* %249, align 8\n'
    assert source.count(anchor)==1
    source = stratified_disk_samples(source)
    lines=[]
    hits=['%251']
    axes=[('x','%.i0234','%.0.i0','%.027.i0'),
          ('y','%.i1235','%.027.i0','%.027.i1'),
          ('z','%.i2236','%.0.i2','%.027.i2')]
    for index in range(1, 4):
        prefix=f'SpotSample{index}'
        for axis,center,basis_x,basis_y in axes:
            lines.append(f'  %{prefix}{axis}OffsetX = fmul fast float {basis_x}, %SpotDisk{index}X')
            lines.append(f'  %{prefix}{axis}OffsetY = fmul fast float {basis_y}, %SpotDisk{index}Y')
            lines.append(f'  %{prefix}{axis}A = fadd fast float {center}, %{prefix}{axis}OffsetX')
            lines.append(f'  %{prefix}{axis} = fadd fast float %{prefix}{axis}A, %{prefix}{axis}OffsetY')
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
