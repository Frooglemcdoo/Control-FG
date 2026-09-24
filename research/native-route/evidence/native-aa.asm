
upload/d3d_rmdwin10_f(4).dll:     file format pei-x86-64


Disassembly of section .text:

000000018001f990 <.text+0x1e990>:
   18001f990:	48 8b c4             	mov    rax,rsp
   18001f993:	48 89 58 18          	mov    QWORD PTR [rax+0x18],rbx
   18001f997:	48 89 50 10          	mov    QWORD PTR [rax+0x10],rdx
   18001f99b:	48 89 48 08          	mov    QWORD PTR [rax+0x8],rcx
   18001f99f:	55                   	push   rbp
   18001f9a0:	56                   	push   rsi
   18001f9a1:	57                   	push   rdi
   18001f9a2:	41 54                	push   r12
   18001f9a4:	41 55                	push   r13
   18001f9a6:	41 56                	push   r14
   18001f9a8:	41 57                	push   r15
   18001f9aa:	48 8d a8 f8 fc ff ff 	lea    rbp,[rax-0x308]
   18001f9b1:	48 81 ec d0 03 00 00 	sub    rsp,0x3d0
   18001f9b8:	0f 29 70 b8          	movaps XMMWORD PTR [rax-0x48],xmm6
   18001f9bc:	4d 8b e9             	mov    r13,r9
   18001f9bf:	0f 29 78 a8          	movaps XMMWORD PTR [rax-0x58],xmm7
   18001f9c3:	4d 8b e0             	mov    r12,r8
   18001f9c6:	44 0f 29 40 98       	movaps XMMWORD PTR [rax-0x68],xmm8
   18001f9cb:	48 8b da             	mov    rbx,rdx
   18001f9ce:	44 0f 29 48 88       	movaps XMMWORD PTR [rax-0x78],xmm9
   18001f9d3:	44 0f 29 90 78 ff ff 	movaps XMMWORD PTR [rax-0x88],xmm10
   18001f9da:	ff 
   18001f9db:	44 0f 29 98 68 ff ff 	movaps XMMWORD PTR [rax-0x98],xmm11
   18001f9e2:	ff 
   18001f9e3:	e8 a8 b3 01 00       	call   0x18003ad90
   18001f9e8:	48 8b cb             	mov    rcx,rbx
   18001f9eb:	e8 30 ab 01 00       	call   0x18003a520
   18001f9f0:	49 8b cc             	mov    rcx,r12
   18001f9f3:	e8 28 ab 01 00       	call   0x18003a520
   18001f9f8:	49 8b cd             	mov    rcx,r13
   18001f9fb:	e8 20 ab 01 00       	call   0x18003a520
   18001fa00:	4c 8b b5 30 03 00 00 	mov    r14,QWORD PTR [rbp+0x330]
   18001fa07:	4d 85 f6             	test   r14,r14
   18001fa0a:	74 08                	je     0x18001fa14
   18001fa0c:	49 8b ce             	mov    rcx,r14
   18001fa0f:	e8 0c ab 01 00       	call   0x18003a520
   18001fa14:	4c 8b bd 38 03 00 00 	mov    r15,QWORD PTR [rbp+0x338]
   18001fa1b:	4d 85 ff             	test   r15,r15
   18001fa1e:	74 08                	je     0x18001fa28
   18001fa20:	49 8b cf             	mov    rcx,r15
   18001fa23:	e8 f8 aa 01 00       	call   0x18003a520
   18001fa28:	48 8b 3d b1 21 0f 00 	mov    rdi,QWORD PTR [rip+0xf21b1]        # 0x180111be0
   18001fa2f:	80 7f 1e 00          	cmp    BYTE PTR [rdi+0x1e],0x0
   18001fa33:	74 2b                	je     0x18001fa60
   18001fa35:	48 8b 8d 48 03 00 00 	mov    rcx,QWORD PTR [rbp+0x348]
   18001fa3c:	e8 df aa 01 00       	call   0x18003a520
   18001fa41:	48 8b 8d 50 03 00 00 	mov    rcx,QWORD PTR [rbp+0x350]
   18001fa48:	e8 d3 aa 01 00       	call   0x18003a520
   18001fa4d:	48 8b 8d 40 03 00 00 	mov    rcx,QWORD PTR [rbp+0x340]
   18001fa54:	e8 c7 aa 01 00       	call   0x18003a520
   18001fa59:	48 8b 3d 80 21 0f 00 	mov    rdi,QWORD PTR [rip+0xf2180]        # 0x180111be0
   18001fa60:	65 48 8b 04 25 58 00 	mov    rax,QWORD PTR gs:0x58
   18001fa67:	00 00 
   18001fa69:	8b 0d 8d 1b 0f 00    	mov    ecx,DWORD PTR [rip+0xf1b8d]        # 0x1801115fc
   18001fa6f:	66 44 0f 6e 57 0c    	movd   xmm10,DWORD PTR [rdi+0xc]
   18001fa75:	66 44 0f 6e 5f 10    	movd   xmm11,DWORD PTR [rdi+0x10]
   18001fa7b:	45 0f 5b d2          	cvtdq2ps xmm10,xmm10
   18001fa7f:	48 8b 34 c8          	mov    rsi,QWORD PTR [rax+rcx*8]
   18001fa83:	b8 08 00 00 00       	mov    eax,0x8
   18001fa88:	45 0f 5b db          	cvtdq2ps xmm11,xmm11
   18001fa8c:	48 8b 04 30          	mov    rax,QWORD PTR [rax+rsi*1]
   18001fa90:	48 85 c0             	test   rax,rax
   18001fa93:	74 0b                	je     0x18001faa0
   18001fa95:	bb 10 00 00 00       	mov    ebx,0x10
   18001fa9a:	48 8b 1c 33          	mov    rbx,QWORD PTR [rbx+rsi*1]
   18001fa9e:	eb 07                	jmp    0x18001faa7
   18001faa0:	48 8b 1d 79 21 0f 00 	mov    rbx,QWORD PTR [rip+0xf2179]        # 0x180111c20
   18001faa7:	48 85 db             	test   rbx,rbx
   18001faaa:	74 2c                	je     0x18001fad8
   18001faac:	ff 15 fe d6 03 00    	call   QWORD PTR [rip+0x3d6fe]        # 0x18005d1b0
   18001fab2:	8b f8                	mov    edi,eax
   18001fab4:	39 43 08             	cmp    DWORD PTR [rbx+0x8],eax
   18001fab7:	74 0c                	je     0x18001fac5
   18001fab9:	48 8b cb             	mov    rcx,rbx
   18001fabc:	ff 15 f6 d6 03 00    	call   QWORD PTR [rip+0x3d6f6]        # 0x18005d1b8
   18001fac2:	89 7b 08             	mov    DWORD PTR [rbx+0x8],edi
   18001fac5:	ff 43 0c             	inc    DWORD PTR [rbx+0xc]
   18001fac8:	48 8b 3d 11 21 0f 00 	mov    rdi,QWORD PTR [rip+0xf2111]        # 0x180111be0
   18001facf:	b8 08 00 00 00       	mov    eax,0x8
   18001fad4:	48 8b 04 30          	mov    rax,QWORD PTR [rax+rsi*1]
   18001fad8:	48 8b 35 39 21 0f 00 	mov    rsi,QWORD PTR [rip+0xf2139]        # 0x180111c18
   18001fadf:	48 8d 4c 24 20       	lea    rcx,[rsp+0x20]
   18001fae4:	48 85 c0             	test   rax,rax
   18001fae7:	41 b8 68 01 00 00    	mov    r8d,0x168
   18001faed:	48 0f 45 f0          	cmovne rsi,rax
   18001faf1:	33 d2                	xor    edx,edx
   18001faf3:	e8 0c 8b 03 00       	call   0x180058604
   18001faf8:	49 8b 85 88 00 00 00 	mov    rax,QWORD PTR [r13+0x88]
   18001faff:	44 8b 6f 0c          	mov    r13d,DWORD PTR [rdi+0xc]
   18001fb03:	8b 4f 10             	mov    ecx,DWORD PTR [rdi+0x10]
   18001fb06:	48 89 85 30 03 00 00 	mov    QWORD PTR [rbp+0x330],rax
   18001fb0d:	48 89 44 24 38       	mov    QWORD PTR [rsp+0x38],rax
   18001fb12:	49 8b 84 24 88 00 00 	mov    rax,QWORD PTR [r12+0x88]
   18001fb19:	00 
   18001fb1a:	48 89 85 38 03 00 00 	mov    QWORD PTR [rbp+0x338],rax
   18001fb21:	48 89 44 24 40       	mov    QWORD PTR [rsp+0x40],rax
   18001fb26:	48 8b 85 18 03 00 00 	mov    rax,QWORD PTR [rbp+0x318]
   18001fb2d:	44 89 6c 24 50       	mov    DWORD PTR [rsp+0x50],r13d
   18001fb32:	89 4c 24 54          	mov    DWORD PTR [rsp+0x54],ecx
   18001fb36:	48 8b 80 88 00 00 00 	mov    rax,QWORD PTR [rax+0x88]
   18001fb3d:	48 89 85 18 03 00 00 	mov    QWORD PTR [rbp+0x318],rax
   18001fb44:	48 89 44 24 20       	mov    QWORD PTR [rsp+0x20],rax
   18001fb49:	48 8b 85 10 03 00 00 	mov    rax,QWORD PTR [rbp+0x310]
   18001fb50:	48 8b 80 88 00 00 00 	mov    rax,QWORD PTR [rax+0x88]
   18001fb57:	48 89 85 10 03 00 00 	mov    QWORD PTR [rbp+0x310],rax
   18001fb5e:	48 89 44 24 28       	mov    QWORD PTR [rsp+0x28],rax
   18001fb63:	4d 85 f6             	test   r14,r14
   18001fb66:	74 09                	je     0x18001fb71
   18001fb68:	4d 8b a6 88 00 00 00 	mov    r12,QWORD PTR [r14+0x88]
   18001fb6f:	eb 03                	jmp    0x18001fb74
   18001fb71:	45 33 e4             	xor    r12d,r12d
   18001fb74:	f3 0f 10 8d 80 03 00 	movss  xmm1,DWORD PTR [rbp+0x380]
   18001fb7b:	00 
   18001fb7c:	0f 57 c0             	xorps  xmm0,xmm0
   18001fb7f:	0f 2f c8             	comiss xmm1,xmm0
   18001fb82:	0f b6 85 58 03 00 00 	movzx  eax,BYTE PTR [rbp+0x358]
   18001fb89:	4c 89 64 24 70       	mov    QWORD PTR [rsp+0x70],r12
   18001fb8e:	89 44 24 58          	mov    DWORD PTR [rsp+0x58],eax
   18001fb92:	f3 0f 10 bd 78 03 00 	movss  xmm7,DWORD PTR [rbp+0x378]
   18001fb99:	00 
   18001fb9a:	f3 0f 11 7d b0       	movss  DWORD PTR [rbp-0x50],xmm7
   18001fb9f:	72 08                	jb     0x18001fba9
   18001fba1:	f3 0f 11 4c 24 30    	movss  DWORD PTR [rsp+0x30],xmm1
   18001fba7:	eb 0b                	jmp    0x18001fbb4
   18001fba9:	f3 0f 10 47 20       	movss  xmm0,DWORD PTR [rdi+0x20]
   18001fbae:	f3 0f 11 44 24 30    	movss  DWORD PTR [rsp+0x30],xmm0
   18001fbb4:	f3 0f 10 b5 70 03 00 	movss  xmm6,DWORD PTR [rbp+0x370]
   18001fbbb:	00 
   18001fbbc:	66 41 0f 6e c5       	movd   xmm0,r13d
   18001fbc1:	66 0f 6e c9          	movd   xmm1,ecx
   18001fbc5:	f3 44 0f 11 54 24 5c 	movss  DWORD PTR [rsp+0x5c],xmm10
   18001fbcc:	f3 44 0f 11 5c 24 60 	movss  DWORD PTR [rsp+0x60],xmm11
   18001fbd3:	c7 45 40 01 00 00 00 	mov    DWORD PTR [rbp+0x40],0x1
   18001fbda:	f3 0f 5e 35 f6 72 04 	divss  xmm6,DWORD PTR [rip+0x472f6]        # 0x180066ed8
   18001fbe1:	00 
   18001fbe2:	f3 0f e6 c0          	cvtdq2pd xmm0,xmm0
   18001fbe6:	f3 0f 11 75 70       	movss  DWORD PTR [rbp+0x70],xmm6
   18001fbeb:	f2 0f 59 85 60 03 00 	mulsd  xmm0,QWORD PTR [rbp+0x360]
   18001fbf2:	00 
   18001fbf3:	f3 0f e6 c9          	cvtdq2pd xmm1,xmm1
   18001fbf7:	f2 0f 59 05 c1 72 04 	mulsd  xmm0,QWORD PTR [rip+0x472c1]        # 0x180066ec0
   18001fbfe:	00 
   18001fbff:	f2 0f 59 8d 68 03 00 	mulsd  xmm1,QWORD PTR [rbp+0x368]
   18001fc06:	00 
   18001fc07:	66 44 0f 5a c0       	cvtpd2ps xmm8,xmm0
   18001fc0c:	f2 0f 59 0d ec 72 04 	mulsd  xmm1,QWORD PTR [rip+0x472ec]        # 0x180066f00
   18001fc13:	00 
   18001fc14:	f3 44 0f 11 44 24 48 	movss  DWORD PTR [rsp+0x48],xmm8
   18001fc1b:	66 44 0f 5a c9       	cvtpd2ps xmm9,xmm1
   18001fc20:	f3 44 0f 11 4c 24 4c 	movss  DWORD PTR [rsp+0x4c],xmm9
   18001fc27:	4d 85 ff             	test   r15,r15
   18001fc2a:	74 09                	je     0x18001fc35
   18001fc2c:	4d 8b b7 88 00 00 00 	mov    r14,QWORD PTR [r15+0x88]
   18001fc33:	eb 03                	jmp    0x18001fc38
   18001fc35:	45 33 f6             	xor    r14d,r14d
   18001fc38:	80 7f 1e 00          	cmp    BYTE PTR [rdi+0x1e],0x0
   18001fc3c:	4c 89 75 78          	mov    QWORD PTR [rbp+0x78],r14
   18001fc40:	0f 84 60 01 00 00    	je     0x18001fda6
   18001fc46:	33 c0                	xor    eax,eax
   18001fc48:	48 8d 4d 90          	lea    rcx,[rbp-0x70]
   18001fc4c:	33 d2                	xor    edx,edx
   18001fc4e:	48 89 44 24 7c       	mov    QWORD PTR [rsp+0x7c],rax
   18001fc53:	41 b8 e0 00 00 00    	mov    r8d,0xe0
   18001fc59:	89 45 84             	mov    DWORD PTR [rbp-0x7c],eax
   18001fc5c:	e8 a3 89 03 00       	call   0x180058604
   18001fc61:	33 d2                	xor    edx,edx
   18001fc63:	48 8d 4d 78          	lea    rcx,[rbp+0x78]
   18001fc67:	41 b8 e0 00 00 00    	mov    r8d,0xe0
   18001fc6d:	e8 92 89 03 00       	call   0x180058604
   18001fc72:	33 d2                	xor    edx,edx
   18001fc74:	48 8d 8d 5c 01 00 00 	lea    rcx,[rbp+0x15c]
   18001fc7b:	41 b8 8c 00 00 00    	mov    r8d,0x8c
   18001fc81:	e8 7e 89 03 00       	call   0x180058604
   18001fc86:	33 c0                	xor    eax,eax
   18001fc88:	48 8d 8d 1c 02 00 00 	lea    rcx,[rbp+0x21c]
   18001fc8f:	33 d2                	xor    edx,edx
   18001fc91:	48 89 85 ec 01 00 00 	mov    QWORD PTR [rbp+0x1ec],rax
   18001fc98:	48 89 85 f4 01 00 00 	mov    QWORD PTR [rbp+0x1f4],rax
   18001fc9f:	48 89 85 fc 01 00 00 	mov    QWORD PTR [rbp+0x1fc],rax
   18001fca6:	44 8d 40 54          	lea    r8d,[rax+0x54]
   18001fcaa:	48 89 85 04 02 00 00 	mov    QWORD PTR [rbp+0x204],rax
   18001fcb1:	48 89 85 0c 02 00 00 	mov    QWORD PTR [rbp+0x20c],rax
   18001fcb8:	89 85 14 02 00 00    	mov    DWORD PTR [rbp+0x214],eax
   18001fcbe:	e8 41 89 03 00       	call   0x180058604
   18001fcc3:	48 8b 85 30 03 00 00 	mov    rax,QWORD PTR [rbp+0x330]
   18001fcca:	4c 8d 4c 24 20       	lea    r9,[rsp+0x20]
   18001fccf:	48 8b 4e 18          	mov    rcx,QWORD PTR [rsi+0x18]
   18001fcd3:	48 89 44 24 50       	mov    QWORD PTR [rsp+0x50],rax
   18001fcd8:	48 8b 85 38 03 00 00 	mov    rax,QWORD PTR [rbp+0x338]
   18001fcdf:	48 89 44 24 58       	mov    QWORD PTR [rsp+0x58],rax
   18001fce4:	8b 47 10             	mov    eax,DWORD PTR [rdi+0x10]
   18001fce7:	89 44 24 6c          	mov    DWORD PTR [rsp+0x6c],eax
   18001fceb:	48 8b 85 18 03 00 00 	mov    rax,QWORD PTR [rbp+0x318]
   18001fcf2:	48 89 44 24 40       	mov    QWORD PTR [rsp+0x40],rax
   18001fcf7:	48 8b 85 10 03 00 00 	mov    rax,QWORD PTR [rbp+0x310]
   18001fcfe:	48 89 44 24 48       	mov    QWORD PTR [rsp+0x48],rax
   18001fd03:	0f b6 85 58 03 00 00 	movzx  eax,BYTE PTR [rbp+0x358]
   18001fd0a:	89 44 24 70          	mov    DWORD PTR [rsp+0x70],eax
   18001fd0e:	48 8b 85 48 03 00 00 	mov    rax,QWORD PTR [rbp+0x348]
   18001fd15:	f3 0f 11 bd 58 01 00 	movss  DWORD PTR [rbp+0x158],xmm7
   18001fd1c:	00 
   18001fd1d:	f3 44 0f 11 54 24 74 	movss  DWORD PTR [rsp+0x74],xmm10
   18001fd24:	f3 44 0f 11 5c 24 78 	movss  DWORD PTR [rsp+0x78],xmm11
   18001fd2b:	48 8b 80 88 00 00 00 	mov    rax,QWORD PTR [rax+0x88]
   18001fd32:	48 89 44 24 20       	mov    QWORD PTR [rsp+0x20],rax
   18001fd37:	48 8b 85 50 03 00 00 	mov    rax,QWORD PTR [rbp+0x350]
   18001fd3e:	f3 44 0f 11 44 24 60 	movss  DWORD PTR [rsp+0x60],xmm8
   18001fd45:	f3 44 0f 11 4c 24 64 	movss  DWORD PTR [rsp+0x64],xmm9
   18001fd4c:	f3 0f 11 b5 18 02 00 	movss  DWORD PTR [rbp+0x218],xmm6
   18001fd53:	00 
   18001fd54:	48 8b 80 88 00 00 00 	mov    rax,QWORD PTR [rax+0x88]
   18001fd5b:	48 89 44 24 28       	mov    QWORD PTR [rsp+0x28],rax
   18001fd60:	48 8b 85 40 03 00 00 	mov    rax,QWORD PTR [rbp+0x340]
   18001fd67:	44 89 6c 24 68       	mov    DWORD PTR [rsp+0x68],r13d
   18001fd6c:	4c 89 65 88          	mov    QWORD PTR [rbp-0x78],r12
   18001fd70:	c7 85 e8 01 00 00 01 	mov    DWORD PTR [rbp+0x1e8],0x1
   18001fd77:	00 00 00 
   18001fd7a:	48 8b 80 88 00 00 00 	mov    rax,QWORD PTR [rax+0x88]
   18001fd81:	48 ff 46 08          	inc    QWORD PTR [rsi+0x8]
   18001fd85:	4c 8b 47 30          	mov    r8,QWORD PTR [rdi+0x30]
   18001fd89:	48 8b 57 28          	mov    rdx,QWORD PTR [rdi+0x28]
   18001fd8d:	4c 89 75 70          	mov    QWORD PTR [rbp+0x70],r14
   18001fd91:	48 89 44 24 30       	mov    QWORD PTR [rsp+0x30],rax
   18001fd96:	48 c7 44 24 38 00 00 	mov    QWORD PTR [rsp+0x38],0x0
   18001fd9d:	00 00 
   18001fd9f:	e8 bc d6 ff ff       	call   0x18001d460
   18001fda4:	eb 1a                	jmp    0x18001fdc0
   18001fda6:	48 ff 46 08          	inc    QWORD PTR [rsi+0x8]
   18001fdaa:	4c 8d 4c 24 20       	lea    r9,[rsp+0x20]
   18001fdaf:	4c 8b 47 30          	mov    r8,QWORD PTR [rdi+0x30]
   18001fdb3:	48 8b 57 28          	mov    rdx,QWORD PTR [rdi+0x28]
   18001fdb7:	48 8b 4e 18          	mov    rcx,QWORD PTR [rsi+0x18]
   18001fdbb:	e8 30 d0 ff ff       	call   0x18001cdf0
   18001fdc0:	8b f8                	mov    edi,eax
   18001fdc2:	48 85 db             	test   rbx,rbx
   18001fdc5:	74 16                	je     0x18001fddd
   18001fdc7:	83 43 0c ff          	add    DWORD PTR [rbx+0xc],0xffffffff
   18001fdcb:	75 10                	jne    0x18001fddd
   18001fdcd:	48 8b cb             	mov    rcx,rbx
   18001fdd0:	c7 43 08 ff ff ff ff 	mov    DWORD PTR [rbx+0x8],0xffffffff
   18001fdd7:	ff 15 cb d3 03 00    	call   QWORD PTR [rip+0x3d3cb]        # 0x18005d1a8
   18001fddd:	e8 fe 21 01 00       	call   0x180031fe0
   18001fde2:	4c 8d 9c 24 d0 03 00 	lea    r11,[rsp+0x3d0]
   18001fde9:	00 
   18001fdea:	81 e7 00 00 f0 ff    	and    edi,0xfff00000
   18001fdf0:	49 8b 5b 50          	mov    rbx,QWORD PTR [r11+0x50]
   18001fdf4:	81 ff 00 00 d0 ba    	cmp    edi,0xbad00000
   18001fdfa:	41 0f 28 73 f0       	movaps xmm6,XMMWORD PTR [r11-0x10]
   18001fdff:	41 0f 28 7b e0       	movaps xmm7,XMMWORD PTR [r11-0x20]
   18001fe04:	0f 95 c0             	setne  al
   18001fe07:	45 0f 28 43 d0       	movaps xmm8,XMMWORD PTR [r11-0x30]
   18001fe0c:	45 0f 28 4b c0       	movaps xmm9,XMMWORD PTR [r11-0x40]
   18001fe11:	45 0f 28 53 b0       	movaps xmm10,XMMWORD PTR [r11-0x50]
   18001fe16:	45 0f 28 5b a0       	movaps xmm11,XMMWORD PTR [r11-0x60]
   18001fe1b:	49 8b e3             	mov    rsp,r11
   18001fe1e:	41 5f                	pop    r15
   18001fe20:	41 5e                	pop    r14
   18001fe22:	41 5d                	pop    r13
   18001fe24:	41 5c                	pop    r12
   18001fe26:	5f                   	pop    rdi
   18001fe27:	5e                   	pop    rsi
   18001fe28:	5d                   	pop    rbp
   18001fe29:	c3                   	ret
   18001fe2a:	cc                   	int3
   18001fe2b:	cc                   	int3
   18001fe2c:	cc                   	int3
   18001fe2d:	cc                   	int3
   18001fe2e:	cc                   	int3
   18001fe2f:	cc                   	int3
   18001fe30:	48 89 5c 24 08       	mov    QWORD PTR [rsp+0x8],rbx
   18001fe35:	48 89 74 24 10       	mov    QWORD PTR [rsp+0x10],rsi
   18001fe3a:	48 89 7c 24 18       	mov    QWORD PTR [rsp+0x18],rdi
   18001fe3f:	4c 89 74 24 20       	mov    QWORD PTR [rsp+0x20],r14
   18001fe44:	48 8b 79 40          	mov    rdi,QWORD PTR [rcx+0x40]
   18001fe48:	4c 8b d9             	mov    r11,rcx
   18001fe4b:	45 0f b6 08          	movzx  r9d,BYTE PTR [r8]
   18001fe4f:	48 b9 b3 01 00 00 00 	movabs rcx,0x100000001b3
   18001fe56:	01 00 00 
   18001fe59:	48 b8 25 23 22 84 e4 	movabs rax,0xcbf29ce484222325
   18001fe60:	9c f2 cb 
   18001fe63:	48 ff cf             	dec    rdi
   18001fe66:	4c 33 c8             	xor    r9,rax
   18001fe69:	45 33 d2             	xor    r10d,r10d
   18001fe6c:	41 0f b6 40 01       	movzx  eax,BYTE PTR [r8+0x1]
   18001fe71:	49 8b d8             	mov    rbx,r8
   18001fe74:	4d 8b 73 60          	mov    r14,QWORD PTR [r11+0x60]
   18001fe78:	49 8b 73 48          	mov    rsi,QWORD PTR [r11+0x48]
   18001fe7c:	4c 0f af c9          	imul   r9,rcx
   18001fe80:	4c 33 c8             	xor    r9,rax
   18001fe83:	41 0f b6 40 02       	movzx  eax,BYTE PTR [r8+0x2]
   18001fe88:	4c 0f af c9          	imul   r9,rcx
   18001fe8c:	4c 33 c8             	xor    r9,rax
   18001fe8f:	41 0f b6 40 03       	movzx  eax,BYTE PTR [r8+0x3]
   18001fe94:	4c 0f af c9          	imul   r9,rcx
   18001fe98:	4c 33 c8             	xor    r9,rax
   18001fe9b:	41 0f b6 40 04       	movzx  eax,BYTE PTR [r8+0x4]
   18001fea0:	4c 0f af c9          	imul   r9,rcx
   18001fea4:	4c 33 c8             	xor    r9,rax
   18001fea7:	41 0f b6 40 05       	movzx  eax,BYTE PTR [r8+0x5]
   18001feac:	4c 0f af c9          	imul   r9,rcx
   18001feb0:	4c 33 c8             	xor    r9,rax
   18001feb3:	41 0f b6 40 06       	movzx  eax,BYTE PTR [r8+0x6]
   18001feb8:	4c 0f af c9          	imul   r9,rcx
   18001febc:	4c 33 c8             	xor    r9,rax
   18001febf:	41 0f b6 40 07       	movzx  eax,BYTE PTR [r8+0x7]
   18001fec4:	4c 0f af c9          	imul   r9,rcx
   18001fec8:	4c 33 c8             	xor    r9,rax
   18001fecb:	4c 0f af c9          	imul   r9,rcx
   18001fecf:	49 8d 4a ff          	lea    rcx,[r10-0x1]
   18001fed3:	4c 23 cf             	and    r9,rdi
   18001fed6:	4b 8d 04 49          	lea    rax,[r9+r9*2]
   18001feda:	49 8b 04 c6          	mov    rax,QWORD PTR [r14+rax*8]
   18001fede:	48 3b f0             	cmp    rsi,rax
   18001fee1:	74 35                	je     0x18001ff18
   18001fee3:	4d 8b 43 30          	mov    r8,QWORD PTR [r11+0x30]
   18001fee7:	4d 85 c0             	test   r8,r8
   18001feea:	74 11                	je     0x18001fefd
   18001feec:	49 39 43 28          	cmp    QWORD PTR [r11+0x28],rax
   18001fef0:	75 0b                	jne    0x18001fefd
   18001fef2:	48 83 f9 ff          	cmp    rcx,0xffffffffffffffff
   18001fef6:	75 0a                	jne    0x18001ff02
   18001fef8:	49 8b c9             	mov    rcx,r9
   18001fefb:	eb 05                	jmp    0x18001ff02
   18001fefd:	48 39 03             	cmp    QWORD PTR [rbx],rax
   18001ff00:	74 29                	je     0x18001ff2b
   18001ff02:	49 ff c2             	inc    r10
   18001ff05:	4d 03 ca             	add    r9,r10
   18001ff08:	4c 23 cf             	and    r9,rdi
   18001ff0b:	4b 8d 04 49          	lea    rax,[r9+r9*2]
   18001ff0f:	49 8b 04 c6          	mov    rax,QWORD PTR [r14+rax*8]
   18001ff13:	48 3b f0             	cmp    rsi,rax
   18001ff16:	75 cf                	jne    0x18001fee7
   18001ff18:	48 c7 02 ff ff ff ff 	mov    QWORD PTR [rdx],0xffffffffffffffff
   18001ff1f:	48 83 f9 ff          	cmp    rcx,0xffffffffffffffff
   18001ff23:	75 13                	jne    0x18001ff38
   18001ff25:	4c 89 4a 08          	mov    QWORD PTR [rdx+0x8],r9
   18001ff29:	eb 11                	jmp    0x18001ff3c
   18001ff2b:	4c 89 0a             	mov    QWORD PTR [rdx],r9
   18001ff2e:	48 c7 42 08 ff ff ff 	mov    QWORD PTR [rdx+0x8],0xffffffffffffffff
   18001ff35:	ff 
   18001ff36:	eb 04                	jmp    0x18001ff3c
   18001ff38:	48 89 4a 08          	mov    QWORD PTR [rdx+0x8],rcx
   18001ff3c:	48 8b 5c 24 08       	mov    rbx,QWORD PTR [rsp+0x8]
   18001ff41:	48 8b c2             	mov    rax,rdx
   18001ff44:	48 8b 74 24 10       	mov    rsi,QWORD PTR [rsp+0x10]
   18001ff49:	48 8b 7c 24 18       	mov    rdi,QWORD PTR [rsp+0x18]
   18001ff4e:	4c 8b 74 24 20       	mov    r14,QWORD PTR [rsp+0x20]
   18001ff53:	c3                   	ret
   18001ff54:	cc                   	int3
   18001ff55:	cc                   	int3
   18001ff56:	cc                   	int3
   18001ff57:	cc                   	int3
   18001ff58:	cc                   	int3
   18001ff59:	cc                   	int3
   18001ff5a:	cc                   	int3
   18001ff5b:	cc                   	int3
   18001ff5c:	cc                   	int3
   18001ff5d:	cc                   	int3
   18001ff5e:	cc                   	int3
   18001ff5f:	cc                   	int3
   18001ff60:	48 89 5c 24 08       	mov    QWORD PTR [rsp+0x8],rbx
   18001ff65:	57                   	push   rdi
   18001ff66:	48 83 ec 20          	sub    rsp,0x20
   18001ff6a:	48 8b d9             	mov    rbx,rcx
   18001ff6d:	48 8b fa             	mov    rdi,rdx
   18001ff70:	48 8b 49 60          	mov    rcx,QWORD PTR [rcx+0x60]
   18001ff74:	48 85 c9             	test   rcx,rcx
   18001ff77:	75 06                	jne    0x18001ff7f
   18001ff79:	48 8d 0c 52          	lea    rcx,[rdx+rdx*2]
   18001ff7d:	eb 10                	jmp    0x18001ff8f
   18001ff7f:	48 3b 7b 40          	cmp    rdi,QWORD PTR [rbx+0x40]
   18001ff83:	74 1d                	je     0x18001ffa2
   18001ff85:	ff 15 f5 d8 03 00    	call   QWORD PTR [rip+0x3d8f5]        # 0x18005d880
   18001ff8b:	48 8d 0c 7f          	lea    rcx,[rdi+rdi*2]
   18001ff8f:	ba 08 00 00 00       	mov    edx,0x8
   18001ff94:	48 c1 e1 03          	shl    rcx,0x3
   18001ff98:	ff 15 ea d8 03 00    	call   QWORD PTR [rip+0x3d8ea]        # 0x18005d888
   18001ff9e:	48 89 43 60          	mov    QWORD PTR [rbx+0x60],rax
   18001ffa2:	48 8b 4b 60          	mov    rcx,QWORD PTR [rbx+0x60]
   18001ffa6:	48 8d 04 7f          	lea    rax,[rdi+rdi*2]
   18001ffaa:	48 8d 14 c1          	lea    rdx,[rcx+rax*8]
   18001ffae:	48 3b ca             	cmp    rcx,rdx
   18001ffb1:	74 27                	je     0x18001ffda
   18001ffb3:	0f 1f 40 00          	nop    DWORD PTR [rax+0x0]
   18001ffb7:	66 0f 1f 84 00 00 00 	nop    WORD PTR [rax+rax*1+0x0]
   18001ffbe:	00 00 
   18001ffc0:	0f 10 43 48          	movups xmm0,XMMWORD PTR [rbx+0x48]
   18001ffc4:	0f 11 01             	movups XMMWORD PTR [rcx],xmm0
   18001ffc7:	f2 0f 10 4b 58       	movsd  xmm1,QWORD PTR [rbx+0x58]
   18001ffcc:	f2 0f 11 49 10       	movsd  QWORD PTR [rcx+0x10],xmm1
   18001ffd1:	48 83 c1 18          	add    rcx,0x18
   18001ffd5:	48 3b ca             	cmp    rcx,rdx
   18001ffd8:	75 e6                	jne    0x18001ffc0
   18001ffda:	33 c0                	xor    eax,eax
   18001ffdc:	48 89 7b 40          	mov    QWORD PTR [rbx+0x40],rdi
   18001ffe0:	48 89 43 38          	mov    QWORD PTR [rbx+0x38],rax
   18001ffe4:	0f 57 c0             	xorps  xmm0,xmm0
   18001ffe7:	48 89 43 30          	mov    QWORD PTR [rbx+0x30],rax
   18001ffeb:	f3 48 0f 2a c7       	cvtsi2ss xmm0,rdi
   18001fff0:	48 85 ff             	test   rdi,rdi
   18001fff3:	79 08                	jns    0x18001fffd
   18001fff5:	f3 0f 58 05 e7 6e 04 	addss  xmm0,DWORD PTR [rip+0x46ee7]        # 0x180066ee4
   18001fffc:	00 
   18001fffd:	f3 0f 10 15 db 6e 04 	movss  xmm2,DWORD PTR [rip+0x46edb]        # 0x180066ee0
   180020004:	00 
   180020005:	0f 28 c8             	movaps xmm1,xmm0
   180020008:	f3 0f 59 4b 10       	mulss  xmm1,DWORD PTR [rbx+0x10]
   18002000d:	33 c9                	xor    ecx,ecx
   18002000f:	48 ba 00 00 00 00 00 	movabs rdx,0x8000000000000000
   180020016:	00 00 80 
   180020019:	0f 2f ca             	comiss xmm1,xmm2
   18002001c:	72 0c                	jb     0x18002002a
   18002001e:	f3 0f 5c ca          	subss  xmm1,xmm2
   180020022:	0f 2f ca             	comiss xmm1,xmm2
   180020025:	73 03                	jae    0x18002002a
   180020027:	48 8b ca             	mov    rcx,rdx
   18002002a:	f3 0f 59 43 14       	mulss  xmm0,DWORD PTR [rbx+0x14]
   18002002f:	f3 48 0f 2c c1       	cvttss2si rax,xmm1
   180020034:	48 03 c1             	add    rax,rcx
   180020037:	33 c9                	xor    ecx,ecx
   180020039:	0f 2f c2             	comiss xmm0,xmm2
   18002003c:	48 89 03             	mov    QWORD PTR [rbx],rax
   18002003f:	72 0c                	jb     0x18002004d
   180020041:	f3 0f 5c c2          	subss  xmm0,xmm2
   180020045:	0f 2f c2             	comiss xmm0,xmm2
   180020048:	73 03                	jae    0x18002004d
   18002004a:	48 8b ca             	mov    rcx,rdx
   18002004d:	f3 48 0f 2c c0       	cvttss2si rax,xmm0
   180020052:	c6 43 18 00          	mov    BYTE PTR [rbx+0x18],0x0
   180020056:	48 03 c1             	add    rax,rcx
   180020059:	48 89 43 08          	mov    QWORD PTR [rbx+0x8],rax
   18002005d:	48 8b 5c 24 30       	mov    rbx,QWORD PTR [rsp+0x30]
   180020062:	48 83 c4 20          	add    rsp,0x20
   180020066:	5f                   	pop    rdi
   180020067:	c3                   	ret
   180020068:	cc                   	int3
   180020069:	cc                   	int3
   18002006a:	cc                   	int3
   18002006b:	cc                   	int3
   18002006c:	cc                   	int3
   18002006d:	cc                   	int3
   18002006e:	cc                   	int3
   18002006f:	cc                   	int3
   180020070:	48 83 ec 28          	sub    rsp,0x28
   180020074:	ba 08 00 00 00       	mov    edx,0x8
   180020079:	8d 4a 70             	lea    ecx,[rdx+0x70]
   18002007c:	ff 15 06 d8 03 00    	call   QWORD PTR [rip+0x3d806]        # 0x18005d888
   180020082:	48 89 00             	mov    QWORD PTR [rax],rax
   180020085:	48 89 40 08          	mov    QWORD PTR [rax+0x8],rax
   180020089:	48 89 40 10          	mov    QWORD PTR [rax+0x10],rax
   18002008d:	66 c7 40 18 01 01    	mov    WORD PTR [rax+0x18],0x101
   180020093:	48 83 c4 28          	add    rsp,0x28
   180020097:	c3                   	ret
   180020098:	cc                   	int3
   180020099:	cc                   	int3
   18002009a:	cc                   	int3
   18002009b:	cc                   	int3
   18002009c:	cc                   	int3
   18002009d:	cc                   	int3
   18002009e:	cc                   	int3
   18002009f:	cc                   	int3
   1800200a0:	48 89 5c 24 08       	mov    QWORD PTR [rsp+0x8],rbx
   1800200a5:	48 89 7c 24 10       	mov    QWORD PTR [rsp+0x10],rdi
   1800200aa:	55                   	push   rbp
   1800200ab:	48 8b ec             	mov    rbp,rsp
   1800200ae:	48 83 ec 60          	sub    rsp,0x60
   1800200b2:	4c 8b c2             	mov    r8,rdx
   1800200b5:	48 8b fa             	mov    rdi,rdx
   1800200b8:	48 8d 55 c0          	lea    rdx,[rbp-0x40]
   1800200bc:	48 8b d9             	mov    rbx,rcx
   1800200bf:	e8 6c fd ff ff       	call   0x18001fe30
   1800200c4:	48 8b 45 c0          	mov    rax,QWORD PTR [rbp-0x40]
   1800200c8:	48 83 f8 ff          	cmp    rax,0xffffffffffffffff
   1800200cc:	74 1c                	je     0x1800200ea
   1800200ce:	48 8d 14 40          	lea    rdx,[rax+rax*2]
   1800200d2:	48 8b 43 60          	mov    rax,QWORD PTR [rbx+0x60]
   1800200d6:	48 8d 04 d0          	lea    rax,[rax+rdx*8]
   1800200da:	48 8b 5c 24 70       	mov    rbx,QWORD PTR [rsp+0x70]
   1800200df:	48 8b 7c 24 78       	mov    rdi,QWORD PTR [rsp+0x78]
   1800200e4:	48 83 c4 60          	add    rsp,0x60
   1800200e8:	5d                   	pop    rbp
   1800200e9:	c3                   	ret
   1800200ea:	48 8b cb             	mov    rcx,rbx
   1800200ed:	e8 2e 02 00 00       	call   0x180020320
   1800200f2:	48 8b 0f             	mov    rcx,QWORD PTR [rdi]
   1800200f5:	0f 57 c0             	xorps  xmm0,xmm0
   1800200f8:	48 89 4d d8          	mov    QWORD PTR [rbp-0x28],rcx
   1800200fc:	0f 11 45 e0          	movups XMMWORD PTR [rbp-0x20],xmm0
   180020100:	84 c0                	test   al,al
   180020102:	74 79                	je     0x18002017d
   180020104:	4c 8d 45 d8          	lea    r8,[rbp-0x28]
   180020108:	48 8b cb             	mov    rcx,rbx
   18002010b:	48 8d 55 c0          	lea    rdx,[rbp-0x40]
   18002010f:	e8 1c fd ff ff       	call   0x18001fe30
   180020114:	48 8b 45 c0          	mov    rax,QWORD PTR [rbp-0x40]
   180020118:	48 83 f8 ff          	cmp    rax,0xffffffffffffffff
   18002011c:	74 30                	je     0x18002014e
   18002011e:	48 8b 53 60          	mov    rdx,QWORD PTR [rbx+0x60]
   180020122:	48 8d 04 40          	lea    rax,[rax+rax*2]
   180020126:	48 89 5d c0          	mov    QWORD PTR [rbp-0x40],rbx
   18002012a:	48 8d 0c c2          	lea    rcx,[rdx+rax*8]
   18002012e:	48 89 4d c8          	mov    QWORD PTR [rbp-0x38],rcx
   180020132:	0f 10 45 c0          	movups xmm0,XMMWORD PTR [rbp-0x40]
   180020136:	0f 11 45 d8          	movups XMMWORD PTR [rbp-0x28],xmm0
   18002013a:	48 8b 45 e0          	mov    rax,QWORD PTR [rbp-0x20]
   18002013e:	48 8b 5c 24 70       	mov    rbx,QWORD PTR [rsp+0x70]
   180020143:	48 8b 7c 24 78       	mov    rdi,QWORD PTR [rsp+0x78]
   180020148:	48 83 c4 60          	add    rsp,0x60
   18002014c:	5d                   	pop    rbp
   18002014d:	c3                   	ret
   18002014e:	4c 8b 4d c8          	mov    r9,QWORD PTR [rbp-0x38]
   180020152:	4c 8d 45 d8          	lea    r8,[rbp-0x28]
   180020156:	48 8d 55 c0          	lea    rdx,[rbp-0x40]
   18002015a:	48 8b cb             	mov    rcx,rbx
   18002015d:	e8 ee 00 00 00       	call   0x180020250
   180020162:	0f 10 00             	movups xmm0,XMMWORD PTR [rax]
   180020165:	0f 11 45 d8          	movups XMMWORD PTR [rbp-0x28],xmm0
   180020169:	48 8b 45 e0          	mov    rax,QWORD PTR [rbp-0x20]
   18002016d:	48 8b 5c 24 70       	mov    rbx,QWORD PTR [rsp+0x70]
   180020172:	48 8b 7c 24 78       	mov    rdi,QWORD PTR [rsp+0x78]
   180020177:	48 83 c4 60          	add    rsp,0x60
   18002017b:	5d                   	pop    rbp
   18002017c:	c3                   	ret
   18002017d:	4c 8b 5b 38          	mov    r11,QWORD PTR [rbx+0x38]
   180020181:	48 b9 aa aa aa aa aa 	movabs rcx,0xaaaaaaaaaaaaaaa
   180020188:	aa aa 0a 
   18002018b:	4c 8b 4b 30          	mov    r9,QWORD PTR [rbx+0x30]
   18002018f:	49 8b c3             	mov    rax,r11
   180020192:	49 2b c1             	sub    rax,r9
   180020195:	48 3b c1             	cmp    rax,rcx
   180020198:	73 69                	jae    0x180020203
   18002019a:	4c 8b 45 c8          	mov    r8,QWORD PTR [rbp-0x38]
   18002019e:	4d 85 c9             	test   r9,r9
   1800201a1:	74 1c                	je     0x1800201bf
   1800201a3:	4c 8b 53 60          	mov    r10,QWORD PTR [rbx+0x60]
   1800201a7:	4b 8d 04 40          	lea    rax,[r8+r8*2]
   1800201ab:	49 8b 0c c2          	mov    rcx,QWORD PTR [r10+rax*8]
   1800201af:	48 39 4b 28          	cmp    QWORD PTR [rbx+0x28],rcx
   1800201b3:	75 0a                	jne    0x1800201bf
   1800201b5:	49 8d 41 ff          	lea    rax,[r9-0x1]
   1800201b9:	48 89 43 30          	mov    QWORD PTR [rbx+0x30],rax
   1800201bd:	eb 0c                	jmp    0x1800201cb
   1800201bf:	4c 8b 53 60          	mov    r10,QWORD PTR [rbx+0x60]
   1800201c3:	49 8d 43 01          	lea    rax,[r11+0x1]
   1800201c7:	48 89 43 38          	mov    QWORD PTR [rbx+0x38],rax
   1800201cb:	0f 10 45 d8          	movups xmm0,XMMWORD PTR [rbp-0x28]
   1800201cf:	4b 8d 04 40          	lea    rax,[r8+r8*2]
   1800201d3:	48 8b 7c 24 78       	mov    rdi,QWORD PTR [rsp+0x78]
   1800201d8:	f2 0f 10 4d e8       	movsd  xmm1,QWORD PTR [rbp-0x18]
   1800201dd:	48 8d 0c c5 00 00 00 	lea    rcx,[rax*8+0x0]
   1800201e4:	00 
   1800201e5:	42 0f 11 04 11       	movups XMMWORD PTR [rcx+r10*1],xmm0
   1800201ea:	f2 42 0f 11 4c 11 10 	movsd  QWORD PTR [rcx+r10*1+0x10],xmm1
   1800201f1:	48 8b 43 60          	mov    rax,QWORD PTR [rbx+0x60]
   1800201f5:	48 8b 5c 24 70       	mov    rbx,QWORD PTR [rsp+0x70]
   1800201fa:	48 03 c1             	add    rax,rcx
   1800201fd:	48 83 c4 60          	add    rsp,0x60
   180020201:	5d                   	pop    rbp
   180020202:	c3                   	ret
   180020203:	48 8d 15 0e 1b 04 00 	lea    rdx,[rip+0x41b0e]        # 0x180061d18 ; 'insert overflow'
   18002020a:	48 8d 4d c0          	lea    rcx,[rbp-0x40]
   18002020e:	e8 1d e6 fe ff       	call   0x18000e830
   180020213:	48 8d 15 6e 7d 0c 00 	lea    rdx,[rip+0xc7d6e]        # 0x1800e7f88
   18002021a:	48 8d 4d c0          	lea    rcx,[rbp-0x40]
   18002021e:	e8 e7 83 03 00       	call   0x18005860a
   180020223:	cc                   	int3
   180020224:	cc                   	int3
   180020225:	cc                   	int3
   180020226:	cc                   	int3
   180020227:	cc                   	int3
   180020228:	cc                   	int3
   180020229:	cc                   	int3
   18002022a:	cc                   	int3
   18002022b:	cc                   	int3
   18002022c:	cc                   	int3
   18002022d:	cc                   	int3
   18002022e:	cc                   	int3
   18002022f:	cc                   	int3
   180020230:	48 8b 09             	mov    rcx,QWORD PTR [rcx]
   180020233:	48 85 c9             	test   rcx,rcx
   180020236:	74 0a                	je     0x180020242
   180020238:	ba 20 00 00 00       	mov    edx,0x20
   18002023d:	e9 52 6e 03 00       	jmp    0x180057094
   180020242:	c3                   	ret
   180020243:	cc                   	int3
   180020244:	cc                   	int3
   180020245:	cc                   	int3
   180020246:	cc                   	int3
   180020247:	cc                   	int3
   180020248:	cc                   	int3
   180020249:	cc                   	int3
   18002024a:	cc                   	int3
   18002024b:	cc                   	int3
   18002024c:	cc                   	int3
   18002024d:	cc                   	int3
   18002024e:	cc                   	int3
   18002024f:	cc                   	int3
   180020250:	48 89 5c 24 08       	mov    QWORD PTR [rsp+0x8],rbx
   180020255:	57                   	push   rdi
   180020256:	48 83 ec 40          	sub    rsp,0x40
   18002025a:	4c 8b 59 38          	mov    r11,QWORD PTR [rcx+0x38]
   18002025e:	48 8b da             	mov    rbx,rdx
   180020261:	4c 8b 51 30          	mov    r10,QWORD PTR [rcx+0x30]
   180020265:	49 8b c3             	mov    rax,r11
   180020268:	49 2b c2             	sub    rax,r10
   18002026b:	48 ba aa aa aa aa aa 	movabs rdx,0xaaaaaaaaaaaaaaa
   180020272:	aa aa 0a 
   180020275:	49 8b f8             	mov    rdi,r8
   180020278:	48 3b c2             	cmp    rax,rdx
   18002027b:	73 75                	jae    0x1800202f2
   18002027d:	4d 85 d2             	test   r10,r10
   180020280:	74 1c                	je     0x18002029e
   180020282:	4c 8b 41 60          	mov    r8,QWORD PTR [rcx+0x60]
   180020286:	4b 8d 04 49          	lea    rax,[r9+r9*2]
   18002028a:	49 8b 04 c0          	mov    rax,QWORD PTR [r8+rax*8]
   18002028e:	48 39 41 28          	cmp    QWORD PTR [rcx+0x28],rax
   180020292:	75 0a                	jne    0x18002029e
   180020294:	49 8d 42 ff          	lea    rax,[r10-0x1]
   180020298:	48 89 41 30          	mov    QWORD PTR [rcx+0x30],rax
   18002029c:	eb 0c                	jmp    0x1800202aa
   18002029e:	4c 8b 41 60          	mov    r8,QWORD PTR [rcx+0x60]
   1800202a2:	49 8d 43 01          	lea    rax,[r11+0x1]
   1800202a6:	48 89 41 38          	mov    QWORD PTR [rcx+0x38],rax
   1800202aa:	0f 10 07             	movups xmm0,XMMWORD PTR [rdi]
   1800202ad:	48 89 0b             	mov    QWORD PTR [rbx],rcx
   1800202b0:	4b 8d 04 49          	lea    rax,[r9+r9*2]
   1800202b4:	48 c1 e0 03          	shl    rax,0x3
   1800202b8:	41 0f 11 04 00       	movups XMMWORD PTR [r8+rax*1],xmm0
   1800202bd:	f2 0f 10 4f 10       	movsd  xmm1,QWORD PTR [rdi+0x10]
   1800202c2:	f2 41 0f 11 4c 00 10 	movsd  QWORD PTR [r8+rax*1+0x10],xmm1
   1800202c9:	48 8b 51 60          	mov    rdx,QWORD PTR [rcx+0x60]
   1800202cd:	48 03 c2             	add    rax,rdx
   1800202d0:	48 89 43 08          	mov    QWORD PTR [rbx+0x8],rax
   1800202d4:	48 8b 41 40          	mov    rax,QWORD PTR [rcx+0x40]
   1800202d8:	48 8d 0c 40          	lea    rcx,[rax+rax*2]
   1800202dc:	48 8d 04 ca          	lea    rax,[rdx+rcx*8]
   1800202e0:	48 89 43 10          	mov    QWORD PTR [rbx+0x10],rax
   1800202e4:	48 8b c3             	mov    rax,rbx
   1800202e7:	48 8b 5c 24 50       	mov    rbx,QWORD PTR [rsp+0x50]
   1800202ec:	48 83 c4 40          	add    rsp,0x40
   1800202f0:	5f                   	pop    rdi
   1800202f1:	c3                   	ret
   1800202f2:	48 8d 15 1f 1a 04 00 	lea    rdx,[rip+0x41a1f]        # 0x180061d18 ; 'insert overflow'
   1800202f9:	48 8d 4c 24 20       	lea    rcx,[rsp+0x20]
   1800202fe:	e8 2d e5 fe ff       	call   0x18000e830
   180020303:	48 8d 15 7e 7c 0c 00 	lea    rdx,[rip+0xc7c7e]        # 0x1800e7f88
   18002030a:	48 8d 4c 24 20       	lea    rcx,[rsp+0x20]
   18002030f:	e8 f6 82 03 00       	call   0x18005860a
   180020314:	cc                   	int3
   180020315:	cc                   	int3
   180020316:	cc                   	int3
   180020317:	cc                   	int3
   180020318:	cc                   	int3
   180020319:	cc                   	int3
   18002031a:	cc                   	int3
   18002031b:	cc                   	int3
   18002031c:	cc                   	int3
   18002031d:	cc                   	int3
   18002031e:	cc                   	int3
   18002031f:	cc                   	int3
   180020320:	48 8b c4             	mov    rax,rsp
   180020323:	57                   	push   rdi
   180020324:	48 81 ec c0 00 00 00 	sub    rsp,0xc0
   18002032b:	48 c7 44 24 20 fe ff 	mov    QWORD PTR [rsp+0x20],0xfffffffffffffffe
   180020332:	ff ff 
   180020334:	48 89 58 08          	mov    QWORD PTR [rax+0x8],rbx
   180020338:	48 89 70 10          	mov    QWORD PTR [rax+0x10],rsi
   18002033c:	0f 29 70 e8          	movaps XMMWORD PTR [rax-0x18],xmm6
   180020340:	48 8b d9             	mov    rbx,rcx
   180020343:	40 32 ff             	xor    dil,dil
   180020346:	f3 0f 10 35 96 6b 04 	movss  xmm6,DWORD PTR [rip+0x46b96]        # 0x180066ee4
   18002034d:	00 
   18002034e:	40 38 79 18          	cmp    BYTE PTR [rcx+0x18],dil
   180020352:	0f 84 ad 00 00 00    	je     0x180020405
   180020358:	40 32 f6             	xor    sil,sil
   18002035b:	48 8b 41 38          	mov    rax,QWORD PTR [rcx+0x38]
   18002035f:	48 2b 41 30          	sub    rax,QWORD PTR [rcx+0x30]
   180020363:	48 8b 49 08          	mov    rcx,QWORD PTR [rcx+0x8]
   180020367:	48 85 c9             	test   rcx,rcx
   18002036a:	0f 84 82 00 00 00    	je     0x1800203f2
   180020370:	48 3b c1             	cmp    rax,rcx
   180020373:	73 7d                	jae    0x1800203f2
   180020375:	4c 8b 43 40          	mov    r8,QWORD PTR [rbx+0x40]
   180020379:	49 83 f8 20          	cmp    r8,0x20
   18002037d:	76 73                	jbe    0x1800203f2
   18002037f:	f3 0f 10 53 14       	movss  xmm2,DWORD PTR [rbx+0x14]
   180020384:	49 d1 e8             	shr    r8,1
   180020387:	49 83 f8 20          	cmp    r8,0x20
   18002038b:	76 34                	jbe    0x1800203c1
   18002038d:	0f 57 c9             	xorps  xmm1,xmm1
   180020390:	f3 48 0f 2a c8       	cvtsi2ss xmm1,rax
   180020395:	48 85 c0             	test   rax,rax
   180020398:	79 04                	jns    0x18002039e
   18002039a:	f3 0f 58 ce          	addss  xmm1,xmm6
   18002039e:	0f 57 c0             	xorps  xmm0,xmm0
   1800203a1:	f3 49 0f 2a c0       	cvtsi2ss xmm0,r8
   1800203a6:	4d 85 c0             	test   r8,r8
   1800203a9:	79 04                	jns    0x1800203af
   1800203ab:	f3 0f 58 c6          	addss  xmm0,xmm6
   1800203af:	f3 0f 59 c2          	mulss  xmm0,xmm2
   1800203b3:	0f 2f c1             	comiss xmm0,xmm1
   1800203b6:	76 09                	jbe    0x1800203c1
   1800203b8:	49 d1 e8             	shr    r8,1
   1800203bb:	49 83 f8 20          	cmp    r8,0x20
   1800203bf:	77 dd                	ja     0x18002039e
   1800203c1:	48 8b d3             	mov    rdx,rbx
   1800203c4:	48 8d 4c 24 40       	lea    rcx,[rsp+0x40]
   1800203c9:	e8 f2 02 00 00       	call   0x1800206c0
   1800203ce:	48 8d 54 24 40       	lea    rdx,[rsp+0x40]
   1800203d3:	48 8b cb             	mov    rcx,rbx
   1800203d6:	e8 65 01 00 00       	call   0x180020540
   1800203db:	40 b6 01             	mov    sil,0x1
   1800203de:	48 8b 8c 24 a0 00 00 	mov    rcx,QWORD PTR [rsp+0xa0]
   1800203e5:	00 
   1800203e6:	48 85 c9             	test   rcx,rcx
   1800203e9:	74 07                	je     0x1800203f2
   1800203eb:	ff 15 8f d4 03 00    	call   QWORD PTR [rip+0x3d48f]        # 0x18005d880
   1800203f1:	90                   	nop
   1800203f2:	c6 43 18 00          	mov    BYTE PTR [rbx+0x18],0x0
   1800203f6:	40 0f b6 ff          	movzx  edi,dil
   1800203fa:	b8 01 00 00 00       	mov    eax,0x1
   1800203ff:	40 84 f6             	test   sil,sil
   180020402:	0f 45 f8             	cmovne edi,eax
   180020405:	48 8b 43 38          	mov    rax,QWORD PTR [rbx+0x38]
   180020409:	48 83 f8 fe          	cmp    rax,0xfffffffffffffffe
   18002040d:	0f 83 fe 00 00 00    	jae    0x180020511
   180020413:	48 8d 50 01          	lea    rdx,[rax+0x1]
   180020417:	48 83 7b 40 04       	cmp    QWORD PTR [rbx+0x40],0x4
   18002041c:	72 0e                	jb     0x18002042c
   18002041e:	48 3b 13             	cmp    rdx,QWORD PTR [rbx]
   180020421:	77 09                	ja     0x18002042c
   180020423:	40 0f b6 c7          	movzx  eax,dil
   180020427:	e9 cb 00 00 00       	jmp    0x1800204f7
   18002042c:	45 33 c0             	xor    r8d,r8d
   18002042f:	48                   	rex.W
