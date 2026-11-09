"""Extract bounded rejected vertex pairs; report byte differences, not equivalence.

Usage: python extract-vertex-evidence.py material-rejection-audit.json EMPTY_OUTPUT_DIR
The frame is the earlier snapshot's frame, not the separate live capture.
"""
from pathlib import Path
import hashlib,json,struct,sys

def chunks(blob):
    if len(blob)<32 or blob[:4]!=b'DXBC': return {'error':'not_a_dxbc_container'}
    size,count=struct.unpack_from('<II',blob,24)
    if size!=len(blob) or count>32 or 32+count*4>size: return {'error':'invalid_dxbc_header'}
    result={};spans=[]
    for i in range(count):
        offset=struct.unpack_from('<I',blob,32+i*4)[0]
        if offset<32+count*4 or offset+8>size: return {'error':'invalid_chunk_offset'}
        tag=blob[offset:offset+4].decode('ascii',errors='replace')
        n=struct.unpack_from('<I',blob,offset+4)[0];end=offset+8+n
        if end>size or tag in result or any(offset<b and a<end for a,b in spans):
            return {'error':'invalid_chunk_extent_or_duplicate'}
        spans.append((offset,end));result[tag]=hashlib.sha256(blob[offset+8:end]).hexdigest()
    return result

def extract(source,out):
    if source.stat().st_size>8*1024*1024: raise ValueError('audit size exceeds cap')
    audit=json.loads(source.read_text());frame=audit['engine_frame']
    assert audit['schema']=='ControlFG.RRMaterialRejectionAudit.v4' and type(frame) is int and frame>0
    e=audit['rejected_vertex_evidence']
    assert e['schema']=='ControlFG.RejectedVertexPairs.v1' and e['encoding']=='hex'
    assert len(e['pairs'])<=32 and len(e['batches'])<=256
    payload=[];total=0;summary=[]
    for index,pair in enumerate(e['pairs']):
        assert pair['index']==index
        programs={}
        for name in ('original_vs','replacement_vs','replacement_ps'):
            text=pair[name];assert type(text) is str and len(text)%2==0 and len(text)<=524288
            assert all(c in '0123456789abcdef' for c in text)
            blob=bytes.fromhex(text);assert 0<len(blob)<=262144
            total+=len(blob);assert total<=2*1024*1024
            filename=f'pair-{index:03d}-{name}.dxbc';payload.append((filename,blob))
            programs[name]={'file':filename,'bytes':len(blob),'sha256':hashlib.sha256(blob).hexdigest(),'chunks':chunks(blob)}
        a=programs['original_vs']['chunks'];b=programs['replacement_vs']['chunks']
        summary.append({'index':index,'programs':programs,'different_chunks':sorted(k for k in a.keys()|b.keys() if a.get(k)!=b.get(k)),
                        'vertex_equivalence':'NOT_PROVEN: chunk equality/difference is diagnostic only'})
    assert total==e['raw_bytes']
    for ref in e['batches']:
        assert type(ref['source_index']) is int and 0<=ref['source_index']<audit['source_batches']
        assert ref['pair_index'] is None or (type(ref['pair_index']) is int and 0<=ref['pair_index']<len(summary))
    if out.exists() and any(out.iterdir()): raise ValueError('output directory must be empty')
    out.mkdir(parents=True,exist_ok=True)
    for name,blob in payload:(out/name).write_bytes(blob)
    report={'snapshot_frame':frame,'pairs':summary,'batches':e['batches'],
            'omitted_pair_attempts':e['omitted_pair_attempts'],'omitted_batch_references':e['omitted_batch_references'],
            'audit_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'rr_ready':False}
    (out/'summary.json').write_text(json.dumps(report,indent=2)+'\n')
    print(f'Extracted {len(summary)} pairs, {len(e["batches"])} batch references from snapshot frame {frame}; RR readiness not inferred.')

if __name__=='__main__':
    if len(sys.argv)!=3: raise SystemExit(__doc__)
    extract(Path(sys.argv[1]),Path(sys.argv[2]))
