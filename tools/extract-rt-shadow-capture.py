"""Recover hash-verified shader/root blobs from one RT integration session log.

Reject partial captures, mismatched lengths, gaps, corrupt hex and hash failures.
Names are derived only from an allow-listed kind and a verified SHA256.
"""
from pathlib import Path
import argparse
import hashlib
import re

BEGIN = re.compile(r'^rt_blob_begin kind=(direct_cs|compute_cs|dxil_library|root_signature) sha=([0-9a-f]{64}) bytes=(\d+)$')
DATA = re.compile(r'^rt_blob_data sha=([0-9a-f]{64}) offset=(\d+) hex=([0-9a-f]+)$')
END = re.compile(r'^rt_blob_end sha=([0-9a-f]{64})$')
MAX_BLOB = 2 * 1024 * 1024
MAX_SESSION = 8 * 1024 * 1024

def extract(text):
    blobs = {}
    active = None
    total = 0
    for line in text.splitlines():
        if match := BEGIN.fullmatch(line):
            if active is not None: raise ValueError('Incomplete blob before next header')
            kind, sha, size = match.groups()
            size = int(size)
            if not 0 < size <= MAX_BLOB or total + size > MAX_SESSION:
                raise ValueError('Capture size limit exceeded')
            active = (kind, sha, size, bytearray())
        elif match := DATA.fullmatch(line):
            sha, offset, hex_bytes = match.groups()
            if active is None or sha != active[1] or int(offset) != len(active[3]):
                raise ValueError('Unexpected blob data or non-contiguous offsets')
            active[3].extend(bytes.fromhex(hex_bytes))
            if len(active[3]) > active[2]: raise ValueError('Blob exceeds declared size')
        elif match := END.fullmatch(line):
            if active is None or match[1] != active[1]: raise ValueError('Unexpected blob terminator')
            kind, sha, size, data = active
            if len(data) != size or hashlib.sha256(data).hexdigest() != sha:
                raise ValueError('Blob length or SHA256 mismatch')
            if sha in blobs: raise ValueError('Duplicate blob')
            blobs[sha] = (kind, bytes(data))
            total += size
            active = None
        elif line.startswith('rt_blob_begin') or line.startswith('rt_blob_data') or line.startswith('rt_blob_end'):
            raise ValueError('Malformed blob record')
    if active is not None: raise ValueError('Truncated capture')
    return blobs

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('log', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    blobs = extract(args.log.read_text(encoding='utf-8'))
    if not blobs: raise SystemExit('No captured blobs found')
    args.output.mkdir(parents=True, exist_ok=True)
    for sha, (kind, data) in blobs.items():
        extension = 'rootsig' if kind == 'root_signature' else 'dxbc'
        (args.output / f'{kind}-{sha}.{extension}').write_bytes(data)
    print(f'PASS: {len(blobs)} complete, SHA256-verified blobs extracted')

if __name__ == '__main__': main()
