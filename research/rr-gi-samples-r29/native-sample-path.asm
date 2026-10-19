
renderer_rmdwin10_f.dll:     file format pei-x86-64


Disassembly of section .text:

0000000180006280 <.text+0x5280>:
   180006280:	48 83 ec 28          	sub    rsp,0x28
   180006284:	41 b8 01 00 00 00    	mov    r8d,0x1
   18000628a:	48 8d 15 d7 8c 62 00 	lea    rdx,[rip+0x628cd7]        # 0x18062ef68
   180006291:	48 8d 0d 88 d8 90 00 	lea    rcx,[rip+0x90d888]        # 0x180913b20
   180006298:	ff 15 8a b0 5d 00    	call   QWORD PTR [rip+0x5db08a]        # 0x1805e1328
   18000629e:	66 0f 6f 05 2a 96 67 	movdqa xmm0,XMMWORD PTR [rip+0x67962a]        # 0x18067f8d0
   1800062a5:	00 
   1800062a6:	48 8d 05 2b bc 61 00 	lea    rax,[rip+0x61bc2b]        # 0x180621ed8
   1800062ad:	48 8d 0d 7c 6d 5c 00 	lea    rcx,[rip+0x5c6d7c]        # 0x1805cd030
   1800062b4:	48 89 05 65 d8 90 00 	mov    QWORD PTR [rip+0x90d865],rax        # 0x180913b20
   1800062bb:	66 0f 7f 05 0d d9 90 	movdqa XMMWORD PTR [rip+0x90d90d],xmm0        # 0x180913bd0
   1800062c2:	00 
   1800062c3:	48 83 c4 28          	add    rsp,0x28
   1800062c7:	e9                   	.byte 0xe9


renderer_rmdwin10_f.dll:     file format pei-x86-64


Disassembly of section .text:

000000018002dac0 <.text+0x2cac0>:
   18002dac0:	8b 81 b0 00 00 00    	mov    eax,DWORD PTR [rcx+0xb0]
   18002dac6:	c3                   	ret


renderer_rmdwin10_f.dll:     file format pei-x86-64


Disassembly of section .text:

000000018012ba10 <.text+0x12aa10>:
   18012ba10:	48 8d 0d 09 81 7e 00 	lea    rcx,[rip+0x7e8109]        # 0x180913b20
   18012ba17:	e8 a4 20 f0 ff       	call   0x18002dac0
   18012ba1c:	89 85 08 04 00 00    	mov    DWORD PTR [rbp+0x408],eax
   18012ba22:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012ba25:	48 81 c1 f8 00 00 00 	add    rcx,0xf8
   18012ba2c:	e8 6f 79 ee ff       	call   0x1800133a0
   18012ba31:	48 8b c8             	mov    rcx,rax
   18012ba34:	e8 77 56 02 00       	call   0x1801510b0
   18012ba39:	41 8b dc             	mov    ebx,r12d
   18012ba3c:	85 c0                	test   eax,eax
   18012ba3e:	b8 02 00 00 00       	mov    eax,0x2
   18012ba43:	0f 4f d8             	cmovg  ebx,eax
   18012ba46:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012ba49:	48 81 c1 40 02 00 00 	add    rcx,0x240
   18012ba50:	e8 4b 79 ee ff       	call   0x1800133a0
   18012ba55:	48 8b c8             	mov    rcx,rax
   18012ba58:	89 5c 24 20          	mov    DWORD PTR [rsp+0x20],ebx
   18012ba5c:	44 8b 8d 08 04 00 00 	mov    r9d,DWORD PTR [rbp+0x408]
   18012ba63:	44 8b 05 22 2a 7e 00 	mov    r8d,DWORD PTR [rip+0x7e2a22]        # 0x18090e48c
   18012ba6a:	8b 15 18 2a 7e 00    	mov    edx,DWORD PTR [rip+0x7e2a18]        # 0x18090e488
   18012ba70:	e8 5b 72 f7 ff       	call   0x1800a2cd0


renderer_rmdwin10_f.dll:     file format pei-x86-64


Disassembly of section .text:

000000018012bebb <.text+0x12aebb>:
   18012bebb:	48 8d 15 2e 43 50 00 	lea    rdx,[rip+0x50432e]        # 0x1806301f0
   18012bec2:	48 8d 0d 1f b4 16 01 	lea    rcx,[rip+0x116b41f]        # 0x1812972e8
   18012bec9:	e8 82 58 f0 ff       	call   0x180031750
   18012bece:	48 8d 0d fb 25 4a 00 	lea    rcx,[rip+0x4a25fb]        # 0x1805ce4d0
   18012bed5:	e8 ea bc 45 00       	call   0x180587bc4
   18012beda:	90                   	nop
   18012bedb:	48 8d 0d fe b3 16 01 	lea    rcx,[rip+0x116b3fe]        # 0x1812972e0
   18012bee2:	e8 1d bf 45 00       	call   0x180587e04
   18012bee7:	48 8d 0d fa b3 16 01 	lea    rcx,[rip+0x116b3fa]        # 0x1812972e8
   18012beee:	e8 5d 4e ee ff       	call   0x180010d50
   18012bef3:	48 8b c8             	mov    rcx,rax
   18012bef6:	e8 95 4d ee ff       	call   0x180010c90
   18012befb:	8b c8                	mov    ecx,eax
   18012befd:	45 33 c0             	xor    r8d,r8d
   18012bf00:	48 8d 95 08 04 00 00 	lea    rdx,[rbp+0x408]
   18012bf07:	ff 15 83 40 4b 00    	call   QWORD PTR [rip+0x4b4083]        # 0x1805dff90
   18012bf0d:	8b 85 08 04 00 00    	mov    eax,DWORD PTR [rbp+0x408]
   18012bf13:	89 05 df b3 16 01    	mov    DWORD PTR [rip+0x116b3df],eax        # 0x1812972f8
   18012bf19:	8b 04 3e             	mov    eax,DWORD PTR [rsi+rdi*1]
   18012bf1c:	39                   	.byte 0x39
   18012bf1d:	05                   	.byte 0x5
   18012bf1e:	de                   	.byte 0xde
   18012bf1f:	b3                   	.byte 0xb3


renderer_rmdwin10_f.dll:     file format pei-x86-64


Disassembly of section .text:

00000001800a2cd0 <.text+0xa1cd0>:
   1800a2cd0:	48 8b c4             	mov    rax,rsp
   1800a2cd3:	55                   	push   rbp
   1800a2cd4:	41 55                	push   r13
   1800a2cd6:	41 56                	push   r14
   1800a2cd8:	48 8d a8 f8 fd ff ff 	lea    rbp,[rax-0x208]
   1800a2cdf:	48 81 ec f0 02 00 00 	sub    rsp,0x2f0
   1800a2ce6:	48 c7 45 90 fe ff ff 	mov    QWORD PTR [rbp-0x70],0xfffffffffffffffe
   1800a2ced:	ff 
   1800a2cee:	48 89 58 08          	mov    QWORD PTR [rax+0x8],rbx
   1800a2cf2:	48 89 70 10          	mov    QWORD PTR [rax+0x10],rsi
   1800a2cf6:	48 89 78 18          	mov    QWORD PTR [rax+0x18],rdi
   1800a2cfa:	4c 89 60 20          	mov    QWORD PTR [rax+0x20],r12
   1800a2cfe:	0f 29 70 d8          	movaps XMMWORD PTR [rax-0x28],xmm6
   1800a2d02:	45 8b f1             	mov    r14d,r9d
   1800a2d05:	41 8b f0             	mov    esi,r8d
   1800a2d08:	8b fa                	mov    edi,edx
   1800a2d0a:	48 8b d9             	mov    rbx,rcx
   1800a2d0d:	41 bd 01 00 00 00    	mov    r13d,0x1
   1800a2d13:	41 3b d5             	cmp    edx,r13d
   1800a2d16:	41 0f 42 fd          	cmovb  edi,r13d
   1800a2d1a:	45 3b c5             	cmp    r8d,r13d
   1800a2d1d:	41 0f 42 f5          	cmovb  esi,r13d
   1800a2d21:	45 3b cd             	cmp    r9d,r13d
   1800a2d24:	45 0f 42 f5          	cmovb  r14d,r13d
   1800a2d28:	e8                   	.byte 0xe8


renderer_rmdwin10_f.dll:     file format pei-x86-64


Disassembly of section .text:

00000001800a2d81 <.text+0xa1d81>:
   1800a2d81:	45 8b cd             	mov    r9d,r13d
   1800a2d84:	44 8b c6             	mov    r8d,esi
   1800a2d87:	8b d7                	mov    edx,edi
   1800a2d89:	48 8d 4d 00          	lea    rcx,[rbp+0x0]
   1800a2d8d:	ff 15 95 cf 53 00    	call   QWORD PTR [rip+0x53cf95]        # 0x1805dfd28
   1800a2d93:	44 89 75 10          	mov    DWORD PTR [rbp+0x10],r14d
   1800a2d97:	4c 8d 05 1a 49 58 00 	lea    r8,[rip+0x58491a]        # 0x1806276b8
   1800a2d9e:	48 8d 55 00          	lea    rdx,[rbp+0x0]
   1800a2da2:	48 8b 0d 2f fb 75 00 	mov    rcx,QWORD PTR [rip+0x75fb2f]        # 0x1808028d8
   1800a2da9:	e8 b2 69 00 00       	call   0x1800a9760


renderer_rmdwin10_f.dll:     file format pei-x86-64


Disassembly of section .text:

00000001800a2e0f <.text+0xa1e0f>:
   1800a2e0f:	45 8b cd             	mov    r9d,r13d
   1800a2e12:	44 8b c6             	mov    r8d,esi
   1800a2e15:	8b d7                	mov    edx,edi
   1800a2e17:	48 8d 4d 60          	lea    rcx,[rbp+0x60]
   1800a2e1b:	ff 15 07 cf 53 00    	call   QWORD PTR [rip+0x53cf07]        # 0x1805dfd28
   1800a2e21:	44 89 75 70          	mov    DWORD PTR [rbp+0x70],r14d
   1800a2e25:	4c 8d 05 bc 48 58 00 	lea    r8,[rip+0x5848bc]        # 0x1806276e8
   1800a2e2c:	48 8d 55 60          	lea    rdx,[rbp+0x60]
   1800a2e30:	48 8b 0d a1 fa 75 00 	mov    rcx,QWORD PTR [rip+0x75faa1]        # 0x1808028d8
   1800a2e37:	e8 24 69 00 00       	call   0x1800a9760


renderer_rmdwin10_f.dll:     file format pei-x86-64


Disassembly of section .text:

00000001800a2e9f <.text+0xa1e9f>:
   1800a2e9f:	45 8b cd             	mov    r9d,r13d
   1800a2ea2:	44 8b c6             	mov    r8d,esi
   1800a2ea5:	8b d7                	mov    edx,edi
   1800a2ea7:	48 8d 8d c0 00 00 00 	lea    rcx,[rbp+0xc0]
   1800a2eae:	ff 15 74 ce 53 00    	call   QWORD PTR [rip+0x53ce74]        # 0x1805dfd28
   1800a2eb4:	44 89 b5 d0 00 00 00 	mov    DWORD PTR [rbp+0xd0],r14d
   1800a2ebb:	4c 8d 05 56 48 58 00 	lea    r8,[rip+0x584856]        # 0x180627718
   1800a2ec2:	48 8d 95 c0 00 00 00 	lea    rdx,[rbp+0xc0]
   1800a2ec9:	48 8b 0d 08 fa 75 00 	mov    rcx,QWORD PTR [rip+0x75fa08]        # 0x1808028d8
   1800a2ed0:	e8 8b 68 00 00       	call   0x1800a9760
