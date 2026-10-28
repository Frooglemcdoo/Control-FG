"""Read-only verification of the R199 Steam camera targets. Never executes the EXE."""
import hashlib
import json
from pathlib import Path
import re
import struct
import sys


def verify(path):
    data = Path(path).read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    expected = "9441db3ae75b267abd989846ad0895e3fe24abcc0f06e58f93c34fc8d4736506"
    if digest != expected:
        raise ValueError("Unsupported executable SHA-256: " + digest)
    pe = struct.unpack_from("<I", data, 60)[0]
    optional = pe + 24
    count = struct.unpack_from("<H", data, pe + 6)[0]
    size = struct.unpack_from("<H", data, pe + 20)[0]
    sections = [struct.unpack_from("<IIII", data, optional + size + i * 40 + 8) for i in range(count)]

    def offset(rva):
        for _, address, length, start in sections:
            if address <= rva < address + length:
                return rva - address + start
        raise ValueError("Unmapped file RVA: " + hex(rva))

    def at(rva, size):
        start = offset(rva)
        return data[start:start + size]

    signature = bytes.fromhex("48 8b c4 56 57 41 55 48 81 ec 80 00 00 00 48 89 58 08")
    pattern = re.escape(signature[:10]) + b"...." + re.escape(signature[14:])
    matches = [m.start() for m in re.finditer(pattern, data, re.S)]
    if matches != [offset(0x384540)] or at(0x384540, len(signature)) != signature:
        raise ValueError("Camera setter is not the unique expected match")
    if at(0x383c99, 12) != bytes.fromhex("e8 02 0f 00 00 48 63 0d 27 69 f1 00"):
        raise ValueError("Camera update call / mode load changed")
    call_target = 0x383c99 + 5 + struct.unpack("<i", at(0x383c9a, 4))[0]
    mode_target = 0x383c9e + 7 + struct.unpack("<i", at(0x383ca1, 4))[0]
    if call_target != 0x384ba0 or mode_target != 0x129a5cc:
        raise ValueError("Unexpected native update or mode target")
    return {"sha256": digest, "signature_matches": 1, "setter_rva": "0x384540",
            "camera_update_rva": hex(call_target), "hook_call_rva": "0x383c99",
            "mode_rva": hex(mode_target), "static_verification": "pass",
            "in_game_behavior": "not tested by this tool"}


if __name__ == "__main__":
    print(json.dumps(verify(sys.argv[1]), indent=2))
