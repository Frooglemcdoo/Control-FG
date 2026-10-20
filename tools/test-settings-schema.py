"""Reject catalog edits that would generate divergent or unusable controls."""
import copy
import importlib.util
import json
from pathlib import Path

root = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('generator', root / 'tools/generate-settings-schema.py')
generator = importlib.util.module_from_spec(spec)
spec.loader.exec_module(generator)
original = json.loads((root / 'src/settings_schema.json').read_text())
generator.generate(original)

def rejects(change):
    schema = copy.deepcopy(original)
    change(schema)
    try:
        generator.generate(schema)
    except (AssertionError, KeyError, ValueError):
        return
    raise AssertionError('Invalid catalog accepted')

rejects(lambda s: s['protocol'].update(stateBits=32))
rejects(lambda s: s['options'][1].update(id=s['options'][0]['id']))
rejects(lambda s: s['options'][1].update(localizationKey=s['options'][0]['localizationKey']))
rejects(lambda s: s['options'][2].update(step=7)) # 144 is not on the 30 + 7n grid.
rejects(lambda s: s['options'][1].update(requires=[{'key':'DynamicTargetMode','value':0}]))
rejects(lambda s: s['options'][1].update(requires=[{'key':'FrameGeneration','value':99}]))
rejects(lambda s: s['options'][1].update(storage={'section':'framegeneration','key':'mode'}))
rejects(lambda s: s['options'][1].update(visibility='typo'))
print('PASS catalog validation: IDs, labels, step alignment, dependency values/cycles, INI ownership and policies')
