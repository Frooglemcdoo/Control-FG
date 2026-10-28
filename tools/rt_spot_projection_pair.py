"""Independent CPU reconstruction of capture-only emitter/projection segments."""
import math
from rt_spot_authored_clip import clip_distance

def dot(a,b): return sum(x*y for x,y in zip(a,b))
def cross(a,b): return [a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]]
def norm(a): return math.sqrt(dot(a,a))
def world(v,rows,point=True): return [sum(v[j]*rows[j][i] for j in range(3))+(rows[3][i] if point else 0) for i in range(3)]
def segments(record):
    ctx=record['light_context'];rows=record['spot_record']['projection_rows']
    a,b,c=([rows[i][j] for i in range(3)] for j in (0,1,3))
    determinant=dot(a,cross(b,c));scale=norm(a)*norm(b)*norm(c)
    if not math.isfinite(scale) or scale<=0 or not math.isfinite(determinant) or abs(determinant)<=scale*1e-6:
        raise ValueError('Unsupported projection apex')
    vectors=[cross(b,c),cross(c,a),cross(a,b)]
    apex=[sum(-rows[3][j]*v[i] for j,v in zip((0,1,3),vectors))/determinant for i in range(3)]
    view=ctx['view_to_world'];cone=world(ctx['cone_view'],view);axis=world(ctx['direction_view'],view,False)
    origin=record['origin'];near=ctx['near_distance'];out=[]
    for target in (world(ctx['emitter_view'],view),world(apex,view)):
        delta=[b-a for a,b in zip(origin,target)];distance=norm(delta)
        if not all(math.isfinite(v) for v in (*origin,*target,*cone,*axis,near,distance)) or distance<=1e-6 or norm(axis)<=1e-6 or near<0:
            raise ValueError('Unsupported projection segment')
        out.append(dict(direction=[x/distance for x in delta],t_max=clip_distance(origin,target,cone,axis,near)))
    return out

def validate_pair(record,pair):
    if not pair[0]['valid'] and not pair[1]['valid']:return {'supported':False}
    if not all(r['valid'] for r in pair):raise ValueError('Partial projection pair')
    expected=segments(record)
    for probe,geometry in zip(pair,expected):
        for key in ('origin','pixel','light_id','frame_tag'):
            if probe[key]!=record[key]:raise ValueError('Projection pair identity mismatch')
        if any(abs(a-b)>2e-4 for a,b in zip(probe['direction'],geometry['direction'])) or abs(probe['t_max']-geometry['t_max'])>max(2e-4,abs(geometry['t_max'])*2e-4):
            raise ValueError('Projection pair does not match captured light geometry')
    return dict(supported=True,emitter=pair[0],projection=pair[1])
