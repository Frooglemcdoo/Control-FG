"""Fail-closed static audit of pinned GI producer geometry and randomized range."""
import re


def once(text, fragment):
    if text.count(fragment) != 1:
        raise ValueError('Unreviewed GI producer: ' + fragment)


def audit_source(source, variant):
    if variant not in (0, 1):
        raise ValueError('Unknown producer variant')
    hit = re.search(r'define void @"[^\n]*closestHitMain[^\n]*\n(.*?)^}', source, re.S | re.M)
    if hit is None:
        raise ValueError('Missing native closest hit')
    hit = hit.group(1)
    # Three packed signed vertex normals are barycentrically interpolated and
    # transformed with WorldToObject. No triangle-edge cross product feeds store.
    for fragment in (
        '%12 = extractelement <2 x float> %11, i32 0',
        '%14 = extractelement <2 x float> %11, i32 1',
        '%15 = fsub fast float %13, %14',
        '%.i0107 = fmul fast float %99, %15',
        '%.i0110 = fmul fast float %88, %12',
        '%.i0118 = fmul fast float %77, %14',
        '%tmp159 = fadd fast float %tmp158, %.i0107',
        '%tmp160 = fmul fast float %tmp159, 0x3F00002000000000',
        '%.i0126 = fmul fast float %tmp160, %WorldToObject64',
        '%.i0138 = fadd fast float %.i0132, %.i0135',
        'float %.i0138, float %.i1139, float %.i2140, float %tmp154, i8 15)',
        '%105 = fadd fast float %RayTCurrent, 0xBF847AE140000000',
        '%.i0141 = fmul fast float %105, %WorldRayDirection',
        '%.i0144 = fadd fast float %.i0141, %WorldRayOrigin',
    ):
        once(hit, fragment)
    mul, add, load, base, limit, noise = (242,243,244,245,246,'Frc27') if variant == 0 else (302,303,304,305,306,'Frc86')
    for fragment in (
        f'%{mul} = fmul fast float %{noise}, 0x3FD99999C0000000',
        f'%{add} = fadd fast float %{mul}, 0x3FE99999A0000000',
        f'%{load} = call %dx.types.CBufRet.f32 @dx.op.cbufferLoadLegacy.f32(i32 59, %dx.types.Handle %DiffuseGIConstants, i32 0)',
        f'%{base} = extractvalue %dx.types.CBufRet.f32 %{load}, 0',
        f'%{limit} = fmul fast float %{base}, %{add}',
    ):
        once(source, fragment)
    calls = [line for line in source.splitlines() if 'call void @dx.op.traceRay.struct.HitData' in line]
    if len(calls) != 1 or ', i32 0, i32 1, i32 0, i32 2, i32 0, float ' not in calls[0] or f', float %{limit}, %struct.HitData*' not in calls[0]:
        raise ValueError('Unreviewed TraceRay support/flags/table')
    return dict(variant=variant, hit_position='O + D * (RayTCurrent - 0.01), stored in view space',
                stored_normal='Barycentrically interpolated packed vertex normal transformed to world space; not a proven geometric face normal',
                tmax='DiffuseGIConstants[0].x * (0.8 + 0.4 * third_fractional_sequence_coordinate)',
                trace=dict(flags=0,instance_mask=1,hit_offset=0,geometry_stride=2,miss_index=0),
                uniform_independent_random_coordinates_verified=False,
                geometric_normal_available_in_captured_output=False,
                reuse_ready=False)
