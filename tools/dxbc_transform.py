"""Rebuild a DXBC container after an instruction transform, preserving other chunks."""
import struct
from dxbc_checksum import checksum

def transform_shex(data, transform):
    count = struct.unpack_from('<I',data,28)[0]
    chunks=[]
    for offset in struct.unpack_from('<'+'I'*count,data,32):
        tag=data[offset:offset+4];size=struct.unpack_from('<I',data,offset+4)[0]
        body=data[offset+8:offset+8+size]
        if tag==b'SHEX':
            w=list(struct.unpack('<'+'I'*(len(body)//4),body))
            transform(w)
            w[1]=len(w)
            body=struct.pack('<'+'I'*len(w),*w)
        chunks.append(tag+struct.pack('<I',len(body))+body)
    offset=32+4*count; offsets=[]
    for chunk in chunks: offsets.append(offset);offset+=len(chunk)
    out=bytearray(data[:28]+struct.pack('<I',count)+struct.pack('<'+'I'*count,*offsets)+b''.join(chunks))
    struct.pack_into('<I',out,24,len(out));out[4:20]=checksum(bytes(out))
    return bytes(out)
