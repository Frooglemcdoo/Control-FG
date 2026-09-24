
/mnt/data/renderer_rmdwin10_f(5).dll:     file format pei-x86-64


Disassembly of section .text:

00000001801e6b88 <.text+0x1e5b88>:
   1801e6b88:	49 8b ce             	mov    rcx,r14
   1801e6b8b:	ff 15 9f 91 3f 00    	call   QWORD PTR [rip+0x3f919f]        # 0x1805dfd30
   1801e6b91:	0f 10 00             	movups xmm0,XMMWORD PTR [rax]
   1801e6b94:	0f 11 85 a8 00 00 00 	movups XMMWORD PTR [rbp+0xa8],xmm0
   1801e6b9b:	0f 10 48 10          	movups xmm1,XMMWORD PTR [rax+0x10]
   1801e6b9f:	0f 11 8d b8 00 00 00 	movups XMMWORD PTR [rbp+0xb8],xmm1
   1801e6ba6:	0f 10 50 20          	movups xmm2,XMMWORD PTR [rax+0x20]
   1801e6baa:	66 0f 73 da 0c       	psrldq xmm2,0xc
   1801e6baf:	66 0f 7e d0          	movd   eax,xmm2
   1801e6bb3:	89 85 68 01 00 00    	mov    DWORD PTR [rbp+0x168],eax
   1801e6bb9:	8b 15 7d bc 61 00    	mov    edx,DWORD PTR [rip+0x61bc7d]        # 0x18080283c
   1801e6bbf:	b9 40 00 00 00       	mov    ecx,0x40
   1801e6bc4:	33 db                	xor    ebx,ebx
   1801e6bc6:	83 f8 01             	cmp    eax,0x1
   1801e6bc9:	65 48 8b 04 25 58 00 	mov    rax,QWORD PTR gs:0x58
   1801e6bd0:	00 00 
   1801e6bd2:	48 8b 34 d0          	mov    rsi,QWORD PTR [rax+rdx*8]
   1801e6bd6:	0f 86 dc 02 00 00    	jbe    0x1801e6eb8
