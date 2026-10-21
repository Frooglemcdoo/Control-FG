"""Exercise the legacy binding stride and custom-data blocks found in R65."""
import importlib.util
from pathlib import Path
import struct
import tempfile

source = Path(__file__).resolve().parents[1] / 'research/native-integration-audit/tools/decode_dxbc.py'
spec = importlib.util.spec_from_file_location('decoder', source)
decoder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(decoder)

def fixture(model, instructions):
    stride = 40 if model >= 0x501 else 32
    binding = [28 + stride, 2, 5, 4, 0, 7, 1, 0]
    if stride == 40:
        binding += [3, 9]
    rdef = struct.pack('<7I', 0, 0, 1, 28, 0x43530000 | model, 0, 0)
    rdef += struct.pack('<' + 'I' * len(binding), *binding) + b'TestTexture\0'
    words = [0x50050, len(instructions) + 2, *instructions]
    shex = struct.pack('<' + 'I' * len(words), *words)
    chunks = [b'RDEF' + struct.pack('<I', len(rdef)) + rdef,
              b'SHEX' + struct.pack('<I', len(shex)) + shex]
    header = b'DXBC' + bytes(16) + struct.pack('<3I', 1, 40 + sum(map(len, chunks)), 2)
    return header + struct.pack('<2I', 40, 40 + len(chunks[0])) + b''.join(chunks)

ret = (1 << 24) | decoder.opnames.index('ret')
custom = decoder.opnames.index('customdata') | (3 << 11)
with tempfile.TemporaryDirectory() as directory:
    path = Path(directory) / 'test.dxbc'
    for model, space, identifier in ((0x500, 0, 0), (0x501, 3, 9)):
        path.write_bytes(fixture(model, [custom, 3, 0x12345678, ret]))
        result = decoder.decode(path)
        binding = result['bindings'][0]
        assert (binding['name'], binding['slot'], binding['space'], binding['identifier']) == ('TestTexture', 7, space, identifier)
        assert [i['name'] for i in result['instructions']] == ['customdata', 'ret']
        assert result['instructions'][1]['dword'] == 5
    for instructions in ([custom, 0], [custom, 20], [ret & 2047], [(5 << 24) | (ret & 2047)]):
        path.write_bytes(fixture(0x501, instructions))
        try:
            decoder.decode(path)
        except ValueError:
            pass
        else:
            raise AssertionError('Invalid instruction length accepted')
print('PASS: SM5.0/5.1 bindings, custom-data advancement, malformed lengths rejected')
