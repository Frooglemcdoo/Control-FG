"""R281: native spotlight terms and complete GI rays in one dense ROI stream."""
import hashlib,struct
from gi_ray_trace import generate as ray_generate, uint, CAPACITY, STRIDE
from gi_spot_pcf import decoder, dst, src, ins
from dxbc_transform import transform_shex


def generate(raw,path,identity):
    plan={};seed=path.parent/'joint-baseline'/'seed.dxbc';seed.parent.mkdir(exist_ok=True)
    ray_generate(raw,seed,identity,instrumentation_plan=plan,roi=True)
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
    # GPU oracle runs the exact new stores and existing complete-ray/accounting blocks.
    def floats(*xs):return [0x4002]+[struct.unpack('<I',struct.pack('<f',x))[0] for x in xs]
    fixture=words[:2]+words[2:3]+plan['declaration']+ins('dcl_input',[0x20032])+ins('dcl_temps',[temp+14])+ins('dcl_thread_group',[32,32,1])
    fixture+=ins('mov',dst(plan['context'],15),floats(0,1,0,0))+ins('mov',dst(plan['context'],8),uint(42))
    fixture+=ins('mov',dst(plan['extra'],2),uint(2))+ins('mov',dst(plan['fade'],15),floats(1,2,3,4))+plan['initialize_code']
    for sample in range(2):
        fixture+=ins('mov',dst(0,4),uint(sample))+ins('mov',dst(plan['extra'],8),uint(65535 if sample else 7))
        fixture+=ins('mov',dst(3,15),floats(.5,1,2,0))+ins('mov',dst(15,15),floats(2,3,4,0))
        fixture+=ins('mov',dst(10,15),floats(1*(sample+1),3*(sample+1),8*(sample+1),0))
        fixture+=ins('mov',dst(1,15),floats(1,2,3,4))+ins('mov',dst(12,15),floats(5,6,7,0))+ins('mov',dst(13,15),floats(0,0,1,0))+plan['geometry_code']
        if not sample:
            fixture+=ins('mov',dst(10,8),uint(6))+ins('mov',dst(14,8),floats(.5,.5,.5,.5))
            fixture+=ins('mov',dst(20,7),floats(2,2,2,0))+ins('mov',dst(17,7),floats(1,1,1,0))
            fixture+=ins('mov',dst(coefficients,15),floats(.1,.2,.3,0))+ins('mov',dst(position,7),floats(10,11,12,0))
            fixture+=ins('mov',dst(color,15),floats(2,3,4,5))+ins('mov',dst(attenuation,7),floats(6,7,8,0))+code
        fixture+=plan['account_code']+plan['ray_code']
    fixture+=ins('mov',dst(1,15),floats(.75,2.25,6,1))+ins('mov',dst(plan['extra'],1),[0x4001,0x3f400000])+plan['summary_code']+ins('ret')
    def fixture_patch(w):w[:]=fixture
    (path.parent/'gi-joint-light-oracle.dxbc').write_bytes(transform_shex(raw,fixture_patch))
    path.write_bytes(changed);parsed=decoder.decode(path)
    return changed,dict(schema='gi-joint-light-r281',sha256=hashlib.sha256(changed).hexdigest(),native_sha256=identity['sha256'],
                       native_instruction_recovery=True,decoded_instruction_count=len(parsed['instructions']),
                       capacity=CAPACITY,stride=STRIDE,spot_record_kind=3,sampling_step=1,roi_extent=[32,32],
                       runtime_enabled=False,gpu_validated=False,
                       limits=['Only spot accumulation branches that execute are recorded; absence is not a measured zero.',
                               'Per-light multiplication is separately rounded, not a subtraction of native fused accumulation.',
                               'Light index is session-local; no named-lamp identification.',
                               'GPU validation and runtime selection must be verified before delivery.'])
