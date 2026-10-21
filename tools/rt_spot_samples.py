"""Four symmetric, bounded spotlight segments for the hash-locked R65 shader.

The light disk, native alpha handlers, inclusion mask and point branch are
retained. Each ray measures its own segment from the native biased origin to
its sampled emitter position; the contact path's fNearDistance is not a limit.
"""
# Exactly representable float32: leave one part in 2**20 before the emitter.
# This keeps geometry behind the sample out without a scene-scale near cutoff.
ENDPOINT_FRACTION = 1.0 - 2.0**-20
ENDPOINT_FRACTION_IR = '0x3FEFFFFE00000000'
# Axis, world-space emitter center, native disk basis offsets, biased ray origin.
AXES = (
    ('x', '%192', '%.i0278', '%.i0284', '%.i0275'),
    ('y', '%194', '%.i1279', '%.i1285', '%.i1276'),
    ('z', '%196', '%.i2280', '%.i2286', '%.i2277'),
)
SAMPLE_SIGNS = ((1,1), (-1,1), (1,-1), (-1,-1))


def replace_once(source, before, after):
    if source.count(before) != 1:
        raise ValueError(f'Native spotlight instruction changed: {before}')
    return source.replace(before, after)


def segment_instructions(index, sx, sy):
    """Emit a sampled emitter-minus-origin vector, safe direction and TMax."""
    prefix = f'SpotSample{index}'
    lines = []
    for axis, emitter, dx, dy, origin in AXES:
        # Subtract before adding disk offsets to avoid rounding a small disk
        # offset away when the world-space emitter position is large.
        lines.extend([
            f'  %{prefix}{axis}Center = fsub fast float {emitter}, {origin}',
            f'  %{prefix}{axis}A = {"fadd" if sx>0 else "fsub"} fast float %{prefix}{axis}Center, {dx}',
            f'  %{prefix}{axis} = {"fadd" if sy>0 else "fsub"} fast float %{prefix}{axis}A, {dy}',
            f'  %{prefix}{axis}Squared = fmul fast float %{prefix}{axis}, %{prefix}{axis}'])
    lines.extend([
        f'  %{prefix}XY = fadd fast float %{prefix}xSquared, %{prefix}ySquared',
        f'  %{prefix}XYZ = fadd fast float %{prefix}XY, %{prefix}zSquared',
        f'  %{prefix}Length = call float @dx.op.unary.f32(i32 24, float %{prefix}XYZ)',
        f'  %{prefix}Nonzero = fcmp fast ogt float %{prefix}Length, 0.000000e+00',
        f'  %{prefix}Divisor = select i1 %{prefix}Nonzero, float %{prefix}Length, float 1.000000e+00',
        f'  %{prefix}Max = fmul fast float %{prefix}Length, {ENDPOINT_FRACTION_IR}'])
    for axis, *_ in AXES:
        lines.append(f'  %{prefix}{axis}Direction = fdiv fast float %{prefix}{axis}, %{prefix}Divisor')
    return lines


def visibility_instructions(hits):
    lines = []
    for index, hit in enumerate(hits):
        lines.extend([
            f'  %SpotOccluded{index} = fcmp fast olt float {hit}, %SpotSample{index}Max',
            f'  %SpotVisible{index} = select i1 %SpotOccluded{index}, float 0.000000e+00, float 1.000000e+00',
            # Miss lengths vary by sample. Exclude them from nearest-hit reduction.
            f'  %SpotBlocker{index} = select i1 %SpotOccluded{index}, float {hit}, float 5.000000e+04'])
    lines.extend([
        '  %SpotVisibleA = fadd fast float %SpotVisible0, %SpotVisible1',
        '  %SpotVisibleB = fadd fast float %SpotVisible2, %SpotVisible3',
        '  %SpotVisibleSum = fadd fast float %SpotVisibleA, %SpotVisibleB',
        '  %SpotOccludedA = or i1 %SpotOccluded0, %SpotOccluded1',
        '  %SpotOccludedB = or i1 %SpotOccluded2, %SpotOccluded3',
        '  %SpotNearestA = call float @dx.op.binary.f32(i32 36, float %SpotBlocker0, float %SpotBlocker1)',
        '  %SpotNearestB = call float @dx.op.binary.f32(i32 36, float %SpotBlocker2, float %SpotBlocker3)',
        '  %SpotNearest = call float @dx.op.binary.f32(i32 36, float %SpotNearestA, float %SpotNearestB)'])
    return lines


def four_spot_samples(source):
    source = replace_once(source, '  %215 = fmul fast float %214, %174',
        '  %215 = fmul fast float 2.000000e+00, %174')
    first_store = '  store float %FMin, float* %249, align 8'
    source = replace_once(source, first_store, '\n'.join(segment_instructions(0,1,1)) +
        '\n  store float %SpotSample0Max, float* %249, align 8')
    # Keep the original handle and payload; use closest-hit traversal as in R71.
    source = replace_once(source, 'i32 157, %dx.types.Handle %250, i32 4, i32 1',
        'i32 157, %dx.types.Handle %250, i32 0, i32 1')
    source = replace_once(source,
        'float %.i0293, float %.i1294, float %.i2295, float %FMin, %struct.ShadowHitData* nonnull %11)',
        'float %SpotSample0xDirection, float %SpotSample0yDirection, float %SpotSample0zDirection, float %SpotSample0Max, %struct.ShadowHitData* nonnull %11)')
    lines = []
    hits = ['%251']
    for index, (sx,sy) in enumerate(SAMPLE_SIGNS[1:],1):
        prefix = f'SpotSample{index}'
        lines.extend(segment_instructions(index,sx,sy))
        lines.extend([
            f'  store float %{prefix}Max, float* %249, align 8',
            f'  call void @dx.op.traceRay.struct.ShadowHitData(i32 157, %dx.types.Handle %250, i32 0, i32 1, i32 0, i32 2, i32 0, float %.i0275, float %.i1276, float %.i2277, float 0.000000e+00, float %{prefix}xDirection, float %{prefix}yDirection, float %{prefix}zDirection, float %{prefix}Max, %struct.ShadowHitData* nonnull %11)',
            f'  %{prefix}Hit = load float, float* %249, align 8'])
        hits.append(f'%{prefix}Hit')
    lines.extend(visibility_instructions(hits))
    anchor = '  %251 = load float, float* %249, align 8\n'
    source = replace_once(source, anchor, anchor+'\n'.join(lines)+'\n')
    for before, after in (
        ('%252 = fcmp fast olt float %251, %FMin', '%252 = or i1 %SpotOccludedA, %SpotOccludedB'),
        ('%253 = select i1 %252, float 0.000000e+00, float 1.000000e+00', '%253 = fmul fast float %SpotVisibleSum, 2.500000e-01'),
        ('%254 = fcmp fast olt float %251, 5.000000e+04', '%254 = fcmp fast olt float %SpotNearest, 5.000000e+04'),
        ('select i1 %255, float %251, float 5.000000e+04', 'select i1 %255, float %SpotNearest, float 5.000000e+04')):
        source = replace_once(source, before, after)
    return source
