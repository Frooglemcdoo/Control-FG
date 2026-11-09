#!/usr/bin/env python3
"""Read Control PC v2 UI resources without modifying game archives.

Extracted game assets are local research inputs, not redistributable mod files.
"""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import struct

MAGIC = b'\x3f\xb3\x4d\xd3'


def read_ui(data):
    def require(condition, message):
        if not condition:
            raise ValueError(message)

    def u32(at):
        require(0 <= at <= len(data) - 4, 'Truncated integer')
        return struct.unpack_from('<I', data, at)[0]

    def name(at, length):
        require(0 < length <= 4096 and at + length <= len(data), 'Invalid path length')
        value = data[at:at + length].decode('utf-8')
        parts = value.split('/')
        require(not any(p in ('', '.', '..') for p in parts)
                and '\\' not in value and ':' not in value and '\0' not in value,
                'Unsafe resource path')
        return value

    require(52 <= len(data) <= 32 * 1024 * 1024, 'Invalid resource size')
    require(data[:4] == MAGIC and u32(4) == 2 and u32(8) == len(data), 'Unknown UI envelope')
    require(u32(12) == 0x70635a61 and u32(16) == 2, 'Unknown UI type/version')
    require(data[20:24] == MAGIC and u32(24) == 2 and u32(28) == 24
            and u32(32) == 0xd0b4291c and u32(36) == 1
            and data[40:44] == MAGIC, 'Unknown page metadata')
    entrypoint = name(48, u32(44))
    pos = 48 + u32(44)
    count = u32(pos)
    require(count <= 4096, 'Too many resource entries')
    pos += 4
    entries = []
    for _ in range(count):
        size = u32(pos + 8)
        require(data[pos:pos + 4] == MAGIC and u32(pos + 4) == 2
                and u32(pos + 12) == 0x4edae845 and u32(pos + 16) == 3,
                'Unknown file record')
        length = u32(pos + 20)
        require(size == length + 76 and pos + size <= len(data), 'Invalid file record size')
        path = name(pos + 24, length)
        meta = pos + 24 + length
        require(data[meta:meta + 4] == MAGIC and u32(meta + 4) == 2
                and u32(meta + 8) == 32 and u32(meta + 12) == 0x1bfc72d5
                and u32(meta + 16) == 1 and data[meta + 28:meta + 32] == MAGIC
                and data[pos + size - 4:pos + size] == MAGIC, 'Unknown file metadata')
        offset, size_bytes = struct.unpack_from('<QQ', data, pos + size - 20)
        entries.append({'path': path, 'offset': offset, 'size': size_bytes})
        pos += size
    payload_size = u32(pos)
    pos += 4
    require(pos + payload_size + 4 == len(data) and data[-4:] == MAGIC,
            'Invalid payload envelope')
    seen = set()
    ranges = []
    for entry in entries:
        start, length = entry['offset'], entry['size']
        require(start <= payload_size and length <= payload_size - start, 'Resource outside payload')
        if length:
            require(entry['path'] not in seen, 'Duplicate embedded resource')
            seen.add(entry['path'])
            ranges.append((start, start + length))
        entry['data'] = data[pos + start:pos + start + length]
    ranges.sort()
    require(all(a[1] <= b[0] for a, b in zip(ranges, ranges[1:])), 'Overlapping resources')
    return entrypoint, entries


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('resource', type=Path)
    parser.add_argument('--extract-to', type=Path, help='Must be a new directory')
    args = parser.parse_args()
    data = args.resource.read_bytes()
    entrypoint, entries = read_ui(data)
    if args.extract_to:
        args.extract_to.mkdir(parents=True, exist_ok=False)
        for entry in entries:
            if entry['size']:
                target = args.extract_to.joinpath(*PurePosixPath(entry['path']).parts)
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(entry['data'])
    print(json.dumps({'sha256': hashlib.sha256(data).hexdigest(), 'entrypoint': entrypoint,
                      'resources': [{k: v for k, v in e.items() if k != 'data'} for e in entries]}, indent=2))


if __name__ == '__main__':
    main()
