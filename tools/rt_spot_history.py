"""Bound spotlight history to current 3x3 visibility; retain native point history.

The captured shader has no previous-light ID input. This is neighborhood clipping,
not an ID validity test: mixed neighborhoods may still retain stale history.
Only use with the hash-verified R65 temporal shader and a full-distance pass.
"""
import struct
from dxbc_checksum import checksum

OP = {'mov':54, 'ftoi':27, 'iadd':30, 'imax':36, 'imin':37,
      'ld':45, 'min':51, 'max':52}

def dest(register, mask):
    return [0x00100002 | (mask << 4), register]

def source(register, swizzle='xyzw'):
    bits = sum('xyzw'.index(c) << (4 + 2*i) for i,c in enumerate(swizzle))
    return [0x00100006 | bits, register]

def immediate(values):
    return [0x00004002, *(v & 0xffffffff for v in values)]

def instruction(name, *operands):
    body = [w for operand in operands for w in operand]
    return [((len(body)+1) << 24) | OP[name], *body]

def history_instructions():
    words = []
    def emit(name, *args): words.extend(instruction(name, *args))
    # r1.z=current spotlight; r2.x=validated history; r2.y=point history.
    # r3 is dead after the native bounds check. Add r4 for dimensions and r5 for the sample address.
    emit('mov', dest(3,12), source(1,'zzzz'))
    emit('ftoi', dest(4,3), [0x00308046,0,0,0])  # cb0[0].xyxx
    emit('iadd', dest(4,3), source(4,'xyxx'), immediate([-1,-1,0,0]))
    for dy in (-1,0,1):
        for dx in (-1,0,1):
            if dx == dy == 0: continue
            emit('iadd', dest(3,3), source(0,'xyxx'), immediate([dx,dy,0,0]))
            emit('imax', dest(3,3), source(3,'xyxx'), immediate([0,0,0,0]))
            emit('imin', dest(3,3), source(3,'xyxx'), source(4,'xyxx'))
            # r5 holds an integer texture address with mip zero, then the sample.
            emit('mov', dest(5,3), source(3,'xyxx'))
            emit('mov', dest(5,12), immediate([0,0,0,0]))
            emit('ld', dest(5,1), source(5,'xyzw'), [0x00207006,3,3])
            emit('min', dest(3,4), source(3,'zzzz'), source(5,'xxxx'))
            emit('max', dest(3,8), source(3,'wwww'), source(5,'xxxx'))
    emit('max', dest(2,1), source(2,'xxxx'), source(3,'zzzz'))
    emit('min', dest(2,1), source(2,'xxxx'), source(3,'wwww'))
    return words

def bounded_history(data):
    count = struct.unpack_from('<I',data,28)[0]
    chunks=[]
    for offset in struct.unpack_from('<'+'I'*count,data,32):
        tag=data[offset:offset+4];size=struct.unpack_from('<I',data,offset+4)[0]
        body=data[offset+8:offset+8+size]
        if tag==b'SHEX':
            w=list(struct.unpack('<'+'I'*(len(body)//4),body))
            assert w[69]&2047==104 and w[70]==4, 'Native temp declaration changed'
            assert w[316]&2047==0 and w[324]&2047==50, 'Native history blend changed'
            w[70]=6
            w[316:316]=history_instructions()
            w[1]=len(w)
            body=struct.pack('<'+'I'*len(w),*w)
        chunks.append(tag+struct.pack('<I',len(body))+body)
    offset=32+4*count; offsets=[]
    for chunk in chunks: offsets.append(offset);offset+=len(chunk)
    out=bytearray(data[:28]+struct.pack('<I',count)+struct.pack('<'+'I'*count,*offsets)+b''.join(chunks))
    struct.pack_into('<I',out,24,len(out));out[4:20]=checksum(bytes(out))
    return bytes(out)
