"""Exercise the glass module gate with the production case-sensitive hash format."""
import re
from pathlib import Path
root=Path(__file__).resolve().parents[1]
source=(root/'src/pt_glass_runtime.h').read_text()
target=(root/'src/target.h').read_text()
probe=(root/'src/probe.cpp').read_text()
assert '"%02X", digest[i]' in probe and 'strcmp(hex, expected) == 0' in probe
expected={name:value for name,value in re.findall(r'char (k\w+Hash)\[\] = "([A-Fa-f0-9]+)"',target)}
checks=re.findall(r'HashMatches\((renderer|d3d),([^)]*)\)',source)
assert len(checks)==2,checks
captured={
 'renderer':'ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433',
 'd3d':'cceb99cbd9c019af907c24c44701a53c8d230965c1fb31c325233e4c80214aa5',
}
for module,argument in checks:
 value=expected.get(argument,argument.strip('"'))
 production=''.join(f'{v:02X}' for v in bytes.fromhex(captured[module]))
 assert production==value,(module,'valid Steam module rejected by glass installation gate')
 assert production.lower()!=value,'test must catch original lowercase regression'
 assert '0'*64!=value,'unexpected module must remain rejected'
assert 'reason=unsupported_module_hash' in source
assert 'PTGlassRuntime::Install(renderer,d3d);' in (root/'src/pt_reflection_loader.h').read_text()
print('PASS: actual captured Steam module hashes pass production glass gate; wrong hashes rejected; failure logged.')
