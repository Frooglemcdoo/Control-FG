
../../upload/renderer_rmdwin10_f(4).dll:     file format pei-x86-64


Disassembly of section .text:

0000000180128dff <.text+0x127dff>:
   180128dff:	8b 35 37 9a 6d 00    	mov    esi,DWORD PTR [rip+0x6d9a37]        # 0x18080283c
   180128e05:	80 3d 42 33 6c 00 00 	cmp    BYTE PTR [rip+0x6c3342],0x0        # 0x1807ec14e
   180128e0c:	74 2c                	je     0x180128e3a
   180128e0e:	65 48 8b 04 25 58 00 	mov    rax,QWORD PTR gs:0x58
   180128e15:	00 00 
   180128e17:	ba 08 00 00 00       	mov    edx,0x8
   180128e1c:	48 8b 04 f0          	mov    rax,QWORD PTR [rax+rsi*8]
   180128e20:	48 8b 0c 02          	mov    rcx,QWORD PTR [rdx+rax*1]
   180128e24:	80 b9 b0 04 00 00 00 	cmp    BYTE PTR [rcx+0x4b0],0x0
   180128e2b:	74 0d                	je     0x180128e3a
