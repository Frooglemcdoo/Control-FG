"""Offline PIX E3 pipeline/binding inventory. Requires dissect.util.

Consumes the existing AllSource and ShaderBlocks zip files; never launches PIX
or the game. Command locations are source order, NOT GPU execution order.
"""
import argparse
import hashlib
import json
import re
import struct
import zipfile
from pathlib import Path
from dissect.util.compression import lzxpress


def bindings(data):
    count = struct.unpack_from('<I', data, 28)[0]
    for offset in struct.unpack_from('<' + 'I' * count, data, 32):
        if data[offset:offset + 4] != b'RDEF':
            continue
        size = struct.unpack_from('<I', data, offset + 4)[0]
        r = data[offset + 8:offset + 8 + size]
        n, start = struct.unpack_from('<II', r, 8)
        # RDEF target version is minor, major at bytes 16,17.
        minor, major = r[16:18]
        stride = 40 if (major, minor) >= (5, 1) else 32
        result = []
        for i in range(n):
            fields = struct.unpack_from('<' + 'I' * (stride // 4), r, start + i * stride)
            name = r[fields[0]:r.index(b'\0', fields[0])].decode()
            result.append(dict(name=name, kind=fields[1], slot=fields[5],
                               count=fields[6], space=fields[8] if stride == 40 else 0))
        return result
    return None  # DXIL reflection is a separate analysis, not a failed RDEF.


def inventory(source, prefixes):
    views = {}
    with zipfile.ZipFile(source) as src, zipfile.ZipFile(prefixes) as packed:
        for name in sorted(src.namelist()):
            if not name.endswith('/project/CreatePSOs.cpp'):
                continue
            view = name.split('/')[0]
            base = name.rsplit('/', 1)[0] + '/'
            text = src.read(name).decode()
            functions = dict(re.findall(r'void (Create\w+_\d+)\(\)\s*\{(.*?)\n\}', text, re.S))
            init = src.read(base + 'FrameResources_000.cpp').decode()
            calls = re.findall(r'^    (Create(?:GraphicsPipelineState|ComputePipelineState|StateObject)_\d+)\(\);', init, re.M)
            raw = packed.read(view + '/shader-prefix.bin')
            cursor = 0
            pipelines = {}
            for function in calls:
                shaders = []
                for size in re.findall(r'g_resourceReader->Read\([^,]+,\s*(\d+)\)', functions[function]):
                    size = int(size)
                    block = raw[cursor:cursor + size]
                    if len(block) != size or block[:4] != bytes.fromhex('0a51e5c0'):
                        raise ValueError(f'{view}: missing/invalid compressed block at {cursor}')
                    decoded = lzxpress.decompress(block[28:])
                    if len(decoded) != int.from_bytes(block[8:16], 'little'):
                        raise ValueError(f'{view}: decompressed size mismatch at {cursor}')
                    cursor += size
                    position = 0
                    while position < len(decoded):
                        if decoded[position:position + 4] != b'DXBC':
                            raise ValueError('Invalid shader container')
                        length = struct.unpack_from('<I', decoded, position + 24)[0]
                        if length < 32 or position + length > len(decoded):
                            raise ValueError('Invalid shader length')
                        shader = decoded[position:position + length]
                        shaders.append(dict(sha256=hashlib.sha256(shader).hexdigest(),
                                            bytes=length, bindings=bindings(shader)))
                        position += length
                pipelines[function.rsplit('_', 1)[1]] = dict(function=function, shaders=shaders)
            dispatches = []
            for command in sorted(n for n in src.namelist() if n.startswith(base + 'CommandLists_') and n.endswith('.cpp')):
                current = {}
                for line_no, line in enumerate(src.read(command).decode().splitlines(), 1):
                    reset = re.search(r'GetCommandList\((\d+)\)->Reset\(.*GetPipelineState\((\d+)\)', line)
                    state = re.search(r'GetCommandList\((\d+)\)->SetPipelineState1?\(Get(?:PipelineState|StateObject)\((\d+)\)', line)
                    if reset or state:
                        match = state or reset
                        current[match[1]] = match[2]
                    dispatch = re.search(r'GetCommandList\((\d+)\)->(DispatchRays|Dispatch)\((.*?)\);', line)
                    if dispatch:
                        dispatches.append(dict(file=command[len(base):], line=line_no,
                                               command_list=dispatch[1], kind=dispatch[2],
                                               arguments=dispatch[3], pipeline=current.get(dispatch[1])))
            views[view] = dict(prefix_bytes_consumed=cursor, pipelines=pipelines, dispatches=dispatches)
    return dict(schema=1, limitation='Static source locations and declared shader bindings; not GPU execution order or resource-value validation.', views=views)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('prefixes', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    args.output.write_text(json.dumps(inventory(args.source, args.prefixes), indent=2) + '\n')
