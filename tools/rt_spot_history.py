"""Bound spotlight history to current 3x3 visibility; retain native point history.

The captured shader has no previous-light ID input. This is neighborhood clipping,
not an ID validity test: mixed neighborhoods may still retain stale history.
Only use with the hash-verified R65 temporal shader and a full-distance pass.
"""
from dxbc_transform import transform_shex

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
    def transform(w):
        assert w[69]&2047==104 and w[70]==4, 'Native temp declaration changed'
        assert w[316]&2047==0 and w[324]&2047==50, 'Native history blend changed'
        w[70]=6
        w[316:316]=history_instructions()
    return transform_shex(data, transform)


def current_spotlight_history(data):
    """Diagnostic: remove spotlight history, preserving native point blending.

    Before the native history-minus-current operation, replace spotlight history
    with current spotlight visibility. x/z/w are spotlight copies; y is point.
    Spatial filtering still follows. No extra resource or render-thread work.
    """
    def transform(words):
        assert words[316:324] == [0x08000000, 0x001000f2, 2,
            0x80100ae6, 0x41, 1, 0x00100e46, 2], 'Native history delta changed'
        words[316:316] = instruction('mov', dest(2,13), source(1,'zzzz'))
    return transform_shex(data, transform)
