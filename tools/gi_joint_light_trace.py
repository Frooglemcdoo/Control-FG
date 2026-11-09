"""R285: native spotlight terms and complete GI rays in one dense ROI stream."""
import hashlib,struct
from gi_ray_trace import generate as ray_generate, uint, CAPACITY, STRIDE
from gi_spot_pcf import decoder, dst, src, ins
from dxbc_transform import transform_shex


def generate(raw,path,identity):
    plan={};seed=path.parent/'joint-baseline'/'seed.dxbc';seed.parent.mkdir(exist_ok=True)
    ray_generate(raw,seed,identity,instrumentation_plan=plan,roi=True,screen=True,capacity=1048576)
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
    # R282 records every visited spotlight, including projection/visibility rejects.
    # Private snapshots are reset at each light iteration; validity marks executed
    # stages. No native register, branch, texture lookup or lighting math changes.
    projection, visibility, response, cookie, depth, metadata, normal, direction, cookie_uv, pre_cookie, post_cookie, gated = range(temp+14,temp+26)
    def mark(bit):return ins('or',dst(cookie,8),src(cookie,0xff),uint(bit))
    reset=[]
    for reg in range(temp+14,temp+26):reset+=ins('mov',dst(reg,15),uint(0))
    reset+=ins('mov',dst(cookie,7),[0x4002,0x3f800000,0x3f800000,0x3f800000,0])
    add(3869,reset)
    # Each site is additionally protected by the complete native SHA contract.
    stages={
        4022:ins('mov',dst(projection,8),src(14,0xff)),
        4049:ins('mov',dst(projection,7),src(22))+mark(1),
        4078:mark(2),
        4128:ins('mov',dst(metadata,1),src(17,0xff)),
        4318:ins('mov',dst(depth,3),src(25,0xcc))+ins('mov',dst(depth,4),src(21,0xaa)),
        4331:ins('mov',dst(depth,8),src(19,0x55))+mark(32),
        4289:mark(64),
        4479:mark(128),
        4487:ins('mov',dst(visibility,1),src(18,0xff))+ins('mov',dst(visibility,2),src(19,0x55))+ins('mov',dst(visibility,4),src(16,0xff)),
        4551:ins('mov',dst(visibility,8),src(15,0xff))+mark(4),
        4691:ins('mov',dst(response,1),src(16,0xff)),
        4705:ins('mov',dst(pre_cookie,7),src(20)),
        4797:ins('mov',dst(cookie_uv,3),src(21)),
        4810:ins('mov',dst(cookie,7),src(21))+mark(16),
        4818:ins('mov',dst(post_cookie,7),src(20)),
        4832:ins('mov',dst(response,8),src(14,0xff)),
        4846:ins('mov',dst(gated,7),src(20)),
        4915:ins('mov',dst(response,2),src(15,0xff)),
        4982:ins('mov',dst(response,4),src(14,0xff))+ins('mov',dst(normal,7),src(13))+ins('mov',dst(direction,7),src(22))+mark(8),
    }
    for at,block in stages.items():
        if at not in instructions:raise ValueError('stage site is not an instruction boundary')
        add(at,block)
    store=plan['store']
    candidate=plan['begin'](1)+store(12,uint(4),1)
    for offset,reg in [(16,projection),(32,visibility),(48,response),(64,cookie),(80,depth),(112,normal),(128,direction),(144,cookie_uv),(192,pre_cookie),(208,post_cookie),(224,gated)]:candidate+=store(offset,src(reg))
    candidate+=store(96,src(metadata,0),1)+store(100,src(10,0xff),1)
    candidate+=store(104,src(plan['extra'],0xff),1)+store(108,src(plan['context'],0xff),1)
    candidate+=store(160,src(plan['hit_position']),7)+store(176,uint(0x282),1)+store(180,src(plan['extra'],0x55),1)+plan['end']()
    add(4993,candidate)
    # R285: measure the native volume lookup on both hit and miss rays.
    lookup, lookup_normal, ray_direction, volume_sum, volume_mean, volume_local, brick_uv = range(temp+26,temp+33)
    volume_stages={
        1093:ins('mov',dst(ray_direction,7),src(14)),
        1122:ins('mov',dst(lookup,7),src(4))+ins('mov',dst(lookup_normal,7),src(15)),
        1192:ins('mov',dst(volume_local,7),src(17)),
        1866:ins('mov',dst(brick_uv,7),src(17)),
        2234:ins('mov',dst(volume_sum,7),src(16))+ins('mov',dst(volume_sum,8),src(12,0xff)),
        2250:ins('mov',dst(volume_mean,7),src(4)),
    }
    for at,block in volume_stages.items():
        if at not in instructions:raise ValueError('volume stage not an instruction boundary')
        add(at,block)
    volume=plan['begin'](1)+store(12,uint(6),1)
    volume+=store(16,src(volume_local),7)+store(32,src(brick_uv),7)+store(48,src(17))
    volume+=store(64,src(16),7)+store(76,src(12,0xff),1)
    volume+=store(80,src(14,0xff),1)+store(84,src(23,0),1)+store(88,src(23,0xaa),1)
    volume+=store(96,src(plan['context'],0xff),1)+store(100,src(plan['extra'],0x55),1)+store(104,src(plan['extra'],0xff),1)+store(108,uint(0x283),1)+plan['end']()
    add(2218,volume) # after native weighted RGB and weight accumulation, inside volume bounds
    volume_summary=plan['begin'](1)+store(12,uint(5),1)
    for offset,reg,mask in [(16,lookup,7),(32,lookup_normal,7),(48,ray_direction,7),(64,volume_sum,15),(80,volume_mean,7),(112,4,7),(128,3,15)]:volume_summary+=store(offset,src(reg),mask)
    volume_summary+=store(96,[0x308e46,2,2,1],7)
    volume_summary+=store(144,src(plan['context'],0xff),1)+store(148,src(plan['extra'],0x55),1)+store(152,src(plan['extra'],0xff),1)+store(156,uint(0x283),1)
    volume_summary+=store(160,[0x308ff6,2,2,0],1)+plan['end']()
    add(2259,volume_summary)
    code=plan['begin'](1)+store(12,uint(3),1)
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
    point_meta,point_color,point_atten,point_response,point_shadow,point_moments,point_before_shadow,point_position=range(temp+35,temp+43)
    point_reset=[]
    for reg in range(temp+35,temp+43):point_reset+=ins('mov',dst(reg,15),uint(0))
    point_reset+=ins('mov',dst(point_response,8),[0x4001,0x3f800000])
    add(2826,point_reset)
    point_stages={
        2836:ins('mov',dst(point_position,15),src(19)),
        2846:ins('mov',dst(point_color,15),src(20)),
        2886:ins('mov',dst(point_meta,1),src(16,0xff))+ins('mov',dst(point_atten,8),src(15,0xff)),
        2901:ins('mov',dst(point_atten,1),src(17,0xff)),
        2963:ins('mov',dst(point_response,1),src(20,0xff)),
        2972:ins('mov',dst(point_atten,2),src(15,0xff)),
        3006:ins('mov',dst(point_atten,4),src(15,0xff)),
        3071:ins('mov',dst(point_response,2),src(15,0xff)),
        3080:ins('mov',dst(point_before_shadow,7),src(20)),
        3287:ins('mov',dst(point_meta,2),src(15,0xff)),
        3533:ins('mov',dst(point_shadow,3),src(21)),
        3546:ins('mov',dst(point_moments,3),src(21))+ins('mov',dst(point_meta,4),uint(1)),
        3560:ins('mov',dst(point_shadow,4),src(16,0xff)),
        3577:ins('mov',dst(point_moments,4),src(17,0xff)),
        3626:ins('mov',dst(point_response,8),src(15,0xff)),
        3729:ins('mov',dst(point_response,4),src(16,0xff)),
    }
    for at,block in point_stages.items():add(at,block)
    point_sum, surface_volume = temp+33,temp+34
    add(3775,ins('mov',dst(point_sum,7),src(14)))
    add(4999,ins('mov',dst(surface_volume,7),src(4)))
    point=plan['begin'](1)+store(12,uint(7),1)
    point+=ins('mul',dst(v,7),src(15,0xff),src(20))+ins('mul',dst(v,7),src(v),src(17))+ins('mul',dst(v,7),src(v),src(3))+store(16,src(v),7)
    point+=store(32,src(20),7)+store(48,src(3),7)+store(64,src(17),7)+store(80,src(15,0xff),1)
    point+=store(96,src(14,0xff),1)+store(100,src(plan['extra'],0x55),1)+store(104,src(plan['extra'],0xff),1)+store(108,src(plan['context'],0xff),1)+store(112,uint(0x284),1)+plan['end']()
    point_detail=plan['begin'](1)+store(12,uint(9),1)
    for offset,reg in [(16,point_position),(32,point_color),(48,point_atten),(64,point_response),(80,point_shadow),(96,point_moments),(112,point_before_shadow),(128,20),(144,point_meta)]:point_detail+=store(offset,src(reg),15 if offset in (16,32,48,64,144) else 7)
    point_detail+=store(160,src(14,0xff),1)+store(164,src(plan['extra'],0x55),1)+store(168,src(plan['extra'],0xff),1)+store(172,src(plan['context'],0xff),1)+store(176,uint(0x285),1)+store(180,src(15,0xff),1)+plan['end']()
    add(3743,point_detail)
    add(3743,point)
    surface=plan['begin'](1)+store(12,uint(8),1)
    for offset,reg in [(16,surface_volume),(32,point_sum),(48,9),(64,4),(80,17),(96,3)]:surface+=store(offset,src(reg),7)
    surface+=store(112,src(plan['extra'],0x55),1)+store(116,src(plan['extra'],0xff),1)+store(120,src(plan['context'],0xff),1)+store(124,uint(0x284),1)+plan['end']()
    add(5011,surface)
    def patch(w):
        out=w[:2]
        for i in range(2,len(words)):
            out+=inserts.get(i,[]);out.append(temp+43 if i==plan['temp_count_offset'] else w[i])
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
    fixture=words[:2]+words[2:3]+plan['declaration']+ins('dcl_constant_buffer',[0x308e46,2,2,2,2,0])+ins('dcl_input',[0x20032])+ins('dcl_temps',[temp+43])+ins('dcl_thread_group',[32,32,1])
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
            fixture+=reset
            fixture_inputs={
                4022:[(14,8,(2,2,2,2))],4049:[(22,7,(.25,.75,.5,0))],4078:[],
                4128:[],4318:[(25,15,(.25,0,0,.75)),(21,4,(.5,.5,.5,.5))],
                4331:[(19,2,(.6,.6,.6,.6))],
                4487:[(18,8,(1,1,1,1)),(19,2,(.5,.5,.5,.5)),(16,8,(.25,.25,.25,.25))],
                4551:[(15,8,(1,1,1,1))],4691:[(16,8,(.8,.8,.8,.8))],
                4705:[(20,7,(4,8,2,0))],4797:[(21,3,(.1,.2,0,0))],
                4810:[(21,7,(.5,.25,1,0))],4818:[(20,7,(2,2,2,0))],
                4832:[(14,8,(1,1,1,1))],4846:[(20,7,(2,2,2,0))],
                4915:[(15,8,(.5,.5,.5,.5))],
                4982:[(14,8,(.5,.5,.5,.5)),(13,7,(0,0,1,0)),(22,7,(0,0,1,0))],
            }
            for at,inputs in fixture_inputs.items():
                for reg,mask,values in inputs:fixture+=ins('mov',dst(reg,mask),floats(*values))
                if at==4128:fixture+=ins('mov',dst(17,8),uint(257))
                fixture+=stages[at]
            fixture+=candidate
            # Rejected candidates have no kind-3 contribution and reset stale data.
            fixture+=reset+ins('mov',dst(10,8),uint(7))+mark(1)+mark(2)+mark(4)+candidate
            fixture+=reset+ins('mov',dst(10,8),uint(8))+mark(1)+candidate
        fixture+=ins('mov',dst(14,7),floats(0,0,1,0))+volume_stages[1093]
        fixture+=ins('mov',dst(4,7),floats(5,6,7,0))+ins('mov',dst(15,7),floats(0,1,0,0))+volume_stages[1122]
        fixture+=ins('mov',dst(17,7),floats(1,2,3,0))+volume_stages[1192]
        fixture+=ins('mov',dst(17,7),floats(.1,.2,.3,0))+volume_stages[1866]
        fixture+=ins('mov',dst(17,15),floats(.5,1,1.5,2))+ins('mov',dst(16,7),floats(1,2,3,0))+ins('mov',dst(12,8),floats(2,2,2,2))
        fixture+=ins('mov',dst(14,8),uint(0))+ins('mov',dst(23,1),uint(123))+ins('mov',dst(23,4),floats(1,1,1,1))+volume
        fixture+=volume_stages[2234]+ins('mov',dst(4,7),floats(.5,1,1.5,0))+volume_stages[2250]
        fixture+=ins('mov',dst(4,7),floats(1,1.5,2,0))+ins('mov',dst(3,8),floats(2,2,2,2))+volume_summary
        if not sample:
            fixture+=ins('mov',dst(15,8),floats(.5,.5,.5,.5))+ins('mov',dst(20,7),floats(2,4,6,0))+ins('mov',dst(17,7),floats(1,1,1,0))+ins('mov',dst(14,8),uint(9))+point
            fixture+=point_reset+ins('mov',dst(point_before_shadow,7),floats(4,8,12,0))+ins('mov',dst(point_response,8),floats(.5,.5,.5,.5))+ins('mov',dst(point_meta,1),uint(2))+ins('mov',dst(point_meta,4),uint(1))+ins('mov',dst(point_shadow,4),floats(2,2,2,2))+ins('mov',dst(point_moments,7),floats(1,2,1,0))+point_detail
            fixture+=ins('mov',dst(surface_volume,7),floats(0,0,0,0))+ins('mov',dst(point_sum,7),floats(1,2,3,0))
            fixture+=ins('mov',dst(9,7),floats(2,3,4,0))+ins('mov',dst(4,7),floats(2,3,4,0))+surface
        fixture+=ins('mov',dst(15,7),floats(2,3,4,0))+plan['account_code']+plan['ray_code']
    fixture+=ins('mov',dst(1,15),floats(.75,2.25,6,1))+ins('mov',dst(plan['extra'],1),[0x4001,0x3f400000])+plan['summary_code']+ins('ret')
    def fixture_patch(w):w[:]=fixture
    (path.parent/'gi-joint-light-oracle.dxbc').write_bytes(transform_shex(raw,fixture_patch))
    path.write_bytes(changed);parsed=decoder.decode(path)
    return changed,dict(schema='gi-joint-light-r285',sha256=hashlib.sha256(changed).hexdigest(),native_sha256=identity['sha256'],
                       native_instruction_recovery=True,decoded_instruction_count=len(parsed['instructions']),
                       capacity=1048576,stride=STRIDE,spot_record_kind=3,sampling_step='runtime_power_of_two',roi_extent='full_dispatch',
                       runtime_enabled=False,gpu_validated=False,
                       limits=['Kind 4 records visited lights, with stage validity; kind 3 records executed contributions.',
                               'Per-light multiplication is separately rounded, not a subtraction of native fused accumulation.',
                               'Light index is session-local; no named-lamp identification.',
                               'GPU validation and runtime selection must be verified before delivery.'])
