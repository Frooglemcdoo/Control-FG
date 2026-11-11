"""Read OGI1 owned-evaluator records. Reject incomplete or incompatible captures."""
import argparse, collections, json, math, pathlib, struct
MAGIC=0x4f474931;STRIDE=256
LUMA=(.2126,.7152,.0722)
def analyze(data):
    if len(data)<16:raise ValueError('Missing OGI1 header')
    count,magic,stride,frame=struct.unpack_from('<4I',data)
    if magic!=MAGIC or stride!=STRIDE:raise ValueError('Not OGI1 schema')
    if not count or len(data)!=16+count*stride:raise ValueError('Empty, overflowed, truncated, or trailing records')
    pixels=collections.defaultdict(list)
    for i in range(count):
        raw=data[16+i*stride:16+(i+1)*stride]
        u=struct.unpack('<64I',raw);f=struct.unpack('<64f',raw)
        if u[51]!=frame:raise ValueError('Mixed/stale frame token')
        if u[3] not in (1,2,3):raise ValueError('Unknown record kind')
        if any(u[60:64]):raise ValueError('Unexpected reserved data')
        pixels[u[:2]].append((u,f))
    results=[];invalid=0
    for xy,records in pixels.items():
        if len(records)==1 and records[0][0][3]==3:
            if records[0][1][4:8]!=(0.,0.,0.,65504.):raise ValueError('Invalid receiver output differs')
            invalid+=1;continue
        rays=sorted((r for r in records if r[0][3]==1),key=lambda r:r[0][2])
        sums=[r for r in records if r[0][3]==2]
        if len(sums)!=1 or len(records)!=len(rays)+1:raise ValueError('Missing/duplicate summary')
        su,sf=sums[0];n=su[2]
        if not 0<n<=64 or len(rays)!=n or [r[0][2] for r in rays]!=list(range(n)):raise ValueError('Incomplete/duplicate ray set')
        for u,f in rays:
            if u[39]!=n or u[19] not in (1,2,4):raise ValueError('Ray count or branch mismatch')
            # Miss position and normal may be unspecified. Never reject them as radiance.
            required=list(f[4:19])+list(f[20:23])+list(f[24:27])+list(f[28:31])+list(f[56:59])
            if not all(math.isfinite(v) for v in required):raise ValueError('Nonfinite lighting data')
        if not all(math.isfinite(v) for v in sf[4:31]):raise ValueError('Nonfinite summary')
        def close(a,b):return abs(a-b)<=2e-5*max(1.,abs(a),abs(b))
        for c in range(3):
            if not close(sf[4+c],sf[15]*rays[-1][1][56+c]/n):raise ValueError('Output/accumulation mismatch')
            if not close(sf[56+c],rays[-1][1][56+c]):raise ValueError('Final running sum mismatch')
        lum=lambda v:sum(a*b for a,b in zip(v,LUMA))
        results.append({'pixel':list(xy),'rays':n,'output':list(sf[4:7]),
            'luminance':lum(sf[4:7]),'volume_ambient':list(sf[16:19]),
            'point':list(sf[20:23]),'spot':list(sf[24:27]),'miss_volume':list(sf[12:15]),
            'decomposition_residual':list(sf[28:31]),
            'ray_details':[{'index':u[2],'branch':{1:'miss_volume',2:'surface_lit',4:'surface_dark'}[u[19]],
                'material':u[35],'secondary':list(f[4:7]),'weighted':list(f[12:15]),
                'volume_ambient':list(f[16:19]),'point':list(f[20:23]),'spot':list(f[24:27]),
                'residual':list(f[28:31]),'distance':f[15]} for u,f in rays]})
    results.sort(key=lambda r:r['luminance'],reverse=True)
    return {'schema':'OGI1','frame_token':frame,'records':count,'valid_pixels':len(results),
            'invalid_receivers':invalid,'pixels':results}
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('input',type=pathlib.Path);p.add_argument('--output',type=pathlib.Path);a=p.parse_args()
    result=analyze(a.input.read_bytes());text=json.dumps(result,indent=2,allow_nan=False)+'\n'
    if a.output:a.output.write_text(text)
    else:print(text)
