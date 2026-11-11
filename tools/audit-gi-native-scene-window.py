#!/usr/bin/env python3
"""Build-locked native scene-window evidence; never certifies an ownership lease."""
import argparse, hashlib, json, struct
from pathlib import Path
import capstone
import pefile

EXPECTED = 'ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433'

def audit(path):
    raw = Path(path).read_bytes()
    digest = hashlib.sha256(raw).hexdigest()
    if digest != EXPECTED:
        raise ValueError('Unsupported renderer identity; do not reuse offsets')
    pe = pefile.PE(data=raw)
    dis = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
    dis.detail = True
    imports = {item.address - pe.OPTIONAL_HEADER.ImageBase:
               (item.name or b'').decode('ascii')
               for group in pe.DIRECTORY_ENTRY_IMPORT for item in group.imports}
    def instruction(rva):
        return next(dis.disasm(pe.get_data(rva,15),rva,count=1))
    def relative_memory(rva):
        ins=instruction(rva)
        refs=[o.mem for o in ins.operands if o.type==capstone.x86.X86_OP_MEM
              and o.mem.base==capstone.x86.X86_REG_RIP]
        assert len(refs)==1
        return rva+ins.size+refs[0].disp
    def imported_call(rva,expected):
        assert instruction(rva).mnemonic=='call'
        name=imports[relative_memory(rva)]
        assert expected in name
        return name
    def direct_call(rva,target):
        ins=instruction(rva)
        assert ins.mnemonic=='call' and ins.operands[0].type==capstone.x86.X86_OP_IMM
        assert ins.operands[0].imm==target
    # Material upload precedes creation/submission of the exact job joined by
    # the GI render invocation. Do not infer that later frames cannot overlap.
    direct_call(0x12491D,0xC7BC0)
    direct_call(0xC7C59,0xC51D0)
    job=relative_memory(0x1249F0)
    assert job==relative_memory(0xA8C02)==0x90E9E8
    assert relative_memory(0x1249E9)==job
    job_run=imported_call(0x124A01,'runJob@JobSystem')
    job_join=imported_call(0xA8C13,'waitJob@JobSystem')
    vtable=relative_memory(0x1249D7)
    callback=struct.unpack('<Q',pe.get_data(vtable+8,8))[0]-pe.OPTIONAL_HEADER.ImageBase
    assert vtable==0x6281D0 and callback==0xA8F70
    assert pe.get_data(callback,9)==bytes.fromhex('4883c118e947faffff')
    tlas_build=imported_call(0xA8BCA,'buildTopLevelAccelerationStructure@DeviceUtilRaytracing')
    # Native graphics preparation -> async scene -> graphics consumer handshake.
    fence_calls = {
        'prepare_submit': imported_call(0x124130, 'submitGraphicsCommandList'),
        'prepare_signal': imported_call(0x124152, 'signalFence'),
        'async_wait': imported_call(0xA8C45, 'waitFence'),
        'cleanup_submit': imported_call(0xA8C61, 'executeDeferredThreadCleanup'),
        'async_signal': imported_call(0xA8C8F, 'signalFence'),
        'consumer_submit': imported_call(0xA8C95, 'submitGraphicsCommandList')}
    assert instruction(0xA8CB4).mnemonic == 'jmp'
    assert 'waitFence' in imports[relative_memory(0xA8CB4)]
    for address in (0x12413D, 0xA8C85):
        assert pe.get_data(address,3) == bytes.fromhex('48ff00')
    for address in (0x124136,0xA8C7B):
        assert pe.get_data(address,7) == bytes.fromhex('488b83d0000000')
    assert relative_memory(0xA8C2F) == relative_memory(0xA8C72) == 0x7EC040
    lo, hi = 0x128D20, 0x12DA38
    assert any(e.struct.BeginAddress == lo and e.struct.EndAddress == hi
               for e in pe.DIRECTORY_ENTRY_EXCEPTION), 'Unexpected function extent'
    instructions = list(dis.disasm(pe.get_data(lo, hi-lo), lo))
    assert instructions[-1].address + instructions[-1].size == hi
    calls = {}
    for ins in instructions:
        if ins.mnemonic != 'call':
            continue
        op = ins.operands[0]
        if op.type == capstone.x86.X86_OP_IMM:
            calls[ins.address] = {'target_rva': hex(op.imm)}
        elif op.type == capstone.x86.X86_OP_MEM and op.mem.base == capstone.x86.X86_REG_RIP:
            slot = ins.address + ins.size + op.mem.disp
            calls[ins.address] = {'import': imports.get(slot, 'unknown'), 'slot_rva': hex(slot)}
        else:
            calls[ins.address] = {'indirect': ins.op_str}
    for address in [0x12C5EB, 0x12D342, 0x12D8BA]:
        assert '?raytrace@DeviceUtilRaytracing@d3d@@' in calls[address]['import']
    assert '?dispatch@DeviceUtil@d3d@@' in calls[0x12CC16]['import']
    # Trivial accessors are byte-verified, not inferred from neighboring methods.
    assert pe.get_data(0xA8190, 8) == bytes.fromhex('48 8b 81 80 00 00 00 c3')
    assert pe.get_data(0xA81A0, 7) == bytes.fromhex('8b 81 88 00 00 00 c3')
    for address, target in [(0x12C5AA,0xA81A0),(0x12C5C3,0xA8190),
                            (0x12D2FF,0xA81A0),(0x12D318,0xA8190),
                            (0x12D878,0xA81A0),(0x12D891,0xA8190)]:
        assert calls[address]['target_rva'] == hex(target)
    # Normal control-flow pairing: from the native GI ray return, every local
    # branch reconverges at GI6. Calls are treated as returning; this does not
    # prove transitive side effects or exceptional/fault paths.
    start, stop = 0x12C5F1, 0x12CC16
    body = {i.address:i for i in instructions if start <= i.address < stop}
    pending, visited, branches = [start], set(), []
    while pending:
        address = pending.pop()
        if address == stop or address in visited:
            continue
        if address not in body:
            raise ValueError(f'GI continuation exits interval at {address:#x}')
        visited.add(address)
        ins = body[address]
        after = address + ins.size
        if ins.group(capstone.CS_GRP_RET):
            raise ValueError('GI continuation returns before shading')
        if ins.group(capstone.CS_GRP_JUMP):
            operand = ins.operands[0]
            if operand.type != capstone.x86.X86_OP_IMM:
                raise ValueError('Indirect GI continuation branch')
            successors = [operand.imm] if ins.mnemonic == 'jmp' else [operand.imm, after]
            # Acyclicity is part of this exact native interval's contract.
            if any(target <= address for target in successors):
                raise ValueError('Backward branch in GI continuation')
            branches.append({'rva':hex(address), 'kind':ins.mnemonic,
                             'successors':[hex(t) for t in successors]})
        else:
            successors = [after]
        pending.extend(successors)
    if calls[0x128F3B].get('target_rva') != '0xa8bf0' or calls[0x128F5B].get('target_rva') != '0xa8080':
        raise ValueError('Native scene join/bind order changed')
    # These later ray passes are CONDITIONAL. Their presence in code alone does
    # not imply they executed in the capture or authorize delayed GI dispatch.
    interval = [{'rva': hex(a), **value} for a,value in calls.items()
                if 0x12C5F1 <= a <= 0x12CC16]
    return {'renderer_sha256': digest, 'function': [hex(lo),hex(hi)],
            'function_sha256': hashlib.sha256(pe.get_data(lo,hi-lo)).hexdigest(),
            'gi_ray_call': '0x12c5eb', 'gi_shade_call': '0x12cc16',
            'later_conditional_ray_calls': {'sun_shadow':'0x12d342','ao':'0x12d8ba'},
            'scene_binding_array_offset':'0x80', 'scene_binding_count_offset':'0x88',
            'scene_join_call':'0x128f3b', 'scene_bind_call':'0x128f5b',
            'scene_fence_handshake':{'scene_fence_offset':'0xd0',
                'async_flag':'0x7ec040','verified_imports':fence_calls,
                'graphics_wait_tail':'0xa8cb4','later_writer_exclusion_proven':False},
            'scene_preparation_chain':{
                'material_rebuild_call':'0x12491d',
                'material_record_update_call':'0xc7c59',
                'job_handle_global':hex(job),
                'job_run':{'rva':'0x124a01','import':job_run},
                'job_callback':hex(callback),'worker':'0xa89c0',
                'worker_tlas_build':{'rva':'0xa8bca','import':tlas_build},
                'job_join':{'rva':'0xa8c13','import':job_join},
                'material_rebuild_precedes_job_run':True,
                'join_uses_same_job_handle':True,
                'excludes_future_frame_or_concurrent_mutation':False},
            'normal_ray_to_shade_control_flow':{
                'start':hex(start), 'sole_exit':hex(stop),
                'instruction_count':len(visited), 'branches':branches,
                'acyclic':True, 'calls_assumed_to_return':True,
                'transitive_mutation_proven_absent':False},
            'calls_between_ray_and_shade':interval,
            'scene_lease_proven':False,
            'remaining':['Verify executed native scene scope and current frame ownership',
                         'Audit transitive/concurrent scene or material mutation',
                         'Tie descriptor allocation and resource retirement to submitted consumer']}

if __name__ == '__main__':
    parser=argparse.ArgumentParser()
    parser.add_argument('renderer')
    parser.add_argument('--output',required=True)
    args=parser.parse_args()
    result=audit(args.renderer)
    Path(args.output).write_text(json.dumps(result,indent=2)+'\n')
    print('Verified native GI interval and two CONDITIONAL later scene consumers; scene lease NOT certified.')
