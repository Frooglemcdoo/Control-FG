"""Offline prototype: native spotlight terms and complete GI rays in one stream.
Not selected by the game runtime. Uses existing sparse grid until ROI integration.
"""
import hashlib
from gi_ray_trace import generate as ray_generate, uint, CAPACITY, STRIDE
from gi_spot_pcf import decoder, dst, src, ins
from dxbc_transform import transform_shex


def generate(raw,path,identity):
    plan={};ray_generate(raw,path,identity,instrumentation_plan=plan)
    path.write_bytes(raw);decoded=decoder.decode(path);instructions={i['dword']:i for i in decoded['instructions']}
    expected={
        3869:('ld_structured',['r[20].xyzw','r[10].w','l(68)','t[11][11].xyzw']),
        4561:('ld_structured',['r[23].xyz','r[10].w','l(16)','t[11][11].xyzx']),
        4571:('ld_structured',['r[26].xyzw','r[10].w','l(48)','t[11][11].xyzw']),
        4634:('dp3',['r[18].w','r[23].xyzx','r[23].xyzx']),
        4648:('mad',['r[18].w','r[20].z','r[18].w','r[19].y']),
        4684:('max',['r[18].w','r[18].w','l(0)']),
        4982:('mad',['r[9].xyz','r[14].wwww','r[20].xyzx','r[9].xyzx']),
        5011:('mul',['r[15].xyz','r[17].xyzx','r[4].xyzx']),
    }
    for offset,(opcode,operands) in expected.items():
        if instructions[offset]['name']!=opcode or instructions[offset]['operands']!=operands:
            raise ValueError('native spotlight contract changed')
    words=plan['words'];inserts={k:list(v) for k,v in plan['inserts'].items()};temp=plan['temp'];v=plan['scratch']
    coefficients,position,color,attenuation=range(temp+10,temp+14)
    def add(at,code):inserts.setdefault(at,[]).extend(code)
    def after(at):return at+((words[at]>>24)&127)
    add(after(3869),ins('mov',dst(coefficients,15),src(20)))
    add(after(4561),ins('mov',dst(position,7),src(23)))
    add(after(4571),ins('mov',dst(color,15),src(26)))
    for offset,mask in [(4634,1),(4648,2),(4684,4)]:
        add(after(offset),ins('mov',dst(attenuation,mask),src(18,0xff)))
    store=plan['store'];code=plan['begin'](1)+store(12,uint(3),1)
    # Exact algebraic per-light term: gated RGB * secondary diffuse factor,
    # then secondary albedo and primary throughput. Normalize with paired summary.
    code+=ins('mul',dst(v,7),src(14,0xff),src(20))
    code+=ins('mul',dst(v,7),src(v),src(17))
    code+=ins('mul',dst(v,7),src(v),src(3))+store(16,src(v),7)
    code+=store(32,src(20),7)+store(48,src(3),7)+store(64,src(17),7)
    code+=store(80,src(position),7)+store(96,src(coefficients))+store(112,src(color))
    code+=store(128,src(attenuation),7)+store(140,src(14,0xff),1)
    code+=store(144,src(10,0xff),1)+store(148,src(plan['extra'],0x55),1)
    code+=store(152,src(plan['extra'],0xff),1)+store(156,src(plan['context'],0xff),1)
    code+=store(160,src(plan['hit_position']),7)+store(172,uint(0x281),1)+plan['end']()
    add(4982,code)
    def patch(w):
        out=w[:2]
        for i in range(2,len(words)):
            out+=inserts.get(i,[]);out.append(temp+14 if i==plan['temp_count_offset'] else w[i])
        w[:]=out
    changed=transform_shex(raw,patch)
    def undo(w):
        out=w[:2];cursor=2
        for i in range(2,len(words)):
            cursor+=len(inserts.get(i,[]));out.append(temp if i==plan['temp_count_offset'] else w[cursor]);cursor+=1
        if cursor!=len(w):raise ValueError('unaccounted shader instruction')
        w[:]=out
    if transform_shex(changed,undo)!=raw:raise ValueError('native recovery failed')
    path.write_bytes(changed);parsed=decoder.decode(path)
    return changed,dict(schema='gi-joint-light-prototype',sha256=hashlib.sha256(changed).hexdigest(),native_sha256=identity['sha256'],
                       native_instruction_recovery=True,decoded_instruction_count=len(parsed['instructions']),
                       capacity=CAPACITY,stride=STRIDE,spot_record_kind=3,sampling_step=32,
                       runtime_enabled=False,gpu_validated=False,
                       limits=['Only spot accumulation branches that execute are recorded; absence is not a measured zero.',
                               'Per-light multiplication is separately rounded, not a subtraction of native fused accumulation.',
                               'Light index is session-local; no named-lamp identification.',
                               'ROI selection and mixed-record parser remain required before deployment.'])
