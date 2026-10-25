"""Extract a complete R148 shader snapshot; verify bytes before creating files."""
import argparse
import hashlib
import json
import re
from pathlib import Path


def decode(text):
    # Each Save Log starts a new export. A partial final export must be reported,
    # not silently replaced by an older snapshot that may omit new shaders.
    lines=text.splitlines()
    starts=[i for i,line in enumerate(lines) if line.startswith('shader_snapshot_begin ')]
    if not starts: raise ValueError('No shader snapshot in log')
    def fields(line): return dict(re.findall(r'(\w+)=([^\s]+)',line))
    begin=fields(lines[starts[-1]])
    serial=int(begin['export']);count=int(begin['count']);dropped=int(begin['dropped'])
    if serial<1 or not 0<=count<=512 or dropped<0: raise ValueError('Invalid snapshot header')
    entries=[];current=None;ended=False;total=0
    for line in lines[starts[-1]+1:]:
        if not line.startswith('shader_snapshot_'): continue
        data=fields(line)
        if int(data['export'])!=serial: raise ValueError('Mixed snapshot exports')
        if line.startswith('shader_snapshot_entry '):
            if current is not None and len(current['data'])!=current['bytes']: raise ValueError('Truncated shader')
            index=int(data['index']);stage=int(data['stage']);size=int(data['bytes']);sha=data['sha']
            if index!=len(entries) or index>=count or stage not in (1,2) or not 4<=size<=256*1024 or not re.fullmatch('[0-9a-f]{64}',sha):
                raise ValueError('Invalid shader metadata')
            total+=size
            if total>8*1024*1024: raise ValueError('Snapshot exceeds memory budget')
            current=dict(index=index,stage=stage,bytes=size,sha=sha,data=bytearray());entries.append(current)
        elif line.startswith('shader_snapshot_data '):
            if current is None or int(data['index'])!=current['index'] or int(data['offset'])!=len(current['data']):
                raise ValueError('Noncontiguous or duplicate shader chunk')
            encoded=data['hex']
            if not re.fullmatch('(?:[0-9a-f]{2}){1,128}',encoded): raise ValueError('Invalid shader chunk')
            current['data'].extend(bytes.fromhex(encoded))
            if len(current['data'])>current['bytes']: raise ValueError('Oversized shader')
        elif line.startswith('shader_snapshot_end '):
            if int(data['count'])!=count or len(entries)!=count: raise ValueError('Missing shaders')
            ended=True;break
        else: raise ValueError('Unknown shader snapshot record')
    if not ended: raise ValueError('Snapshot export did not finish')
    for entry in entries:
        blob=bytes(entry['data'])
        if len(blob)!=entry['bytes'] or not blob.startswith(b'DXBC') or hashlib.sha256(blob).hexdigest()!=entry['sha']:
            raise ValueError('Shader size, container or SHA256 mismatch')
        entry['data']=blob
    return entries,dropped


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('log',type=Path);parser.add_argument('output',type=Path)
    args=parser.parse_args()
    entries,dropped=decode(args.log.read_text(encoding='utf-8-sig'))
    args.output.mkdir(parents=True,exist_ok=False)
    manifest=[]
    for entry in entries:
        name=f"{entry['index']:03d}-{'ps' if entry['stage']==1 else 'cs'}-{entry['sha']}.dxbc"
        (args.output/name).write_bytes(entry['data'])
        manifest.append({k:v for k,v in entry.items() if k!='data'}|{'file':name})
    (args.output/'manifest.json').write_text(json.dumps(dict(shaders=manifest,dropped=dropped),indent=2)+'\n')
    print(f'Extracted {len(entries)} verified shaders; skipped observations: {dropped}')

if __name__=='__main__':main()
