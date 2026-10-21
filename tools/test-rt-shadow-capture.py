import hashlib
import importlib.util
from pathlib import Path
spec=importlib.util.spec_from_file_location('capture',Path(__file__).with_name('extract-rt-shadow-capture.py'))
capture=importlib.util.module_from_spec(spec);spec.loader.exec_module(capture)
data=bytes(range(256));sha=hashlib.sha256(data).hexdigest()
begin=f'rt_blob_begin kind=direct_cs sha={sha} bytes=256\n'
rows=''.join(f'rt_blob_data sha={sha} offset={offset} hex={data[offset:offset+128].hex()}\n' for offset in (0,128))
end=f'rt_blob_end sha={sha}\n'
text=begin+rows+end
assert capture.extract('ms=0 unrelated log line\n'+text)[sha]==('direct_cs',data)
assert capture.extract(text.replace('kind=direct_cs', 'kind=compute_cs'))[sha] == ('compute_cs', data)
bad=[begin+rows,text.replace('offset=128','offset=127'),text.replace('hex=0001','hex=ff01'),
     text.replace('bytes=256','bytes=257'),text.replace('kind=direct_cs','kind=../../escape'),
     text.replace('hex=0001','hex=0g01'),text+text,rows+end]
for broken in bad:
    try: capture.extract(broken)
    except ValueError: pass
    else: raise AssertionError('Invalid capture accepted')
print('PASS: exact recovery; truncation, corruption, gaps, duplicates and unsafe names rejected')
