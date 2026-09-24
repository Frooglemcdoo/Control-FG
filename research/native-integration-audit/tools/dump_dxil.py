"""Print DXIL LLVM IR using the system LLVM C API; no shader execution."""
import ctypes as C, pathlib, struct, sys
p=pathlib.Path(sys.argv[1]);d=p.read_bytes()
assert d[:4]==b'DXBC'
chunks=[]
for o in struct.unpack_from('<'+'I'*struct.unpack_from('<I',d,28)[0],d,32):
 if d[o:o+4]==b'DXIL':chunks.append(d[o+8:o+8+struct.unpack_from('<I',d,o+4)[0]])
assert len(chunks)==1
x=chunks[0];assert x[8:12]==b'DXIL'
o,n=struct.unpack_from('<II',x,16);bc=x[8+o:8+o+n];assert len(bc)==n and bc[:4]==b'BC\xc0\xde'
l=C.CDLL('libLLVM.so.20.1')
def api(name,args,rest):
 f=getattr(l,name);f.argtypes=args;f.restype=rest;return f
ptr=C.c_void_p
buf=api('LLVMCreateMemoryBufferWithMemoryRangeCopy',[C.c_char_p,C.c_size_t,C.c_char_p],ptr)(bc,len(bc),b'dxil')
mod=ptr();err=ptr()
parse=api('LLVMParseBitcode',[ptr,C.POINTER(ptr),C.POINTER(ptr)],C.c_int)
if parse(buf,C.byref(mod),C.byref(err)):
 raise RuntimeError(C.string_at(err).decode() if err.value else 'LLVM parse failed')
s=api('LLVMPrintModuleToString',[ptr],ptr)(mod)
print(C.string_at(s).decode())
api('LLVMDisposeMessage',[ptr],None)(s)
api('LLVMDisposeModule',[ptr],None)(mod)
api('LLVMDisposeMemoryBuffer',[ptr],None)(buf)
