"""Audit the runtime-resolved scene query. No game process is accessed.
Usage: python audit-query-target.py renderer.dll [probe-log.txt]
Requires pefile and capstone. A PASS verifies static contracts, not safe retirement.
"""
import hashlib
import json
import struct
import sys
from pathlib import Path
import capstone
import pefile

EXPECTED = 'ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433'

def audit(path, log_path=None):
    raw = Path(path).read_bytes()
    if hashlib.sha256(raw).hexdigest() != EXPECTED:
        raise ValueError('Unsupported renderer SHA256')
    p = pefile.PE(data=raw)
    c = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
    evidence = []
    def check(rva, expected):
        i = next(c.disasm(p.get_data(rva, 15), rva))
        actual = i.mnemonic + ' ' + i.op_str
        if actual != expected:
            raise ValueError(f'{rva:#x}: expected {expected}, got {actual}')
        evidence.append({'rva': hex(rva), 'instruction': actual})
    # Vtable slot observed in user's runtime capture resolves to this image.
    if struct.unpack('<Q', p.get_data(0x634cb8 + 0x50, 8))[0] != p.OPTIONAL_HEADER.ImageBase + 0x165520:
        raise ValueError('Query vtable target mismatch')
    checks = {
        0x165553: 'bt r9d, 0x11', 0x16555a: 'bt ebx, 0x11',
        0x16556f: 'xor edx, edx', 0x165575: 'call 0x1655c0',
        0x165589: 'mov edx, 1', 0x165595: 'call 0x1655c0',
        0x165636: 'shl rcx, 8',
        0x165642: 'mov rcx, qword ptr [rcx + rdi + 0x68]',
        0x1657ca: 'cmp byte ptr [r15 + 0x39], 0',
        0x1657da: 'call 0x165dc0',
        0x165e20: 'movups xmm0, xmmword ptr [rax]',
        0x165e26: 'minps xmm6, xmm0',
        0x165e37: 'maxps xmm7, xmm0',
        0x165eda: 'call 0x165dc0',
        0x165f41: 'mov byte ptr [r9 + 0x39], 1',
        0x165f5c: 'movups xmmword ptr [r9], xmm0',
        0x165f63: 'movups xmmword ptr [r9 + 0x10], xmm2',
        0x1658ff: 'test dword ptr [rbx + 0x28], r12d',
        0x165913: 'and eax, r9d', 0x165916: 'cmp eax, r15d',
        0x165929: 'call qword ptr [rax + 0x28]',
        0x165938: 'call 0x22730', 0x165944: 'add rbx, 0x30',
        0x165fd4: 'test dword ptr [rcx + 8], esi',
        0xa81eb: 'mov dword ptr [rax + 0x18], ebx',
        0xa81f5: 'mov dword ptr [rax + 0x78], ebx',
        0xa81fb: 'mov dword ptr [rax + 0x98], ebx',
        0xa8204: 'mov dword ptr [rax + 0x88], ebx',
        0xa83af: 'mov r8d, 0x100', 0xa83b9: 'mov r9d, 8',
        0xa83d5: 'mov dword ptr [rsp + 0x20], ebx',
        0xa83d9: 'call qword ptr [r10 + 0x50]',
    }
    for rva, expected in checks.items():
        check(rva, expected)
    runtime = None
    if log_path:
        log = Path(log_path).read_text()
        for token in ('renderer_sha256=' + EXPECTED,
                      'query_module=renderer_rmdwin10_f.dll query_rva=0x165520 vtable_module=renderer_rmdwin10_f.dll vtable_rva=0x634cb8',
                      'unique_query_targets=1', 'PASS query target identified'):
            if token not in log:
                raise ValueError('Runtime log does not establish expected target: ' + token)
        runtime = {'sha256': hashlib.sha256(Path(log_path).read_bytes()).hexdigest(),
                   'scope': 'Target identity only; not a synchronized scene snapshot'}
    return {'status': 'PASS', 'renderer_sha256': EXPECTED, 'runtime_evidence': runtime,
            'query_rva': '0x165520', 'traversal_rva': '0x1655c0',
            'bounds_refresh_rva': '0x165dc0',
            'query_has_native_writes': True, 'atomic_scene_snapshot': False,
            'native_retirement_validated': False, 'performance_claim': False,
            'evidence': evidence}

if __name__ == '__main__':
    try:
        print(json.dumps(audit(*sys.argv[1:]), indent=2))
    except Exception as exc:
        print(json.dumps({'status': 'FAIL', 'error': str(exc)}))
        sys.exit(1)
