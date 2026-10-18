"""Run the production hash function against the captured character shader."""
from pathlib import Path
import hashlib
import re
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
data = bytes.fromhex((root / 'tools/fixtures/rr-character-albedo.hex').read_text())
runtime = (root / 'src/rr_character_base_albedo_runtime.h').read_text()
source = (root / 'src/rr_albedo_shader.h').read_text()
function = re.search(r'inline uint64_t HashFNV1a\([^}]+\}[^}]+\}', source).group()
expected = re.search(r'albedo.hashFNV1a!=UINT64_C\((0x[0-9a-f]+)\)', runtime).group(1)
assert len(data) == int(re.search(r'albedo.bytes.size\(\)!=(\d+)', runtime).group(1))
assert hashlib.sha256(data).hexdigest() in runtime
with tempfile.TemporaryDirectory() as temp:
    path = Path(temp)
    cpp = path / 'match.cpp'
    exe = path / 'match.exe'
    values = ','.join(str(x) for x in data)
    cpp.write_text('#include <cstdint>\n#include <cstddef>\n#include <cassert>\n'
                   + function + '\nint main(){uint8_t data[]={' + values + '};\n'
                   + 'assert(HashFNV1a(data,sizeof(data))==UINT64_C(' + expected + '));\n'
                   + 'data[100]^=1;assert(HashFNV1a(data,sizeof(data))!=UINT64_C('
                   + expected + '));}\n')
    if shutil.which('cl'):
        cmd = ['cl', '/nologo', '/std:c++17', '/EHsc', '/W4', '/WX', str(cpp),
               '/Fe' + str(exe), '/Fo' + str(path / 'match.obj')]
    else:
        cmd = ['g++', '-std=c++17', '-Wall', '-Wextra', '-Werror', str(cpp), '-o', str(exe)]
    subprocess.run(cmd, check=True)
    subprocess.run([str(exe)], check=True)
print('PASS: captured character shader accepted by production hash; modified shader rejected; SHA256 and size match')
