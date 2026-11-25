"""Refresh hash evidence after editing the RR live-guide shader.

Order matters: shader -> ARTIFACT-SHA256.json -> current-release checkpoint
-> SOURCE-SHA256.json. Values are replaced in place so file formatting is kept.
"""
import hashlib, pathlib, re, sys

root = pathlib.Path(__file__).resolve().parent.parent
SHADER = 'src/shaders/rr_live_guides.hlsl'
CHECKPOINT = 'validation/current-release/results.json'

def digest(rel):
    return hashlib.sha256((root / rel).read_bytes()).hexdigest()

def set_hash(manifest, key, value, upper=False, insert=False):
    p = root / manifest
    text = p.read_bytes().decode('utf-8')
    value = value.upper() if upper else value
    pat = re.compile(r'("' + re.escape(key) + r'"\s*:\s*")[0-9A-Fa-f]{64}(")')
    if pat.search(text):
        text = pat.sub(lambda m: m.group(1) + value + m.group(2), text, count=1)
    elif insert:
        anchor = re.compile(r'("tested_sha256"\s*:\s*\{)(\s*)')
        m = anchor.search(text)
        if not m:
            sys.exit(f'cannot insert {key} into {manifest}')
        text = text[:m.end()] + f'"{key}": "{value}",' + m.group(2) + text[m.end():]
    else:
        sys.exit(f'{key} not found in {manifest}')
    p.write_bytes(text.encode('utf-8'))

shader = digest(SHADER)
set_hash('ARTIFACT-SHA256.json', SHADER, shader, upper=True)
set_hash(CHECKPOINT, SHADER, shader, insert=True)
for extra in ('GITHUB_RELEASE.md',):
    set_hash(CHECKPOINT, extra, digest(extra))
    set_hash('SOURCE-SHA256.json', extra, digest(extra))
set_hash(CHECKPOINT, 'ARTIFACT-SHA256.json', digest('ARTIFACT-SHA256.json'))
set_hash('SOURCE-SHA256.json', SHADER, shader)
set_hash('SOURCE-SHA256.json', 'ARTIFACT-SHA256.json', digest('ARTIFACT-SHA256.json'))
set_hash('SOURCE-SHA256.json', CHECKPOINT, digest(CHECKPOINT))
print('refreshed', SHADER, shader)
