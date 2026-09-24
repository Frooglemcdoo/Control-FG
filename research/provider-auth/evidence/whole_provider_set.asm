
../../upload/d3d_rmdwin10_f(4).dll:     file format pei-x86-64


Disassembly of section .text:

0000000180016a10 <.text+0x15a10>:
   180016a10:	48 83 ec 38          	sub    rsp,0x38
   180016a14:	4d 8b c8             	mov    r9,r8
   180016a17:	48 63 c1             	movsxd rax,ecx
   180016a1a:	48 03 c0             	add    rax,rax
   180016a1d:	4c 89 4c 24 20       	mov    QWORD PTR [rsp+0x20],r9
   180016a22:	4c 8b ca             	mov    r9,rdx
   180016a25:	4c 8d 05 1c be 0f 00 	lea    r8,[rip+0xfbe1c]        # 0x180112848
   180016a2c:	33 d2                	xor    edx,edx
   180016a2e:	4d 63 04 c0          	movsxd r8,DWORD PTR [r8+rax*8]
   180016a32:	e8 49 ff ff ff       	call   0x180016980
   180016a37:	48 83 c4 38          	add    rsp,0x38
   180016a3b:	c3                   	ret
