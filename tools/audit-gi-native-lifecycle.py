#!/usr/bin/env python3
"""Read-only, hash-pinned audit of recovered R103 native lifecycle callsites.

Does not patch a DLL, prove caller coverage, or enable a live GI hook.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path

SHA256 = 'cceb99cbd9c019af907c24c44701a53c8d230965c1fb31c325233e4c80214aa5'
CALLS = {
    0x26030: (0x25217, 0x2527f, 0x252f1, 0x25367, 0x253d1, 0x2544b, 0x254bb, 0x2551f),
    0x26360: (0x2824f, 0x2ae34, 0x35763),
    0x25f00: (0x17c25, 0x17c67, 0x17d6e, 0x17f0b, 0x25200, 0x25268, 0x252d8, 0x25350,
              0x253b8, 0x25430, 0x254a0, 0x25508, 0x2565d, 0x28233, 0x2ae20, 0x35752),
    0x33a40: (0x357c6, 0x374a1),
}
RELEASE_SEQUENCE = {
    0x25f0d: '488b09',          # wrapper.resource at offset 0
    0x25f15: 'e816e5ffff',      # enqueue resource at 24430 before Release
    0x24485: '488b07488bcfff5008', # resource vtable AddRef
    0x244ac: '488938',          # queue entry retains resource pointer
    0x244af: '488d0d6affffff',  # eventual callback 24420
    0x244b6: '48894808',        # queue entry callback
    0x24420: '488b0148ff6010',  # callback tail-calls resource Release
    0x25f1a: '488b0b488b01ff5010', # wrapper's original Release
    0x25f55: '4889034889431048894318', # clear resource/descriptor fields
}


def require(condition, message):
    if not condition:
        raise ValueError(message)


class PE:
    def __init__(self, data):
        self.data = data
        require(data[:2] == b'MZ', 'DOS signature')
        pe = struct.unpack_from('<I', data, 0x3c)[0]
        require(data[pe:pe+4] == b'PE\0\0', 'PE signature')
        machine, count = struct.unpack_from('<HH', data, pe+4)
        optional = struct.unpack_from('<H', data, pe+20)[0]
        require(machine == 0x8664 and struct.unpack_from('<H', data, pe+24)[0] == 0x20b, 'AMD64 PE32+')
        self.sections = []
        for n in range(count):
            off = pe+24+optional+n*40
            _, rva, size, raw = struct.unpack_from('<IIII', data, off+8)
            flags = struct.unpack_from('<I', data, off+36)[0]
            require(raw+size <= len(data), 'section file bounds')
            self.sections.append((rva, size, raw, flags))

    def offset(self, rva, size):
        found = [raw+rva-start for start, length, raw, flags in self.sections
                 if flags & 0x20000000 and start <= rva and rva+size <= start+length]
        require(len(found) == 1, f'nonunique/non-executable RVA {rva:x}')
        return found[0]

    def read(self, rva, size):
        off = self.offset(rva, size)
        return self.data[off:off+size]


def check_calls(pe):
    records = []
    for target, sites in CALLS.items():
        pe.offset(target, 1)
        for site in sites:
            raw = pe.read(site, 5)
            require(raw[0] == 0xe8, f'not direct call at {site:x}')
            actual = site+5+struct.unpack_from('<i', raw, 1)[0]
            require(actual == target, f'wrong target at {site:x}: {actual:x}')
            records.append({'site': hex(site), 'target': hex(target), 'bytes': raw.hex()})
    require(pe.read(0x34378, 4) == bytes.fromhex('4883c040'), 'binding row increment')
    for site, expected in RELEASE_SEQUENCE.items():
        raw = bytes.fromhex(expected)
        require(pe.read(site, len(raw)) == raw, f'release ownership sequence at {site:x}')
    return records


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('dll', type=Path)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    data = args.dll.read_bytes()
    require(hashlib.sha256(data).hexdigest() == SHA256, 'unsupported DLL hash')
    pe = PE(data)
    records = check_calls(pe)
    # Exercise the actual instruction checks, not just the outer hash gate.
    rejected = 0
    for sites in CALLS.values():
        for site in sites:
            for byte in (0, 1):  # replaced opcode and changed relative target
                changed = bytearray(data)
                changed[pe.offset(site, 5)+byte] ^= 1
                try:
                    check_calls(PE(changed))
                except ValueError:
                    rejected += 1
                else:
                    raise RuntimeError('mutated call accepted')
    sequence_rejected = 0
    for site in RELEASE_SEQUENCE:
        changed = bytearray(data)
        changed[pe.offset(site, 1)] ^= 1
        try:
            check_calls(PE(changed))
        except ValueError:
            sequence_rejected += 1
        else:
            raise RuntimeError('mutated release sequence accepted')
    report = {'schema': 2, 'sha256': SHA256, 'source_commit': '2f314e8992aec822f13cafbf7efa8bb55dc3915c',
              'calls': records, 'mutation_rejections': rejected, 'binding_row_stride': 64,
              'scope': 'listed direct calls only; not exhaustive callers, GI dispatch identity, ABI or lifetime proof',
              'release_sequence': {hex(k): v for k, v in RELEASE_SEQUENCE.items()},
              'release_sequence_mutation_rejections': sequence_rejected,
              'native_deferred_reference': True,
              'gpu_completion_proven': False,
              'runtime_hooks_installed_by_audit': False}
    text = json.dumps(report, indent=2)+'\n'
    if args.output:
        args.output.write_text(text, encoding='utf-8')
    print(f'PASS {len(records)} direct calls; {rejected} modified calls rejected; no runtime changes')


if __name__ == '__main__':
    main()
