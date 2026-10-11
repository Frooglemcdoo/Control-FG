"""Check storage precision and record boundaries for the private hit contract."""
import math, random, struct
random.seed(31517)
def sign(x): return 1 if x>=0 else -1
def pack(n):
    x,y,z=[a/sum(map(abs,n)) for a in n]
    if z<0:x,y=(1-abs(y))*sign(x),(1-abs(x))*sign(y)
    a,b=[round(max(0,min(1,p*.5+.5))*65535) for p in (x,y)]
    return a|(b<<16)
def unpack(p):
    x=(p&65535)*2/65535-1;y=(p>>16)*2/65535-1;z=1-abs(x)-abs(y)
    if z<0:x,y=(1-abs(y))*sign(x),(1-abs(x))*sign(y)
    length=math.sqrt(x*x+y*y+z*z)
    return [v/length for v in (x,y,z)]
worst=0
for _ in range(50000):
    n=[random.uniform(-1,1) for _ in range(3)];length=math.sqrt(sum(v*v for v in n));n=[v/length for v in n]
    decoded=unpack(pack(n));angle=math.degrees(math.acos(max(-1,min(1,sum(a*b for a,b in zip(n,decoded))))))
    worst=max(worst,angle)
assert worst<.005,worst
fmt='<3fI2fII'
assert struct.calcsize(fmt)==32
for material in (0,2048,32767,65533,65535):
    values=(135.025,-62.875,3.5001,material,-12.25,7.8125,pack([0,0,1]),0)
    result=struct.unpack(fmt,struct.pack(fmt,*values))
    assert result[3:]==values[3:]
    assert max(abs(a-b) for a,b in zip(result[:3],values[:3]))<.00001
capacity=8388608;size=capacity*96
assert size==768*1024*1024
for width,height in [(1920,1080),(2560,1440),(3840,2160)]:
    last=width*height*3-1
    assert last*32+32<=size
print(f'PASS: 32-byte hits; full UV/material round trip; oct normal error {worst:.6f} degrees; 4K bounds.')
