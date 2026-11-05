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
                            f'\n  %{index} = and i32 %GITaggedSlice, 2147483647')
    def closest(body):
        body = decode(body, 9, 8)
        old = '  %105 = fadd fast float %RayTCurrent, 0xBF847AE140000000'
        return replace_once(body, old, old.replace('%105', '%GINativeDistance') +
                            '\n  %GIOwnedHit = icmp slt i32 %GITaggedSlice, 0' +
                            '\n  %105 = select i1 %GIOwnedHit, float %RayTCurrent, float %GINativeDistance')
    return edit_export(edit_export(source, 'closestHitMain', closest),
                       'missMain', lambda body: decode(body, 5, 4))


def owned_raygen(source, variant):
    # Same orthonormal basis and cosine coordinates as gi_producer_proposals.
    # Use the existing hit/miss ABI, ray count, acceleration structure and origin.
    def patch(body):
        normals = ['%FMad61','%FMad59','%FMad57'] if variant == 0 else ['%FMad175','%FMad173','%FMad171']
        ux, uy, uz = ('%239','%240','%Sqrt9') if variant == 0 else ('%299','%300','%Sqrt56')
        ray = '%rayIndex.03' if variant == 0 else '%rayIndex2.038'
        directions = ('%.i0203','%.i1204','%.i2205') if variant == 0 else ('%.i0330','%.i1331','%.i2332')
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
