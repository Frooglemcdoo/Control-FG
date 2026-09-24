
/workspace/scratch/65fb49337344/upload/d3d_rmdwin10_f(4).dll:     file format pei-x86-64


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
