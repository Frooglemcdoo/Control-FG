from pathlib import Path
import struct, subprocess, re

class PE:
    def __init__(self,path):
        self.path=Path(path);self.data=self.path.read_bytes();d=self.data
        h=struct.unpack_from('<I',d,0x3c)[0];o=h+24
        assert d[h:h+4]==b'PE\0\0' and struct.unpack_from('<H',d,o)[0]==0x20b
        self.base=struct.unpack_from('<Q',d,o+24)[0]
        n=struct.unpack_from('<H',d,h+6)[0];sz=struct.unpack_from('<H',d,h+20)[0]
        self.sections=[]
        for i in range(n):
            p=o+sz+i*40;vs,va,rs,rp=struct.unpack_from('<IIII',d,p+8)
            self.sections.append((d[p:p+8].rstrip(b'\0').decode(),va,rs,rp))
        self.directories=[struct.unpack_from('<II',d,o+112+i*8) for i in range(16)]
        self.exports={};self.imports={}
        rva,size=self.directories[0]
        if rva:
            p=self.off(rva);v=struct.unpack_from('<IIHHIIIIIII',d,p)
            _,_,_,_,_,ordinal,nfunc,nnames,func,names,ords=v
            for i in range(nnames):
                name=self.string(self.u32(names+i*4));idx=self.u16(ords+i*2)
                self.exports[name]=self.u32(func+idx*4)
        rva,size=self.directories[1]
        if rva:
            p=self.off(rva)
            while True:
                original,_,_,name,first=struct.unpack_from('<IIIII',d,p)
                if not any((original,name,first)):break
                q=original or first;i=0
                while self.u64(q+8*i):
                    val=self.u64(q+8*i)
                    self.imports[first+8*i]=self.string(val+2) if not val>>63 else 'ordinal:'+str(val&65535)
                    i+=1
                p+=20
        self.functions=[]
        rva,size=self.directories[3]
        if rva:
            p=self.off(rva)
            for off in range(0,size,12):
                a,b,u=struct.unpack_from('<III',d,p+off)
                self.functions.append((a,b))
    def off(self,rva):
        for name,va,rs,rp in self.sections:
            if va<=rva<va+rs:return rp+rva-va
        raise ValueError(hex(rva))
    def u16(self,rva):return struct.unpack_from('<H',self.data,self.off(rva))[0]
    def u32(self,rva):return struct.unpack_from('<I',self.data,self.off(rva))[0]
    def u64(self,rva):return struct.unpack_from('<Q',self.data,self.off(rva))[0]
    def string(self,rva):
        p=self.off(rva);return self.data[p:self.data.index(b'\0',p)].decode('ascii')
    def disasm(self,a,b=None):
        if b is None:
            bounds=next(((s,e) for s,e in self.functions if s<=a<e),None)
            if bounds:a,b=bounds
            else:b=a+128
        return subprocess.check_output(['objdump','-d','-M','intel',f'--start-address={self.base+a}',f'--stop-address={self.base+b}',str(self.path)],text=True)
    def all_disasm(self):
        return subprocess.check_output(['objdump','-d','-M','intel',str(self.path)],text=True)

if __name__=='__main__':
    import sys
    p=PE(sys.argv[1]);pattern=sys.argv[2]
    for name,addr in p.exports.items():
        if re.search(pattern,name):
            print(name,hex(addr));print(p.disasm(addr))
