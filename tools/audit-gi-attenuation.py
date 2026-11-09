"""Read-only comparison of native spotlight attenuation across three shader families."""
import argparse, hashlib, importlib.util, json, re, struct, tempfile, zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('decoder',ROOT/'research/native-integration-audit/tools/decode_dxbc.py')
decoder=importlib.util.module_from_spec(spec);spec.loader.exec_module(decoder)

def inspect(raw,path):
    path.write_bytes(raw);d=decoder.decode(path);a=d['instructions'];records=[]
    bindings=[b for b in d['bindings'] if b['name']=='g_sbDeferredLightSpotData']
    if not bindings:return records
    assert len(bindings)==1
    binding=bindings[0]
    for j,x in enumerate(a):
        if x['name']!='max' or x['operands'][-1]!='l(0.25)':continue
        if [v['name'] for v in a[j-3:j+5]]!=['dp3','mul','mad','max','mul','rcp','add','max']:continue
        dot,square,mad,floor,pi,rcp,sub,clamp=a[j-3:j+5]
        dist=dot['operands'][0];radius=square['operands'][0]
        coeff=re.fullmatch(r'(r\[\d+\])\.y',square['operands'][1])[1]
        assert dot['operands'][1]==dot['operands'][2]
        expected=[('mul',[radius,coeff+'.y',coeff+'.y']),
                  ('mad',[dist,coeff+'.z',dist,radius]),
                  ('max',[dist,dist,'l(0.25)']),
                  ('mul',[dist,dist,'l(3.14159274)']),
                  ('rcp',[dist,dist]),('add',[dist,'-'+coeff+'.x',dist]),
                  ('max',[dist,dist,'l(0)'])]
        assert [(v['name'],v['operands']) for v in a[j-2:j+5]]==expected
        loads=[k for k,v in enumerate(a[:j-3]) if v['name']=='ld_structured'
               and v['operands'][0]==coeff+'.xyzw' and v['operands'][2]=='l(68)'
               and v['operands'][3].startswith(f"t[{binding['identifier']}][")]
        assert loads
        load=loads[-1]
        # The x/y/z coefficients must reach this block without being overwritten.
        for v in a[load+1:j-3]:
            if not v['operands']:continue
            m=re.fullmatch(re.escape(coeff)+r'\.([xyzw]+)',v['operands'][0])
            assert not m or not (set(m[1]) & set('xyz')), (v,coeff)
        records.append(dict(shader_sha256=hashlib.sha256(raw).hexdigest(),bytes=len(raw),
                            coefficient_load=a[load],binding=binding,
                            attenuation_block=a[j-3:j+5],
                            formula='max(1 / (pi * max(c76 * distanceSquared + c72*c72, 0.25)) - c68, 0)'))
    return records

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('archive',type=Path);p.add_argument('--output',type=Path,required=True);args=p.parse_args()
    result=dict(archive_sha256=hashlib.sha256(args.archive.read_bytes()).hexdigest(),families={})
    with zipfile.ZipFile(args.archive) as z,tempfile.TemporaryDirectory() as td:
        for family,expected in [('deferredlight',8),('rt_deferred_reflection',4),('rt_deferred_diffusegi',8)]:
            name=next(n for n in z.namelist() if n.replace('\\','/').endswith('/'+family+'.obj'))
            data=z.read(name);offset=0;index=0;rows=[]
            while True:
                offset=data.find(b'DXBC',offset)
                if offset<0:break
                size=struct.unpack_from('<I',data,offset+24)[0]
                assert size>=32 and offset+size<=len(data)
                raw=data[offset:offset+size];offset+=size
                for row in inspect(raw,Path(td)/'shader.dxbc'):
                    row['variant']=index;rows.append(row)
                index+=1
            assert len(rows)==expected,(family,len(rows))
            result['families'][family]=dict(total_containers=index,matched_variants=rows)
            print(f'PASS {family}: {len(rows)} variants, identical formula and unchanged coefficients from spot record bytes 68/72/76')
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text(json.dumps(result,indent=2)+'\n')
if __name__=='__main__':main()
