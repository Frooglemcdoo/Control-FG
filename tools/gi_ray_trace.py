"""R278 complete-ray and no-reuse accounting probe: append stores, preserve every native GI6 instruction."""
import hashlib
import struct
from gi_spot_pcf import decoder, dst, src, ins
from dxbc_transform import transform_shex

CAPACITY = 65536
STRIDE = 256


def uint(v): return [0x4001, v]
def uav(mask=15): return [0x21e002 | (mask << 4), 7, 0]
def branch(value):
    code = ins('if', value)
    code[0] |= 0x40000
    return code


def generate(raw, path, identity, *, instrumentation_plan=None, roi=False, wide=False, screen=False, capacity=CAPACITY):
    assert identity['sha256'] == '1dfb4c9b054037e030ff9eed6ed5e482c8c32f9f301bb02c89eb0084c52842de'
    assert hashlib.sha256(raw).hexdigest() == identity['sha256']
    path.write_bytes(raw)
    decoded = decoder.decode(path)
    inst = {i['dword']: i for i in decoded['instructions']}
    expected = {
        661: ('ld', ['r[8].xyz', 'r[0].xyww', 't[1][1].yzwx']),
        698: ('dp3', ['r[4].x', 'r[6].xyzx', '-r[3].xyzx']),
        995: ('ld', ['r[7].w', 'r[0].xyzw', 't[19][24].yzwx']),
        1051: ('ld', ['r[13].xyzw', 'r[0].xyzw', 't[20][25].xyzw']),
        5029: ('mad', ['r[10].xyz', 'r[3].xyzx', 'r[15].xyzx', 'r[10].xyzx']),
        5136: ('mul', ['r[0].xyz', 'r[0].xxxx', 'r[10].xyzx']),
        5164: ('mov', ['r[1].w', 'l(1)']),
    }
    for offset, (name, operands) in expected.items():
        assert inst[offset]['name'] == name and inst[offset]['operands'] == operands
    n = struct.unpack_from('<I', raw, 28)[0]
    start = next(o+8 for o in struct.unpack_from('<'+'I'*n, raw, 32) if raw[o:o+4] == b'SHEX')
    size = struct.unpack_from('<I', raw, start-4)[0]
    words = list(struct.unpack_from('<'+'I'*(size//4), raw, start))
    td = next(i for i in inst if inst[i]['name'] == 'dcl_temps')
    temp = words[td+1]
    a, q, v, context, extra, fade, hit_position, hit_normal = range(temp, temp+8)
    surface_sum, fallback_sum = temp+8, temp+9
    inserts = {}
    def add(at, code): inserts.setdefault(at, []).extend(code)
    def store(offset, value, mask=15):
        return ins('iadd', dst(q,1), src(a,0), uint(offset)) + ins('store_raw', uav(mask), src(q,0), value)
    def begin(kind):
        # Dispatch coordinates survive the native register reuse at final output.
        code = ins('mov', dst(v,3), [0x20e46])
        if roi:
            # b0 space2 holds integer origin XY and extent XY, no native CB changes.
            code += ins('ineg',dst(q,3),[0x308e46,5,0,0])
            code += ins('iadd',dst(q,3),src(v),src(q))
            if screen:
                code += ins('and',dst(a,6),src(q,0x44),[0x308006,5,0,1])
                code += ins('ieq',dst(a,6),src(a),uint(0))
                code += ins('and',dst(a,2),src(a,0x55),src(a,0xaa))
            if wide:
                # Wide ROI: even/even pixels plus every pixel in the center 32x32.
                code += ins('and',dst(a,2),src(q,0),uint(1))+ins('and',dst(a,4),src(q,0x55),uint(1))
                code += ins('or',dst(a,2),src(a,0x55),src(a,0xaa))+ins('ieq',dst(a,2),src(a,0x55),uint(0))
                code += ins('iadd',dst(a,12),src(q,0x44),uint(0xffffffd0))
                code += ins('ult',dst(a,12),src(a),uint(32))
                code += ins('and',dst(a,4),src(a,0xaa),src(a,0xff))+ins('or',dst(a,2),src(a,0x55),src(a,0xaa))
            code += ins('ult',dst(q,3),src(q),[0x308ee6,5,0,0])
        else:
            code += ins('and', dst(q,3), src(v), uint(31))
            code += ins('ieq', dst(q,3), src(q), uint(16))
        code += ins('and', dst(q,1), src(q,0), src(q,0x55))
        if wide or screen:code += ins('and',dst(q,1),src(q,0),src(a,0x55))
        code += branch(src(q,0))
        code += ins('imm_atomic_iadd', dst(q,1), uav(1), uint(0), uint(1))
        code += ins('ult', dst(q,2), src(q,0), uint(capacity)) + branch(src(q,0x55))
        code += ins('ishl', dst(a,1), src(q,0), uint(8)) + ins('iadd', dst(a,1), src(a,0), uint(16))
        code += ins('mov', dst(v,4), src(0,0xaa) if kind == 1 else uint(0xffffffff))
        code += ins('mov', dst(v,8), uint(kind)) + store(0,src(v))
        # Never export undefined padding or stale records from an earlier use.
        for offset in range(16,256,16): code += store(offset,uint(0))
        return code
    def end(): return ins('endif') + ins('endif')
    add(661, ins('mov', dst(context,7), src(7)))
    add(698, ins('mov', dst(context,8), src(3,0xff)))
    # Preserve the actual dispatch count and depth-fade constants before the loop.
    initialize = ins('mov',dst(surface_sum,7),uint(0)) + ins('mov',dst(fallback_sum,7),uint(0))
    add(977, initialize + ins('mov', dst(extra,2), list(words[988:992])) +
        ins('mov', dst(fade,15), [0x308e46,1,1,17]))
    add(1003, ins('mov', dst(extra,8), src(7,0xff)))
    # Later hit-shading branches reuse r12/r13 in view space. Snapshot their
    # world-space values immediately after the hit loads, before that reuse.
    geometry = ins('mov',dst(hit_position,7),src(12)) + ins('mov',dst(hit_normal,7),src(13))
    add(1059,geometry)
    account = ins('ieq',dst(q,1),src(extra,0xff),uint(65535)) + branch(src(q,0))
    account += ins('mad',dst(fallback_sum,7),src(3),src(15),src(fallback_sum)) + ins('else')
    account += ins('mad',dst(surface_sum,7),src(3),src(15),src(surface_sum)) + ins('endif')
    ray = begin(1)
    ray += ins('mul', dst(v,7), src(3), src(15)) + store(16,src(v),7)
    ray += store(32,src(15),7) + store(48,src(3),7) + store(64,src(10),7)
    ray += store(80,src(1)) + store(96,src(context)) + store(112,src(hit_position),7)
    ray += ins('ult',dst(q,2),src(extra,0xff),uint(65534)) + branch(src(q,0x55))
    ray += store(128,src(hit_normal),7) + ins('endif')
    ray += ins('mov',dst(v,1),src(extra,0xff)) + ins('mov',dst(v,2),src(extra,0x55))
    ray += store(144,src(v),3) + store(160,src(fade)) + end()
    add(5038,account+ray) # after native accumulation, before ray index increment
    add(5136,ins('mov',dst(extra,1),src(0,0)))
    summary = begin(2)
    summary += store(96,src(context)) + store(148,src(extra,0x55),1)
    summary += store(160,src(fade)) + store(176,src(1)) + store(192,src(10),7)
    summary += store(208,src(extra,0),1)
    summary += ins('utof',dst(v,8),src(extra,0x55))
    for offset, accumulator in ((224,surface_sum),(240,fallback_sum)):
        summary += ins('mul',dst(v,7),src(extra,0),src(accumulator))
        summary += ins('div',dst(v,7),src(v),src(v,0xff)) + store(offset,src(v),7)
    summary += store(236,uint(0x278),1) + end()
    add(5169,summary) # valid-pixel branch only, after final output alpha assignment
    decl = ins('dcl_unordered_access_view_raw',[0x31ee46,7,0,0,2])
    if roi: decl += ins('dcl_constant_buffer',[0x308e46,5,0,0,2 if screen else 1,2])
    add(245,decl)
    def patch(w):
        out = w[:2]
        for i in range(2,len(w)):
            out += inserts.get(i,[])
            out.append(temp+10 if i == td+1 else w[i])
        w[:] = out
    if instrumentation_plan is not None:
        instrumentation_plan.update(words=words, inserts=inserts, temp=temp, temp_count_offset=td+1,
                                    begin=begin, end=end, store=store, scratch=v, context=context,
                                    extra=extra, fade=fade, hit_position=hit_position, hit_normal=hit_normal,
                                    initialize_code=initialize, account_code=account, ray_code=ray, summary_code=summary, geometry_code=geometry, declaration=decl)
    changed = transform_shex(raw,patch)
    def undo(w):
        out=w[:2]; cursor=2
        for i in range(2,len(words)):
            cursor += len(inserts.get(i,[]))
            out.append(temp if i == td+1 else w[cursor]); cursor += 1
        assert cursor == len(w)
        w[:] = out
    assert transform_shex(changed,undo) == raw
    path.write_bytes(changed)
    # A GPU fixture executes these exact append-only blocks. Sample 1 is a miss.
    def floats(*xs): return [0x4002]+[struct.unpack('<I',struct.pack('<f',x))[0] for x in xs]
    fixture = words[:2]+words[2:3]+decl+ins('dcl_input',[0x20032])+ins('dcl_temps',[temp+10])+ins('dcl_thread_group',[32,32,1])
    fixture += ins('mov',dst(context,15),floats(0,1,0,0)) + ins('mov',dst(context,8),uint(42))
    fixture += ins('mov',dst(extra,2),uint(2)) + ins('mov',dst(fade,15),floats(1,2,3,4))
    fixture += initialize
    for sample in range(2):
        fixture += ins('mov',dst(0,4),uint(sample))
        fixture += ins('mov',dst(extra,8),uint(65535 if sample else 7))
        fixture += ins('mov',dst(3,15),floats(.5,1,2,0)) + ins('mov',dst(15,15),floats(2,3,4,0))
        fixture += ins('mov',dst(10,15),floats(1*(sample+1),3*(sample+1),8*(sample+1),0))
        fixture += ins('mov',dst(1,15),floats(1,2,3,4)) + ins('mov',dst(12,15),floats(5,6,7,0))
        fixture += ins('mov',dst(13,15),floats(0,0,1,0)) + geometry
        fixture += ins('mov',dst(12,15),floats(-5,-6,-7,0)) + ins('mov',dst(13,15),floats(1,0,0,0)) + account + ray
    fixture += ins('mov',dst(1,15),floats(.75,2.25,6,1))
    fixture += ins('mov',dst(extra,1),[0x4001,0x3f400000]) + summary + ins('ret')
    def fixture_patch(w): w[:] = fixture
    (path.parent/'gi-ray-trace-oracle.dxbc').write_bytes(transform_shex(raw,fixture_patch))
    return changed, dict(schema='gi-ray-trace-r278',native_sha256=identity['sha256'],
        sha256=hashlib.sha256(changed).hexdigest(),capacity=CAPACITY,stride=STRIDE,
        sampling_step=32,native_instruction_recovery=True,ray_record_kind=1,summary_record_kind=2,no_reuse_accounting=True,accounting_marker=0x278,
        limitation='Consumer observations; not a producer PDF or temporal-variance capture.')
