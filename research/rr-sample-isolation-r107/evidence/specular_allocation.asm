
/workspace/scratch/093cfd391569/recovered-engine/renderer_rmdwin10_f(1).dll:     file format pei-x86-64


Disassembly of section .text:

0000000180128ff4 <.text+0x127ff4>:
   180128ff4:	44 8b 0d e5 a7 7e 00 	mov    r9d,DWORD PTR [rip+0x7ea7e5]        # 0x1809137e0
   180128ffb:	44 89 8d 00 01 00 00 	mov    DWORD PTR [rbp+0x100],r9d
   180129002:	49 8b 07             	mov    rax,QWORD PTR [r15]
   180129005:	48 8b 90 f8 00 00 00 	mov    rdx,QWORD PTR [rax+0xf8]
   18012900c:	48 8b 88 38 02 00 00 	mov    rcx,QWORD PTR [rax+0x238]
   180129013:	41 8b c0             	mov    eax,r8d
   180129016:	44 39 62 28          	cmp    DWORD PTR [rdx+0x28],r12d
   18012901a:	41 0f 4e c4          	cmovle eax,r12d
   18012901e:	89 44 24 20          	mov    DWORD PTR [rsp+0x20],eax
   180129022:	44 8b 05 63 54 7e 00 	mov    r8d,DWORD PTR [rip+0x7e5463]        # 0x18090e48c
   180129029:	8b 15 59 54 7e 00    	mov    edx,DWORD PTR [rip+0x7e5459]        # 0x18090e488
   18012902f:	e8 9c 9c f7 ff       	call   0x1800a2cd0
