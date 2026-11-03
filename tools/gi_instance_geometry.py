"""Offline DXR triangle decoder. Requires immutable, same-generation buffer bytes.
Never dereferences a process/GPU address. Does not establish hit identity itself.
"""
import math
import struct
from dataclasses import dataclass

@dataclass(frozen=True)
class Snapshot:
    generation: int
    base: int
    data: bytes

    def read(self, address, size, generation):
        if generation != self.generation:
            raise ValueError('stale snapshot')
        if self.base < 0 or address < self.base or size < 0 or address + size > 1 << 64:
            raise ValueError('invalid address range')
        offset = address - self.base
        if offset + size > len(self.data):
            raise ValueError('snapshot range unavailable')
        return self.data[offset:offset + size]

@dataclass(frozen=True)
class Instance:
    transform: tuple
    instance_id: int
    mask: int
    hit_group: int
    flags: int
    blas: int


def decode_instance(data):
    if len(data) != 64:
        raise ValueError('instance record must be 64 bytes')
    values = struct.unpack('<12fIIQ', data)
    if not all(math.isfinite(v) for v in values[:12]):
        raise ValueError('nonfinite instance transform')
    if not values[14] or values[14] % 256:
        raise ValueError('invalid BLAS address')
    return Instance(values[:12], values[12] & 0xffffff, values[12] >> 24,
                    values[13] & 0xffffff, values[13] >> 24, values[14])


def transform(point, matrix):
    if len(matrix) != 12 or not all(math.isfinite(v) for v in (*point, *matrix)):
        raise ValueError('invalid transform input')
    result = tuple(sum(matrix[4*r+c]*point[c] for c in range(3)) + matrix[4*r+3] for r in range(3))
    if not all(math.isfinite(v) for v in result):
        raise ValueError('nonfinite transformed point')
    return result


def decode_triangle(*, vertices, indices, vertex_address, index_address,
                    vertex_count, index_count, stride, vertex_format, index_format,
                    primitive, instance, generation, geometry_transform=None):
    # Float3 with aligned packed/padded stride; explicit 16/32-bit indices.
    # Fail closed on other layouts instead of guessing the position offset.
    if vertex_format != 6 or index_format not in (57, 42) or stride < 12 or stride % 4:
        raise ValueError('unaudited geometry format')
    if min(vertex_count, index_count, primitive) < 0 or index_count % 3:
        raise ValueError('invalid triangle counts')
    if primitive >= index_count // 3:
        raise ValueError('primitive outside geometry')
    if instance.mask == 0:
        raise ValueError('disabled instance')
    width = 2 if index_format == 57 else 4
    if vertex_address % 4 or index_address % width:
        raise ValueError('unaligned geometry address')
    ids = struct.unpack('<3H' if width == 2 else '<3I', indices.read(index_address + 3*width*primitive, 3*width, generation))
    if any(i >= vertex_count for i in ids):
        raise ValueError('index outside vertex range')
    points = []
    for i in ids:
        p = struct.unpack('<3f', vertices.read(vertex_address + stride*i, 12, generation))
        if geometry_transform is not None:
            p = transform(p, geometry_transform)
        points.append(transform(p, instance.transform))
    a,b,c = points
    u = tuple(b[i]-a[i] for i in range(3)); v = tuple(c[i]-a[i] for i in range(3))
    scale = max(abs(x) for x in (*u,*v))
    if not math.isfinite(scale) or scale == 0:
        raise ValueError('degenerate triangle')
    u = tuple(x/scale for x in u); v = tuple(x/scale for x in v)
    n = (u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0])
    length = math.sqrt(sum(x*x for x in n))
    if length == 0 or not math.isfinite(length):
        raise ValueError('degenerate triangle')
    return tuple(points), tuple(x/length for x in n)
