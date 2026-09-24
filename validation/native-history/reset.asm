
/workspace/scratch/65fb49337344/upload/renderer_rmdwin10_f(4).dll:     file format pei-x86-64


Disassembly of section .text:

0000000180013e20 <.text+0x12e20>:
   180013e20:	48 83 ec 38          	sub    rsp,0x38
   180013e24:	48 c7 44 24 20 fe ff 	mov    QWORD PTR [rsp+0x20],0xfffffffffffffffe
   180013e2b:	ff ff 
   180013e2d:	48 8b 15 e4 ec 8f 00 	mov    rdx,QWORD PTR [rip+0x8fece4]        # 0x180912b18
   180013e34:	48 c7 05 d9 ec 8f 00 	mov    QWORD PTR [rip+0x8fecd9],0x0        # 0x180912b18
   180013e3b:	00 00 00 00 
   180013e3f:	48 85 d2             	test   rdx,rdx
   180013e42:	74 0d                	je     0x180013e51
   180013e44:	48 8b 0d 8d ea 7e 00 	mov    rcx,QWORD PTR [rip+0x7eea8d]        # 0x1808028d8
   180013e4b:	e8 b0 5f 09 00       	call   0x1800a9e00
   180013e50:	90                   	nop
   180013e51:	48 8b 15 c8 ec 8f 00 	mov    rdx,QWORD PTR [rip+0x8fecc8]        # 0x180912b20
   180013e58:	48 c7 05 bd ec 8f 00 	mov    QWORD PTR [rip+0x8fecbd],0x0        # 0x180912b20
   180013e5f:	00 00 00 00 
   180013e63:	48 85 d2             	test   rdx,rdx
   180013e66:	74 0d                	je     0x180013e75
   180013e68:	48 8b 0d 69 ea 7e 00 	mov    rcx,QWORD PTR [rip+0x7eea69]        # 0x1808028d8
   180013e6f:	e8 8c 5f 09 00       	call   0x1800a9e00
   180013e74:	90                   	nop
   180013e75:	48 83 c4 38          	add    rsp,0x38
   180013e79:	c3                   	ret
