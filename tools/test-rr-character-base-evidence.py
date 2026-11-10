"""Audit captured native reflectance and execute the production R417 allowlist."""
import base64, hashlib, json, re, shutil, subprocess, tempfile
from pathlib import Path
from character_surface import decode, base_composition_site

root = Path(__file__).resolve().parents[1]
folder = root / 'research/character-surface-r346'
allowed = {'skin-basic', 'skin-live', 'skin-live-noise-a', 'skin-live-noise-b'}
rows = []
with tempfile.TemporaryDirectory() as tmp:
    tmp = Path(tmp)
    for family in json.loads((folder / 'families.json').read_text()):
        data = base64.b64decode((folder / (family['name'] + '.dxbc.b64')).read_bytes())
        sha = hashlib.sha256(data).hexdigest()
        assert sha == family['sha']
        rows.append((family['name'], len(data), sha))
        if family['name'] not in allowed:
            continue
        path = tmp / 'native.dxbc'; path.write_bytes(data)
        d = decode(path); instructions = d['instructions']
        site = base_composition_site(d); at = instructions.index(site)
        # Native diffuse is multiplied by r3, formed immediately from tint*base.
        sample, multiply = instructions[at-2:at]
        assert sample['name'] == 'sample' and sample['operands'][0] == 'r[4].xyzw'
        assert sample['operands'][1] == 'v[4].xyxx'
        assert multiply['name'] == 'mul' and multiply['operands'] == ['r[3].xyzw', 'r[3].xyzw', 'r[4].xyzw']
        assert site['operands'][2] == 'r[3].xyzx'
        material = next(b for b in d['bindings'] if b['name'] == 'g_sbMaterialDataPart3')
        mid = next(b for b in d['bindings'] if b['name'] == 'mid_general')
        textures = next(b for b in d['bindings'] if b['name'] == 'g_sMaterialTextureArray')
        cb = f"cb[{mid['identifier']}][{mid['slot']}][0].x"
        tint = next(i for i in reversed(instructions[:at-2]) if i['operands'] and i['operands'][0] == 'r[3].xyzw')
        assert tint['name'] == 'ld_structured'
        assert tint['operands'][1:] == [cb, 'l(52)', f"t[{material['identifier']}][{material['slot']}].xyzw"]
        index = re.fullmatch(r't\[(\d+)\]\[(r\[\d+\]\.[xyzw])\]\.xyzw', sample['operands'][2])
        assert index and int(index[1]) == textures['identifier']
        # Last write of the sampled index must be materialIndex*35, no +7 detail.
        register, lane = index[2].split('.')
        writes = [i for i in instructions[:at-2] if any(o.startswith(register+'.') and lane in o.split('.')[-1] for o in i['operands'][:2 if i['name']=='imul' else 1])]
        assert writes[-1]['name'] == 'imul' and writes[-1]['operands'] == ['null', index[2], cb, 'l(35)']
        print('PASS native base*tint with no detail multiplier:', family['name'])
    header = root / 'src/rr_character_base_evidence.h'
    code = '#include <cassert>\n#include "' + header.as_posix() + '"\nint main(){\n'
    for name, size, sha in rows:
        expected = 'true' if name in allowed else 'false'
        code += f'assert(control_skin_guide::Matches({size},"{sha}")=={expected});\n'
        changed = ('0' if sha[0]!='0' else '1') + sha[1:]
        code += f'assert(!control_skin_guide::Matches({size},"{changed}"));\n'
        code += f'assert(!control_skin_guide::Matches({size+1},"{sha}"));\n'
    code += 'assert(!control_skin_guide::Matches(30160,nullptr));}\n'
    cpp = tmp / 'evidence.cpp'; cpp.write_text(code); exe = tmp / 'evidence.exe'
    cmd = (['cl','/nologo','/std:c++17','/EHsc','/W4','/WX',str(cpp),'/Fe'+str(exe),'/Fo'+str(tmp/'evidence.obj')]
           if shutil.which('cl') else ['g++','-std=c++17','-Wall','-Wextra','-Werror',str(cpp),'-o',str(exe)])
    subprocess.run(cmd,check=True); subprocess.run([str(exe)],check=True)
print('PASS production allowlist: four admitted; detail/cloth/hair, changed hashes and wrong sizes rejected')
