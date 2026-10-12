#!/usr/bin/env python3
"""Index PIX's explicit SBT reconstruction records; never infer position streams."""
import argparse,collections,hashlib,json,pathlib,re
FUNCTION=re.compile(r'void CreateShaderTable_(\d+)\([^\n]+\)\s*\{')
RECORD=re.compile(r'GetShaderIdentifier\(LR"\(([^)]*)\)"\).*?&output\[(\d+)\]\);\s*static std::unique_ptr<ShaderTableBuilder>.*?AddGpuva\(GetGpuva\((\d+), (\d+)\)\);\s*shaderTableBuilder->AddGpuva\(GetGpuva\((\d+), (\d+)\)\);\s*shaderTableBuilder->AddRootConstants\(\{([^}]+)\}\);',re.S)
def parse(text,source):
    starts=list(FUNCTION.finditer(text));records=[]
    for n,start in enumerate(starts):
        body=text[start.end():starts[n+1].start() if n+1<len(starts) else len(text)]
        declared=body.count('AddRootConstants(');found=list(RECORD.finditer(body))
        if declared!=len(found):raise ValueError(f'{source} function {start[1]}: {declared} constant records, {len(found)} parsed')
        for m in found:
            constants=[int(x.strip()) for x in m[7].split(',')]
            if len(constants)!=10:raise ValueError('Unsupported native constant layout')
            records.append(dict(function=int(start[1]),record_offset=int(m[2]),export=m[1],index_resource=int(m[3]),index_base=int(m[4]),attribute_resource=int(m[5]),attribute_base=int(m[6]),constants=constants,source=source))
    return records

def main():
    ap=argparse.ArgumentParser();ap.add_argument('source',type=pathlib.Path);ap.add_argument('output',type=pathlib.Path);args=ap.parse_args()
    records=[];sources={}
    for p in sorted(args.source.glob('ShaderTableReconstruction_*.cpp')):
        data=p.read_bytes();sources[p.name]=hashlib.sha256(data).hexdigest();records.extend(parse(data.decode('utf-8-sig'),p.name))
    if not records:raise ValueError('No explicit shader-table records found')
    unique={}
    for r in records:
        key=(r['export'],r['index_resource'],r['index_base'],r['attribute_resource'],r['attribute_base'],*r['constants'])
        unique.setdefault(key,r)
    result=dict(schema=1,source_hashes=sources,record_count=len(records),unique_count=len(unique),
        material_count=len({r['constants'][3] for r in records}),
        index_strides=dict(collections.Counter(str(r['constants'][1]) for r in records)),
        attribute_strides=dict(collections.Counter(str(r['constants'][2]) for r in records)),
        exports=dict(collections.Counter(r['export'] for r in records)),
        position_mapping='UNRESOLVED: local roots contain index and attribute addresses only; never guess a position stream',
        records=list(unique.values()))
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(result,indent=2)+'\n')
    print(json.dumps({k:v for k,v in result.items() if k not in ('records','source_hashes')},indent=2))
if __name__=='__main__':main()
