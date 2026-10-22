"""Emitter-bounded spotlight segments using the existing R94 disk samples.

Keep sampling, native biased origins and hit handlers. Replace the shared
contact-shadow cutoff with each sampled emitter's actual endpoint.
"""
from rt_spot_samples import replace_once

ENDPOINT_FRACTION = 1.0 - 2.0**-20
ENDPOINT_FRACTION_IR = '0x3FEFFFFE00000000'
# Axis, world emitter center, native basis X/Y, native biased origin.
AXES = (
    ('x', '%192', '%.0.i0', '%.027.i0', '%.i0275'),
    ('y', '%194', '%.027.i0', '%.027.i1', '%.i1276'),
    ('z', '%196', '%.0.i2', '%.027.i2', '%.i2277'),
)


def segment_instructions(index):
    prefix = f'SpotSegment{index}'
    lines = []
    for axis, emitter, basis_x, basis_y, origin in AXES:
        lines.extend([
            f'  %{prefix}{axis}Center = fsub fast float {emitter}, {origin}',
            f'  %{prefix}{axis}OffsetX = fmul fast float {basis_x}, %SpotDisk{index}X',
            f'  %{prefix}{axis}OffsetY = fmul fast float {basis_y}, %SpotDisk{index}Y',
            f'  %{prefix}{axis}A = fadd fast float %{prefix}{axis}Center, %{prefix}{axis}OffsetX',
            f'  %{prefix}{axis} = fadd fast float %{prefix}{axis}A, %{prefix}{axis}OffsetY',
            f'  %{prefix}{axis}Squared = fmul fast float %{prefix}{axis}, %{prefix}{axis}'])
    lines.extend([
        f'  %{prefix}XY = fadd fast float %{prefix}xSquared, %{prefix}ySquared',
        f'  %{prefix}XYZ = fadd fast float %{prefix}XY, %{prefix}zSquared',
        f'  %{prefix}Length = call float @dx.op.unary.f32(i32 24, float %{prefix}XYZ)',
        f'  %{prefix}Nonzero = fcmp fast ogt float %{prefix}Length, 0.000000e+00',
        f'  %{prefix}Divisor = select i1 %{prefix}Nonzero, float %{prefix}Length, float 1.000000e+00',
        f'  %{prefix}Max = fmul fast float %{prefix}Length, {ENDPOINT_FRACTION_IR}'])
    for axis, *_ in AXES:
        lines.append(f'  %{prefix}{axis}Normalized = fdiv fast float %{prefix}{axis}, %{prefix}Divisor')
        fallback = '1.000000e+00' if axis == 'z' else '0.000000e+00'
        lines.append(f'  %{prefix}{axis}Direction = select i1 %{prefix}Nonzero, float %{prefix}{axis}Normalized, float {fallback}')
    return lines


def emitter_bounded_segments(source):
    """Apply only after the four-sample R94 ray transform."""
    store = '  store float %SpotFullDistance, float* %249, align 8'
    if source.count(store) != 4:
        raise ValueError('Expected four R94 spotlight payload initializations')
    for i in range(4):
        prefix = f'SpotSegment{i}'
        source = source.replace(store, '\n'.join(segment_instructions(i)) +
            f'\n  store float %{prefix}Max, float* %249, align 8', 1)
        directions = ['%.i0293', '%.i1294', '%.i2295'] if i == 0 else [f'%SpotSample{i}{a}Direction' for a in 'xyz']
        before = ', '.join('float '+v for v in directions) + ', float %SpotFullDistance, %struct.ShadowHitData* nonnull %11)'
        after = ', '.join(f'float %{prefix}{a}Direction' for a in 'xyz') + f', float %{prefix}Max, %struct.ShadowHitData* nonnull %11)'
        source = replace_once(source, before, after)
        hit = '%251' if i == 0 else f'%SpotSample{i}Hit'
        source = replace_once(source,
            f'%SpotOccluded{i} = fcmp fast olt float {hit}, %SpotFullDistance',
            f'%SpotOccluded{i} = fcmp fast olt float {hit}, %{prefix}Max')
    # A miss on a short segment must not masquerade as a blocker on a longer one.
    # Mean distance already includes only samples marked occluded above.
    source = replace_once(source,
        '%252 = fcmp fast olt float %SpotNearest, %SpotFullDistance',
        '%252 = fcmp fast ogt float %SpotBlockerCountSum, 0.000000e+00')
    return replace_once(source,
        '%254 = fcmp fast olt float %SpotNearest, 5.000000e+04',
        '%254 = fcmp fast olt float %SpotMeanBlocker, 5.000000e+04')
