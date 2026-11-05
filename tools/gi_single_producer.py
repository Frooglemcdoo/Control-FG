"""Move Owned's source-ray sampling into Control's existing GI dispatch slots."""
import re


def replace_once(source, old, new):
    if source.count(old) != 1:
        raise ValueError("Unexpected GI instruction: " + old)
    return source.replace(old, new)


def edit_export(source, name, edit):
    pattern = r'^define void @"[^"\n]*\?' + name + r'@@[^\n]*\n.*?^}'
    matches = list(re.finditer(pattern, source, re.M | re.S))
    if len(matches) != 1:
        raise ValueError("Unexpected GI export: " + name)
    match = matches[0]
    return source[:match.start()] + edit(match.group()) + source[match.end():]


def shared_hits(source):
    # The original and Owned raygens share the engine's hit table. Both write
    # the original slice index. Only an Owned surface ray removes the -1cm bias.
    def decode(body, index, address):
        old = f'  %{index} = load i32, i32* %{address}, align 4'
        return replace_once(body, old, old.replace(f'%{index} =', '%GITaggedSlice =') +
                            f'\n  %GIRegularSlice = and i32 %GITaggedSlice, 2147483647'
                            '\n  %GIFlags = lshr i32 %GITaggedSlice, 30'
                            '\n  %GIVisibility = icmp eq i32 %GIFlags, 3'
                            '\n  %GIHistorySlice = and i32 %GITaggedSlice, 63'
                            f'\n  %{index} = select i1 %GIVisibility, i32 %GIHistorySlice, i32 %GIRegularSlice')
    def closest(body):
        body = decode(body, 9, 8)
        old = '  %105 = fadd fast float %RayTCurrent, 0xBF847AE140000000'
        entry = '  %128 = call '
        guard = """  br i1 %GIVisibility, label %GIValidateEndpoint, label %GIPublish
GIValidateEndpoint:
  %GIShift = lshr i32 %GITaggedSlice, 6
  %GIMillimeters = and i32 %GIShift, 65535
  %GITarget = uitofp i32 %GIMillimeters to float
  %GIActual = fmul float %RayTCurrent, 1.000000e+03
  %GIError = fsub float %GIActual, %GITarget
  %GIUpper = fcmp ole float %GIError, 3.000000e+00
  %GILower = fcmp oge float %GIError, -3.000000e+00
  %GIReach = and i1 %GIUpper, %GILower
  %GIMatShift = lshr i32 %17, 8
  %GIMatMix = xor i32 %17, %GIMatShift
  %GIMatKey = and i32 %GIMatMix, 255
  %GIExpectedShift = lshr i32 %GITaggedSlice, 22
  %GIExpectedMat = and i32 %GIExpectedShift, 255
  %GISameMat = icmp eq i32 %GIMatKey, %GIExpectedMat
  %GIMatch = and i1 %GIReach, %GISameMat
  br i1 %GIMatch, label %GIPublish, label %GIExit
GIPublish:
"""
        body=replace_once(body,entry,guard+entry)
        body=replace_once(body,'  store i32 %9,','  br label %GIExit\nGIExit:\n  store i32 %9,')
        return replace_once(body, old, old.replace('%105', '%GINativeDistance') +
                            '\n  %GIOwnedHit = icmp slt i32 %GITaggedSlice, 0' +
                            '\n  %105 = select i1 %GIOwnedHit, float %RayTCurrent, float %GINativeDistance')
    def miss(body):
        body=decode(body,5,4)
        body=replace_once(body,'  %28 = call ', '  br i1 %GIVisibility, label %GIExit, label %GIPublish\nGIPublish:\n  %28 = call ')
        return replace_once(body,'  ret void','  br label %GIExit\nGIExit:\n  ret void')
    return edit_export(edit_export(source, 'closestHitMain', closest),
                       'missMain', miss)


def owned_raygen(source, variant):
    # Same orthonormal basis and cosine coordinates as gi_producer_proposals.
    # Use the existing hit/miss ABI, ray count, acceleration structure and origin.
    def patch(body):
        normals = ['%FMad61','%FMad59','%FMad57'] if variant == 0 else ['%FMad175','%FMad173','%FMad171']
        ux, uy, uz = ('%239','%240','%Sqrt9') if variant == 0 else ('%299','%300','%Sqrt56')
        ray = '%rayIndex.03' if variant == 0 else '%rayIndex2.038'
        directions = ('%.i0203','%.i1204','%.i2205') if variant == 0 else ('%.i0330','%.i1331','%.i2332')
        # Divide each angular stratum into four subcells across four frames.
        # Pixel and ray scramble the visitation order without consulting history.
        phase = """  %GITileSeed = xor i32 %GI307PixelX, %GI307PixelY
  %GITileKey = xor i32 %GITileSeed, %GI307RaySeed
  %GITilePhase = xor i32 %GI307Frame, %GITileKey
  %GITileX = and i32 %GITilePhase, 1
  %GITileShift = lshr i32 %GITilePhase, 1
  %GITileY = and i32 %GITileShift, 1
"""
        for axis, dim in [('X', 0), ('Y', 1)]:
            phase += (f'  %GITile{axis}F = uitofp i32 %GITile{axis} to float\n'
                      f'  %GITile{axis}Jitter = fadd float %GITile{axis}F, %GI307D{dim}Unit\n'
                      f'  %GITile{axis}Unit = fmul float %GITile{axis}Jitter, 5.000000e-01\n')
        old = '  %GI307U = fadd float %GI307CellXF, %GI307D0Unit'
        body = replace_once(body, old, phase + old.replace('%GI307D0Unit', '%GITileXUnit'))
        body = replace_once(body, 'fadd float %GI307CellYF, %GI307D1Unit',
                            'fadd float %GI307CellYF, %GITileYUnit')
        ir = []
        def op(name, instruction):
            result = '%GISingle' + name
            ir.append('  ' + result + ' = ' + instruction)
            return result
        def mul(name, a, b):
            return op(name, f'fmul float {a}, {b}')
        def add(name, a, b):
            return op(name, f'fadd float {a}, {b}')
        n2 = [mul('Sq'+str(i), n, n) for i,n in enumerate(normals)]
        inv = op('InvLen', 'call float @dx.op.unary.f32(i32 25, float ' +
                 add('Len2',add('XY',n2[0],n2[1]),n2[2]) + ')')
        x,y,z = [mul('N'+str(i),n,inv) for i,n in enumerate(normals)]
        positive = op('Positive', f'fcmp oge float {z}, 0.000000e+00')
        sign = op('Sign', f'select i1 {positive}, float 1.000000e+00, float -1.000000e+00')
        a = op('A','fdiv float -1.000000e+00, '+add('Den',sign,z))
        b = mul('B',mul('NxNy',x,y),a)
        negx = op('NegX',f'fsub float 0.000000e+00, {x}')
        negy = op('NegY',f'fsub float 0.000000e+00, {y}')
        tangent = [add('Tx','1.000000e+00',mul('Sxxa',mul('Sxx',mul('Sx',sign,x),x),a)),
                   mul('Ty',sign,b), mul('Tz',sign,negx)]
        bitangent = [b,add('By',sign,mul('Yya',mul('YY',y,y),a)),negy]
        direction = [add('Dir'+str(i),add('Plane'+str(i),mul('U'+str(i),ux,tangent[i]),
                         mul('V'+str(i),uy,bitangent[i])),mul('W'+str(i),uz,n))
                     for i,n in enumerate([x,y,z])]
        tagged = op('Slice',f'or i32 {ray}, -2147483648')
        body = replace_once(body,'  store i32 '+ray+',','\n'.join(ir)+'\n  store i32 '+tagged+',')
        old = ', '.join('float '+d for d in directions)
        return replace_once(body,old,', '.join('float '+d for d in direction))
    return edit_export(source,'rayGenMain',patch)
