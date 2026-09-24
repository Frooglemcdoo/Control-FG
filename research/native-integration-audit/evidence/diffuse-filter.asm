
upload/renderer_rmdwin10_f(4).dll:     file format pei-x86-64


Disassembly of section .text:

0000000180014c90 <.text+0x13c90>:
   180014c90:	48 8b c4             	mov    rax,rsp
   180014c93:	44 89 40 18          	mov    DWORD PTR [rax+0x18],r8d
   180014c97:	48 89 50 10          	mov    QWORD PTR [rax+0x10],rdx
   180014c9b:	48 89 48 08          	mov    QWORD PTR [rax+0x8],rcx
   180014c9f:	55                   	push   rbp
   180014ca0:	53                   	push   rbx
   180014ca1:	56                   	push   rsi
   180014ca2:	57                   	push   rdi
   180014ca3:	41 54                	push   r12
   180014ca5:	41 55                	push   r13
   180014ca7:	41 56                	push   r14
   180014ca9:	41 57                	push   r15
   180014cab:	48 8d a8 48 fe ff ff 	lea    rbp,[rax-0x1b8]
   180014cb2:	48 81 ec 78 02 00 00 	sub    rsp,0x278
   180014cb9:	48 c7 45 50 fe ff ff 	mov    QWORD PTR [rbp+0x50],0xfffffffffffffffe
   180014cc0:	ff 
   180014cc1:	0f 29 70 a8          	movaps XMMWORD PTR [rax-0x58],xmm6
   180014cc5:	0f 29 78 98          	movaps XMMWORD PTR [rax-0x68],xmm7
   180014cc9:	44 0f 29 40 88       	movaps XMMWORD PTR [rax-0x78],xmm8
   180014cce:	44 0f 29 88 78 ff ff 	movaps XMMWORD PTR [rax-0x88],xmm9
   180014cd5:	ff 
   180014cd6:	44 0f 29 90 68 ff ff 	movaps XMMWORD PTR [rax-0x98],xmm10
   180014cdd:	ff 
   180014cde:	44 0f 29 98 58 ff ff 	movaps XMMWORD PTR [rax-0xa8],xmm11
   180014ce5:	ff 
   180014ce6:	44 0f 29 a0 48 ff ff 	movaps XMMWORD PTR [rax-0xb8],xmm12
   180014ced:	ff 
   180014cee:	44 0f 29 a8 38 ff ff 	movaps XMMWORD PTR [rax-0xc8],xmm13
   180014cf5:	ff 
   180014cf6:	44 0f 29 b0 28 ff ff 	movaps XMMWORD PTR [rax-0xd8],xmm14
   180014cfd:	ff 
   180014cfe:	44 0f 29 b8 18 ff ff 	movaps XMMWORD PTR [rax-0xe8],xmm15
   180014d05:	ff 
   180014d06:	48 8b f2             	mov    rsi,rdx
   180014d09:	48 8b d9             	mov    rbx,rcx
   180014d0c:	44 8b 2d 75 97 8f 00 	mov    r13d,DWORD PTR [rip+0x8f9775]        # 0x18090e488
   180014d13:	44 8b 25 72 97 8f 00 	mov    r12d,DWORD PTR [rip+0x8f9772]        # 0x18090e48c
   180014d1a:	44 8b 0d 1b db 7e 00 	mov    r9d,DWORD PTR [rip+0x7edb1b]        # 0x18080283c
   180014d21:	65 48 8b 04 25 58 00 	mov    rax,QWORD PTR gs:0x58
   180014d28:	00 00 
   180014d2a:	b9 40 00 00 00       	mov    ecx,0x40
   180014d2f:	4e 8b 3c c8          	mov    r15,QWORD PTR [rax+r9*8]
   180014d33:	41 8b 04 0f          	mov    eax,DWORD PTR [r15+rcx*1]
   180014d37:	39 05 db a5 27 01    	cmp    DWORD PTR [rip+0x127a5db],eax        # 0x18128f318
   180014d3d:	7e 6e                	jle    0x180014dad
   180014d3f:	48 8d 0d d2 a5 27 01 	lea    rcx,[rip+0x127a5d2]        # 0x18128f318
   180014d46:	e8 19 31 57 00       	call   0x180587e64
   180014d4b:	83 3d c6 a5 27 01 ff 	cmp    DWORD PTR [rip+0x127a5c6],0xffffffff        # 0x18128f318
   180014d52:	75 59                	jne    0x180014dad
   180014d54:	48 c7 05 c1 a5 27 01 	mov    QWORD PTR [rip+0x127a5c1],0x0        # 0x18128f320
   180014d5b:	00 00 00 00 
   180014d5f:	48 8d 15 ba aa 60 00 	lea    rdx,[rip+0x60aaba]        # 0x18061f820 ; 'g_DLF_vScreenToView'
   180014d66:	48 8d 0d bb a5 27 01 	lea    rcx,[rip+0x127a5bb]        # 0x18128f328
   180014d6d:	e8 fe b4 1f 00       	call   0x180210270
   180014d72:	c7 05 b0 a5 27 01 03 	mov    DWORD PTR [rip+0x127a5b0],0x3        # 0x18128f32c
   180014d79:	00 00 00 
   180014d7c:	48 8d 05 ad a5 27 01 	lea    rax,[rip+0x127a5ad]        # 0x18128f330
   180014d83:	48 89 05 96 a5 27 01 	mov    QWORD PTR [rip+0x127a596],rax        # 0x18128f320
   180014d8a:	0f 57 c0             	xorps  xmm0,xmm0
   180014d8d:	0f 11 05 9c a5 27 01 	movups XMMWORD PTR [rip+0x127a59c],xmm0        # 0x18128f330
   180014d94:	48 8d 0d 75 40 5b 00 	lea    rcx,[rip+0x5b4075]        # 0x1805c8e10
   180014d9b:	e8 24 2e 57 00       	call   0x180587bc4
   180014da0:	90                   	nop
   180014da1:	48 8d 0d 70 a5 27 01 	lea    rcx,[rip+0x127a570]        # 0x18128f318
   180014da8:	e8 57 30 57 00       	call   0x180587e04
   180014dad:	66 41 0f 6e c5       	movd   xmm0,r13d
   180014db2:	0f 5b c0             	cvtdq2ps xmm0,xmm0
   180014db5:	f3 0f 11 45 98       	movss  DWORD PTR [rbp-0x68],xmm0
   180014dba:	66 41 0f 6e c4       	movd   xmm0,r12d
   180014dbf:	0f 5b c0             	cvtdq2ps xmm0,xmm0
   180014dc2:	f3 0f 11 45 94       	movss  DWORD PTR [rbp-0x6c],xmm0
   180014dc7:	48 8b 15 0a dc 7e 00 	mov    rdx,QWORD PTR [rip+0x7edc0a]        # 0x1808029d8
   180014dce:	48 81 c2 50 01 00 00 	add    rdx,0x150
   180014dd5:	48 8d 4d f0          	lea    rcx,[rbp-0x10]
   180014dd9:	e8 72 3c 00 00       	call   0x180018a50
   180014dde:	f3 0f 10 55 10       	movss  xmm2,DWORD PTR [rbp+0x10]
   180014de3:	0f 57 c9             	xorps  xmm1,xmm1
   180014de6:	f3 0f 59 d1          	mulss  xmm2,xmm1
   180014dea:	f3 0f 11 55 90       	movss  DWORD PTR [rbp-0x70],xmm2
   180014def:	f3 0f 10 45 f0       	movss  xmm0,DWORD PTR [rbp-0x10]
   180014df4:	f3 44 0f 10 3d 8b a1 	movss  xmm15,DWORD PTR [rip+0x66a18b]        # 0x18067ef88
   180014dfb:	66 00 
   180014dfd:	f3 41 0f 59 c7       	mulss  xmm0,xmm15
   180014e02:	f3 44 0f 10 6d 00    	movss  xmm13,DWORD PTR [rbp+0x0]
   180014e08:	41 0f 28 dd          	movaps xmm3,xmm13
   180014e0c:	f3 0f 5c d8          	subss  xmm3,xmm0
   180014e10:	f3 0f 58 da          	addss  xmm3,xmm2
   180014e14:	f3 0f 58 5d 20       	addss  xmm3,DWORD PTR [rbp+0x20]
   180014e19:	f3 0f 11 5d d8       	movss  DWORD PTR [rbp-0x28],xmm3
   180014e1e:	f3 44 0f 10 75 14    	movss  xmm14,DWORD PTR [rbp+0x14]
   180014e24:	f3 44 0f 59 f1       	mulss  xmm14,xmm1
   180014e29:	f3 0f 10 45 f4       	movss  xmm0,DWORD PTR [rbp-0xc]
   180014e2e:	f3 41 0f 59 c7       	mulss  xmm0,xmm15
   180014e33:	f3 44 0f 10 55 04    	movss  xmm10,DWORD PTR [rbp+0x4]
   180014e39:	41 0f 28 d2          	movaps xmm2,xmm10
   180014e3d:	f3 0f 5c d0          	subss  xmm2,xmm0
   180014e41:	f3 41 0f 58 d6       	addss  xmm2,xmm14
   180014e46:	f3 0f 58 55 24       	addss  xmm2,DWORD PTR [rbp+0x24]
   180014e4b:	f3 0f 11 95 d8 01 00 	movss  DWORD PTR [rbp+0x1d8],xmm2
   180014e52:	00 
   180014e53:	f3 44 0f 10 5d 18    	movss  xmm11,DWORD PTR [rbp+0x18]
   180014e59:	f3 44 0f 59 d9       	mulss  xmm11,xmm1
   180014e5e:	f3 0f 10 45 f8       	movss  xmm0,DWORD PTR [rbp-0x8]
   180014e63:	f3 41 0f 59 c7       	mulss  xmm0,xmm15
   180014e68:	f3 0f 10 75 08       	movss  xmm6,DWORD PTR [rbp+0x8]
   180014e6d:	0f 28 d6             	movaps xmm2,xmm6
   180014e70:	f3 0f 5c d0          	subss  xmm2,xmm0
   180014e74:	f3 41 0f 58 d3       	addss  xmm2,xmm11
   180014e79:	f3 0f 58 55 28       	addss  xmm2,DWORD PTR [rbp+0x28]
   180014e7e:	f3 0f 11 55 d0       	movss  DWORD PTR [rbp-0x30],xmm2
   180014e83:	f3 0f 10 7d 1c       	movss  xmm7,DWORD PTR [rbp+0x1c]
   180014e88:	f3 0f 59 f9          	mulss  xmm7,xmm1
   180014e8c:	f3 44 0f 10 45 fc    	movss  xmm8,DWORD PTR [rbp-0x4]
   180014e92:	41 0f 28 c0          	movaps xmm0,xmm8
   180014e96:	f3 41 0f 59 c7       	mulss  xmm0,xmm15
   180014e9b:	f3 0f 10 65 0c       	movss  xmm4,DWORD PTR [rbp+0xc]
   180014ea0:	0f 28 cc             	movaps xmm1,xmm4
   180014ea3:	f3 0f 5c c8          	subss  xmm1,xmm0
   180014ea7:	f3 0f 58 cf          	addss  xmm1,xmm7
   180014eab:	f3 0f 58 4d 2c       	addss  xmm1,DWORD PTR [rbp+0x2c]
   180014eb0:	0f 28 c1             	movaps xmm0,xmm1
   180014eb3:	0f 28 d0             	movaps xmm2,xmm0
   180014eb6:	41 0f 28 c7          	movaps xmm0,xmm15
   180014eba:	44 0f 28 f8          	movaps xmm15,xmm0
   180014ebe:	0f 29 45 c0          	movaps XMMWORD PTR [rbp-0x40],xmm0
   180014ec2:	f3 44 0f 5e fa       	divss  xmm15,xmm2
   180014ec7:	f3 0f 10 55 d0       	movss  xmm2,DWORD PTR [rbp-0x30]
   180014ecc:	f3 41 0f 59 d7       	mulss  xmm2,xmm15
   180014ed1:	f3 0f 10 85 d8 01 00 	movss  xmm0,DWORD PTR [rbp+0x1d8]
   180014ed8:	00 
   180014ed9:	f3 41 0f 59 c7       	mulss  xmm0,xmm15
   180014ede:	f3 0f 11 85 d8 01 00 	movss  DWORD PTR [rbp+0x1d8],xmm0
   180014ee5:	00 
   180014ee6:	f3 44 0f 59 2d 99 a0 	mulss  xmm13,DWORD PTR [rip+0x66a099]        # 0x18067ef88
   180014eed:	66 00 
   180014eef:	f3 0f 10 45 f0       	movss  xmm0,DWORD PTR [rbp-0x10]
   180014ef4:	f3 41 0f 5c c5       	subss  xmm0,xmm13
   180014ef9:	f3 0f 58 45 90       	addss  xmm0,DWORD PTR [rbp-0x70]
   180014efe:	f3 0f 58 45 20       	addss  xmm0,DWORD PTR [rbp+0x20]
   180014f03:	f3 0f 11 45 90       	movss  DWORD PTR [rbp-0x70],xmm0
   180014f08:	f3 44 0f 10 2d 77 a0 	movss  xmm13,DWORD PTR [rip+0x66a077]        # 0x18067ef88
   180014f0f:	66 00 
   180014f11:	f3 45 0f 59 d5       	mulss  xmm10,xmm13
   180014f16:	f3 44 0f 10 65 f4    	movss  xmm12,DWORD PTR [rbp-0xc]
   180014f1c:	f3 45 0f 5c e2       	subss  xmm12,xmm10
   180014f21:	f3 45 0f 58 e6       	addss  xmm12,xmm14
   180014f26:	f3 44 0f 58 65 24    	addss  xmm12,DWORD PTR [rbp+0x24]
   180014f2c:	f3 41 0f 59 f5       	mulss  xmm6,xmm13
   180014f31:	f3 44 0f 10 4d f8    	movss  xmm9,DWORD PTR [rbp-0x8]
   180014f37:	f3 44 0f 5c ce       	subss  xmm9,xmm6
   180014f3c:	f3 45 0f 58 cb       	addss  xmm9,xmm11
   180014f41:	f3 44 0f 58 4d 28    	addss  xmm9,DWORD PTR [rbp+0x28]
   180014f47:	f3 41 0f 59 e5       	mulss  xmm4,xmm13
   180014f4c:	f3 44 0f 5c c4       	subss  xmm8,xmm4
   180014f51:	f3 44 0f 58 c7       	addss  xmm8,xmm7
   180014f56:	f3 44 0f 58 45 2c    	addss  xmm8,DWORD PTR [rbp+0x2c]
   180014f5c:	41 0f 28 c0          	movaps xmm0,xmm8
   180014f60:	0f 28 c8             	movaps xmm1,xmm0
   180014f63:	0f 28 5d c0          	movaps xmm3,XMMWORD PTR [rbp-0x40]
   180014f67:	0f 28 c3             	movaps xmm0,xmm3
   180014f6a:	f3 0f 5e c1          	divss  xmm0,xmm1
   180014f6e:	f3 44 0f 59 c8       	mulss  xmm9,xmm0
   180014f73:	f3 44 0f 59 e0       	mulss  xmm12,xmm0
   180014f78:	f3 0f 10 6d 90       	movss  xmm5,DWORD PTR [rbp-0x70]
   180014f7d:	f3 0f 59 e8          	mulss  xmm5,xmm0
   180014f81:	0f 28 ca             	movaps xmm1,xmm2
   180014f84:	0f 28 c1             	movaps xmm0,xmm1
   180014f87:	0f 57 c9             	xorps  xmm1,xmm1
   180014f8a:	0f 28 d3             	movaps xmm2,xmm3
   180014f8d:	f3 0f 5e d0          	divss  xmm2,xmm0
   180014f91:	f3 0f c2 c1 00       	cmpeqss xmm0,xmm1
   180014f96:	66 0f 38 14 d1       	blendvps xmm2,xmm1,xmm0
   180014f9b:	f3 0f 10 65 d8       	movss  xmm4,DWORD PTR [rbp-0x28]
   180014fa0:	f3 41 0f 59 e7       	mulss  xmm4,xmm15
   180014fa5:	f3 0f 59 e2          	mulss  xmm4,xmm2
   180014fa9:	f3 0f 59 95 d8 01 00 	mulss  xmm2,DWORD PTR [rbp+0x1d8]
   180014fb0:	00 
   180014fb1:	41 0f 28 c9          	movaps xmm1,xmm9
   180014fb5:	0f 28 c1             	movaps xmm0,xmm1
   180014fb8:	0f 57 c9             	xorps  xmm1,xmm1
   180014fbb:	f3 0f 5e d8          	divss  xmm3,xmm0
   180014fbf:	f3 0f c2 c1 00       	cmpeqss xmm0,xmm1
   180014fc4:	66 0f 38 14 d9       	blendvps xmm3,xmm1,xmm0
   180014fc9:	f3 0f 59 eb          	mulss  xmm5,xmm3
   180014fcd:	f3 41 0f 59 dc       	mulss  xmm3,xmm12
   180014fd2:	f3 0f 5c d3          	subss  xmm2,xmm3
   180014fd6:	f3 0f 5c ec          	subss  xmm5,xmm4
   180014fda:	f3 0f 5e 6d 98       	divss  xmm5,DWORD PTR [rbp-0x68]
   180014fdf:	f3 0f 5e 55 94       	divss  xmm2,DWORD PTR [rbp-0x6c]
   180014fe4:	41 0f 28 c5          	movaps xmm0,xmm13
   180014fe8:	f3 0f 5c 45 98       	subss  xmm0,DWORD PTR [rbp-0x68]
   180014fed:	f3 0f 59 c5          	mulss  xmm0,xmm5
   180014ff1:	f3 0f 10 35 bf 9e 66 	movss  xmm6,DWORD PTR [rip+0x669ebf]        # 0x18067eeb8
   180014ff8:	00 
   180014ff9:	f3 0f 59 c6          	mulss  xmm0,xmm6
   180014ffd:	f3 44 0f 5c 6d 94    	subss  xmm13,DWORD PTR [rbp-0x6c]
   180015003:	f3 44 0f 59 ea       	mulss  xmm13,xmm2
   180015008:	f3 44 0f 59 ee       	mulss  xmm13,xmm6
   18001500d:	f3 0f 11 45 80       	movss  DWORD PTR [rbp-0x80],xmm0
   180015012:	f3 0f 11 6d 84       	movss  DWORD PTR [rbp-0x7c],xmm5
   180015017:	f3 44 0f 11 6d 88    	movss  DWORD PTR [rbp-0x78],xmm13
   18001501d:	f3 0f 11 55 8c       	movss  DWORD PTR [rbp-0x74],xmm2
   180015022:	45 33 c0             	xor    r8d,r8d
   180015025:	48 8d 55 80          	lea    rdx,[rbp-0x80]
   180015029:	8b 0d f9 a2 27 01    	mov    ecx,DWORD PTR [rip+0x127a2f9]        # 0x18128f328
   18001502f:	ff 15 5b af 5c 00    	call   QWORD PTR [rip+0x5caf5b]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   180015035:	0f 10 45 80          	movups xmm0,XMMWORD PTR [rbp-0x80]
   180015039:	0f 11 05 f0 a2 27 01 	movups XMMWORD PTR [rip+0x127a2f0],xmm0        # 0x18128f330
   180015040:	48 8b cb             	mov    rcx,rbx
   180015043:	ff 15 ef ac 5c 00    	call   QWORD PTR [rip+0x5cacef]        # 0x1805dfd38 ; ?getFormat@Texture2D@d3d@@QEBA?AW4PixelFormat@2@XZ
   180015049:	44 0f b6 f0          	movzx  r14d,al
   18001504d:	48 8b 3d 84 d8 7e 00 	mov    rdi,QWORD PTR [rip+0x7ed884]        # 0x1808028d8
   180015054:	48 89 bd d8 01 00 00 	mov    QWORD PTR [rbp+0x1d8],rdi
   18001505b:	0f 57 c0             	xorps  xmm0,xmm0
   18001505e:	0f 29 45 80          	movaps XMMWORD PTR [rbp-0x80],xmm0
   180015062:	c7 44 24 70 ff ff ff 	mov    DWORD PTR [rsp+0x70],0xffffffff
   180015069:	ff 
   18001506a:	48 8d 45 80          	lea    rax,[rbp-0x80]
   18001506e:	48 89 44 24 68       	mov    QWORD PTR [rsp+0x68],rax
   180015073:	c7 44 24 60 01 00 00 	mov    DWORD PTR [rsp+0x60],0x1
   18001507a:	00 
   18001507b:	f3 0f 11 74 24 58    	movss  DWORD PTR [rsp+0x58],xmm6
   180015081:	33 c0                	xor    eax,eax
   180015083:	89 44 24 50          	mov    DWORD PTR [rsp+0x50],eax
   180015087:	89 44 24 48          	mov    DWORD PTR [rsp+0x48],eax
   18001508b:	48 89 44 24 40       	mov    QWORD PTR [rsp+0x40],rax
   180015090:	48 89 44 24 38       	mov    QWORD PTR [rsp+0x38],rax
   180015095:	89 44 24 30          	mov    DWORD PTR [rsp+0x30],eax
   180015099:	c7 44 24 28 05 00 00 	mov    DWORD PTR [rsp+0x28],0x5
   1800150a0:	00 
   1800150a1:	44 88 74 24 20       	mov    BYTE PTR [rsp+0x20],r14b
   1800150a6:	44 8d 48 01          	lea    r9d,[rax+0x1]
   1800150aa:	45 8b c4             	mov    r8d,r12d
   1800150ad:	41 8b d5             	mov    edx,r13d
   1800150b0:	48 8d 4d 70          	lea    rcx,[rbp+0x70]
   1800150b4:	ff 15 6e ac 5c 00    	call   QWORD PTR [rip+0x5cac6e]        # 0x1805dfd28 ; ??0Texture2DInit@d3d@@QEAA@HHHW4PixelFormat@1@HHPEBX_KW4MiscFlags@1@HMHV?$Vector4Template@M@m@@W4TileMode@1@@Z
   1800150ba:	4c 8d 05 77 a7 60 00 	lea    r8,[rip+0x60a777]        # 0x18061f838 ; 'rend::DeferredLightFiltering::filterDiffuseNoise - tmp'
   1800150c1:	48 8d 55 70          	lea    rdx,[rbp+0x70]
   1800150c5:	48 8b cf             	mov    rcx,rdi
   1800150c8:	e8 93 46 09 00       	call   0x1800a9760
   1800150cd:	48 8b d8             	mov    rbx,rax
   1800150d0:	48 89 45 d8          	mov    QWORD PTR [rbp-0x28],rax
   1800150d4:	48 8b 0e             	mov    rcx,QWORD PTR [rsi]
   1800150d7:	48 85 c9             	test   rcx,rcx
   1800150da:	74 54                	je     0x180015130
   1800150dc:	8b 3d a6 93 8f 00    	mov    edi,DWORD PTR [rip+0x8f93a6]        # 0x18090e488
   1800150e2:	ff 15 00 ac 5c 00    	call   QWORD PTR [rip+0x5cac00]        # 0x1805dfce8 ; ?getWidth@Texture2D@d3d@@QEBAHXZ
   1800150e8:	3b c7                	cmp    eax,edi
   1800150ea:	75 21                	jne    0x18001510d
   1800150ec:	8b 3d 9a 93 8f 00    	mov    edi,DWORD PTR [rip+0x8f939a]        # 0x18090e48c
   1800150f2:	48 8b 0e             	mov    rcx,QWORD PTR [rsi]
   1800150f5:	ff 15 e5 ab 5c 00    	call   QWORD PTR [rip+0x5cabe5]        # 0x1805dfce0 ; ?getHeight@Texture2D@d3d@@QEBAHXZ
   1800150fb:	3b c7                	cmp    eax,edi
   1800150fd:	75 0e                	jne    0x18001510d
   1800150ff:	48 8b 0e             	mov    rcx,QWORD PTR [rsi]
   180015102:	ff 15 30 ac 5c 00    	call   QWORD PTR [rip+0x5cac30]        # 0x1805dfd38 ; ?getFormat@Texture2D@d3d@@QEBA?AW4PixelFormat@2@XZ
   180015108:	41 3a c6             	cmp    al,r14b
   18001510b:	74 1c                	je     0x180015129
   18001510d:	48 8b 16             	mov    rdx,QWORD PTR [rsi]
   180015110:	48 c7 06 00 00 00 00 	mov    QWORD PTR [rsi],0x0
   180015117:	48 85 d2             	test   rdx,rdx
   18001511a:	74 0d                	je     0x180015129
   18001511c:	48 8b 0d b5 d7 7e 00 	mov    rcx,QWORD PTR [rip+0x7ed7b5]        # 0x1808028d8
   180015123:	e8 d8 4c 09 00       	call   0x1800a9e00
   180015128:	90                   	nop
   180015129:	48 8b bd d8 01 00 00 	mov    rdi,QWORD PTR [rbp+0x1d8]
   180015130:	48 83 3e 00          	cmp    QWORD PTR [rsi],0x0
   180015134:	0f 85 af 00 00 00    	jne    0x1800151e9
   18001513a:	0f 57 c0             	xorps  xmm0,xmm0
   18001513d:	0f 29 45 80          	movaps XMMWORD PTR [rbp-0x80],xmm0
   180015141:	c7 44 24 70 ff ff ff 	mov    DWORD PTR [rsp+0x70],0xffffffff
   180015148:	ff 
   180015149:	48 8d 45 80          	lea    rax,[rbp-0x80]
   18001514d:	48 89 44 24 68       	mov    QWORD PTR [rsp+0x68],rax
   180015152:	c7 44 24 60 01 00 00 	mov    DWORD PTR [rsp+0x60],0x1
   180015159:	00 
   18001515a:	f3 0f 11 74 24 58    	movss  DWORD PTR [rsp+0x58],xmm6
   180015160:	33 c0                	xor    eax,eax
   180015162:	89 44 24 50          	mov    DWORD PTR [rsp+0x50],eax
   180015166:	89 44 24 48          	mov    DWORD PTR [rsp+0x48],eax
   18001516a:	48 89 44 24 40       	mov    QWORD PTR [rsp+0x40],rax
   18001516f:	48 89 44 24 38       	mov    QWORD PTR [rsp+0x38],rax
   180015174:	89 44 24 30          	mov    DWORD PTR [rsp+0x30],eax
   180015178:	c7 44 24 28 05 00 00 	mov    DWORD PTR [rsp+0x28],0x5
   18001517f:	00 
   180015180:	44 88 74 24 20       	mov    BYTE PTR [rsp+0x20],r14b
   180015185:	44 8d 48 01          	lea    r9d,[rax+0x1]
   180015189:	44 8b 05 fc 92 8f 00 	mov    r8d,DWORD PTR [rip+0x8f92fc]        # 0x18090e48c
   180015190:	8b 15 f2 92 8f 00    	mov    edx,DWORD PTR [rip+0x8f92f2]        # 0x18090e488
   180015196:	48 8d 4d f0          	lea    rcx,[rbp-0x10]
   18001519a:	ff 15 88 ab 5c 00    	call   QWORD PTR [rip+0x5cab88]        # 0x1805dfd28 ; ??0Texture2DInit@d3d@@QEAA@HHHW4PixelFormat@1@HHPEBX_KW4MiscFlags@1@HMHV?$Vector4Template@M@m@@W4TileMode@1@@Z
   1800151a0:	48 8b d0             	mov    rdx,rax
   1800151a3:	4c 8d 05 c6 a6 60 00 	lea    r8,[rip+0x60a6c6]        # 0x18061f870 ; 'm_pColorHistory'
   1800151aa:	48 8b cf             	mov    rcx,rdi
   1800151ad:	e8 ae 45 09 00       	call   0x1800a9760
   1800151b2:	48 8b 16             	mov    rdx,QWORD PTR [rsi]
   1800151b5:	48 89 06             	mov    QWORD PTR [rsi],rax
   1800151b8:	48 85 d2             	test   rdx,rdx
   1800151bb:	74 0f                	je     0x1800151cc
   1800151bd:	48 8b 0d 14 d7 7e 00 	mov    rcx,QWORD PTR [rip+0x7ed714]        # 0x1808028d8
   1800151c4:	e8 37 4c 09 00       	call   0x1800a9e00
   1800151c9:	48 8b 06             	mov    rax,QWORD PTR [rsi]
   1800151cc:	0f 57 c0             	xorps  xmm0,xmm0
   1800151cf:	0f 29 45 80          	movaps XMMWORD PTR [rbp-0x80],xmm0
   1800151d3:	48 8b c8             	mov    rcx,rax
   1800151d6:	ff 15 54 ab 5c 00    	call   QWORD PTR [rip+0x5cab54]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   1800151dc:	48 8b c8             	mov    rcx,rax
   1800151df:	48 8d 55 80          	lea    rdx,[rbp-0x80]
   1800151e3:	ff 15 57 ab 5c 00    	call   QWORD PTR [rip+0x5cab57]        # 0x1805dfd40 ; ?clearUAV@DeviceUtil@d3d@@SAXPEAVNativeTexture@2@V?$Vector4Template@M@m@@@Z
   1800151e9:	ff 15 c1 aa 5c 00    	call   QWORD PTR [rip+0x5caac1]        # 0x1805dfcb0 ; ?retrieveForThread@DeviceState@d3d@@SAAEAV12@XZ
   1800151ef:	4c 8b f0             	mov    r14,rax
   1800151f2:	48 89 45 98          	mov    QWORD PTR [rbp-0x68],rax
   1800151f6:	48 8d 3d 5b a1 27 01 	lea    rdi,[rip+0x127a15b]        # 0x18128f358
   1800151fd:	b8 40 00 00 00       	mov    eax,0x40
   180015202:	41 8b 0c 07          	mov    ecx,DWORD PTR [r15+rax*1]
   180015206:	39 0d 34 a1 27 01    	cmp    DWORD PTR [rip+0x127a134],ecx        # 0x18128f340
   18001520c:	7e 67                	jle    0x180015275
   18001520e:	48 8d 0d 2b a1 27 01 	lea    rcx,[rip+0x127a12b]        # 0x18128f340
   180015215:	e8 4a 2c 57 00       	call   0x180587e64
   18001521a:	83 3d 1f a1 27 01 ff 	cmp    DWORD PTR [rip+0x127a11f],0xffffffff        # 0x18128f340
   180015221:	75 52                	jne    0x180015275
   180015223:	48 c7 05 1a a1 27 01 	mov    QWORD PTR [rip+0x127a11a],0x0        # 0x18128f348
   18001522a:	00 00 00 00 
   18001522e:	48 8d 15 4b a6 60 00 	lea    rdx,[rip+0x60a64b]        # 0x18061f880 ; 'g_DLF_tOutputHistory'
   180015235:	48 8d 0d 14 a1 27 01 	lea    rcx,[rip+0x127a114]        # 0x18128f350
   18001523c:	e8 2f b0 1f 00       	call   0x180210270
   180015241:	c7 05 09 a1 27 01 09 	mov    DWORD PTR [rip+0x127a109],0x9        # 0x18128f354
   180015248:	00 00 00 
   18001524b:	48 89 3d f6 a0 27 01 	mov    QWORD PTR [rip+0x127a0f6],rdi        # 0x18128f348
   180015252:	48 8b cf             	mov    rcx,rdi
   180015255:	ff 15 15 aa 5c 00    	call   QWORD PTR [rip+0x5caa15]        # 0x1805dfc70 ; ??0ShaderTexture@d3d@@QEAA@XZ
   18001525b:	90                   	nop
   18001525c:	48 8d 0d 7d 3b 5b 00 	lea    rcx,[rip+0x5b3b7d]        # 0x1805c8de0
   180015263:	e8 5c 29 57 00       	call   0x180587bc4
   180015268:	90                   	nop
   180015269:	48 8d 0d d0 a0 27 01 	lea    rcx,[rip+0x127a0d0]        # 0x18128f340
   180015270:	e8 8f 2b 57 00       	call   0x180587e04
   180015275:	48 8b 0e             	mov    rcx,QWORD PTR [rsi]
   180015278:	48 85 c9             	test   rcx,rcx
   18001527b:	74 0b                	je     0x180015288
   18001527d:	ff 15 ad aa 5c 00    	call   QWORD PTR [rip+0x5caaad]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   180015283:	48 8b f8             	mov    rdi,rax
   180015286:	eb 02                	jmp    0x18001528a
   180015288:	33 ff                	xor    edi,edi
   18001528a:	8b 0d c0 a0 27 01    	mov    ecx,DWORD PTR [rip+0x127a0c0]        # 0x18128f350
   180015290:	ff 15 f2 a9 5c 00    	call   QWORD PTR [rip+0x5ca9f2]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   180015296:	8b d0                	mov    edx,eax
   180015298:	4c 8b c7             	mov    r8,rdi
   18001529b:	48 8d 4d 80          	lea    rcx,[rbp-0x80]
   18001529f:	ff 15 db a9 5c 00    	call   QWORD PTR [rip+0x5ca9db]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   1800152a5:	45 33 c0             	xor    r8d,r8d
   1800152a8:	48 8d 55 80          	lea    rdx,[rbp-0x80]
   1800152ac:	8b 0d 9e a0 27 01    	mov    ecx,DWORD PTR [rip+0x127a09e]        # 0x18128f350
   1800152b2:	ff 15 d8 ac 5c 00    	call   QWORD PTR [rip+0x5cacd8]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   1800152b8:	8b 0d 92 a0 27 01    	mov    ecx,DWORD PTR [rip+0x127a092]        # 0x18128f350
   1800152be:	ff 15 c4 a9 5c 00    	call   QWORD PTR [rip+0x5ca9c4]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   1800152c4:	8b d0                	mov    edx,eax
   1800152c6:	4c 8b c7             	mov    r8,rdi
   1800152c9:	48 8d 0d 88 a0 27 01 	lea    rcx,[rip+0x127a088]        # 0x18128f358
   1800152d0:	ff 15 ba a9 5c 00    	call   QWORD PTR [rip+0x5ca9ba]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   1800152d6:	bf 40 00 00 00       	mov    edi,0x40
   1800152db:	41 8b 04 3f          	mov    eax,DWORD PTR [r15+rdi*1]
   1800152df:	39 05 83 a0 27 01    	cmp    DWORD PTR [rip+0x127a083],eax        # 0x18128f368
   1800152e5:	7e 64                	jle    0x18001534b
   1800152e7:	48 8d 0d 7a a0 27 01 	lea    rcx,[rip+0x127a07a]        # 0x18128f368
   1800152ee:	e8 71 2b 57 00       	call   0x180587e64
   1800152f3:	83 3d 6e a0 27 01 ff 	cmp    DWORD PTR [rip+0x127a06e],0xffffffff        # 0x18128f368
   1800152fa:	75 4f                	jne    0x18001534b
   1800152fc:	48 c7 05 69 a0 27 01 	mov    QWORD PTR [rip+0x127a069],0x0        # 0x18128f370
   180015303:	00 00 00 00 
   180015307:	48 8d 15 8a a5 60 00 	lea    rdx,[rip+0x60a58a]        # 0x18061f898 ; 'g_DLF_iFilterPassCount'
   18001530e:	48 8d 0d 63 a0 27 01 	lea    rcx,[rip+0x127a063]        # 0x18128f378
   180015315:	e8 56 af 1f 00       	call   0x180210270
   18001531a:	c7 05 58 a0 27 01 10 	mov    DWORD PTR [rip+0x127a058],0x10        # 0x18128f37c
   180015321:	00 00 00 
   180015324:	48 8d 05 55 a0 27 01 	lea    rax,[rip+0x127a055]        # 0x18128f380
   18001532b:	48 89 05 3e a0 27 01 	mov    QWORD PTR [rip+0x127a03e],rax        # 0x18128f370
   180015332:	48 8d 0d 77 3a 5b 00 	lea    rcx,[rip+0x5b3a77]        # 0x1805c8db0
   180015339:	e8 86 28 57 00       	call   0x180587bc4
   18001533e:	90                   	nop
   18001533f:	48 8d 0d 22 a0 27 01 	lea    rcx,[rip+0x127a022]        # 0x18128f368
   180015346:	e8 b9 2a 57 00       	call   0x180587e04
   18001534b:	8b 85 d0 01 00 00    	mov    eax,DWORD PTR [rbp+0x1d0]
   180015351:	89 85 d8 01 00 00    	mov    DWORD PTR [rbp+0x1d8],eax
   180015357:	45 33 c0             	xor    r8d,r8d
   18001535a:	48 8d 95 d8 01 00 00 	lea    rdx,[rbp+0x1d8]
   180015361:	8b 0d 11 a0 27 01    	mov    ecx,DWORD PTR [rip+0x127a011]        # 0x18128f378
   180015367:	ff 15 23 ac 5c 00    	call   QWORD PTR [rip+0x5cac23]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   18001536d:	8b 85 d8 01 00 00    	mov    eax,DWORD PTR [rbp+0x1d8]
   180015373:	89 05 07 a0 27 01    	mov    DWORD PTR [rip+0x127a007],eax        # 0x18128f380
   180015379:	48 8b b5 c0 01 00 00 	mov    rsi,QWORD PTR [rbp+0x1c0]
   180015380:	48 8b d6             	mov    rdx,rsi
   180015383:	48 8b 0d 66 d9 8e 00 	mov    rcx,QWORD PTR [rip+0x8ed966]        # 0x180902cf0
   18001538a:	e8 b1 17 1d 00       	call   0x1801e6b40
   18001538f:	41 8b 04 3f          	mov    eax,DWORD PTR [r15+rdi*1]
   180015393:	39 05 ef 9f 27 01    	cmp    DWORD PTR [rip+0x1279fef],eax        # 0x18128f388
   180015399:	7e 6e                	jle    0x180015409
   18001539b:	48 8d 0d e6 9f 27 01 	lea    rcx,[rip+0x1279fe6]        # 0x18128f388
   1800153a2:	e8 bd 2a 57 00       	call   0x180587e64
   1800153a7:	83 3d da 9f 27 01 ff 	cmp    DWORD PTR [rip+0x1279fda],0xffffffff        # 0x18128f388
   1800153ae:	75 59                	jne    0x180015409
   1800153b0:	48 c7 05 d5 9f 27 01 	mov    QWORD PTR [rip+0x1279fd5],0x0        # 0x18128f390
   1800153b7:	00 00 00 00 
   1800153bb:	48 8d 15 ee a4 60 00 	lea    rdx,[rip+0x60a4ee]        # 0x18061f8b0 ; 'g_DLF_rwtColorTarget'
   1800153c2:	48 8d 0d cf 9f 27 01 	lea    rcx,[rip+0x1279fcf]        # 0x18128f398
   1800153c9:	e8 a2 ae 1f 00       	call   0x180210270
   1800153ce:	c7 05 c4 9f 27 01 09 	mov    DWORD PTR [rip+0x1279fc4],0x9        # 0x18128f39c
   1800153d5:	00 00 00 
   1800153d8:	48 8d 05 c1 9f 27 01 	lea    rax,[rip+0x1279fc1]        # 0x18128f3a0
   1800153df:	48 89 05 aa 9f 27 01 	mov    QWORD PTR [rip+0x1279faa],rax        # 0x18128f390
   1800153e6:	48 8b c8             	mov    rcx,rax
   1800153e9:	ff 15 81 a8 5c 00    	call   QWORD PTR [rip+0x5ca881]        # 0x1805dfc70 ; ??0ShaderTexture@d3d@@QEAA@XZ
   1800153ef:	90                   	nop
   1800153f0:	48 8d 0d 89 39 5b 00 	lea    rcx,[rip+0x5b3989]        # 0x1805c8d80
   1800153f7:	e8 c8 27 57 00       	call   0x180587bc4
   1800153fc:	90                   	nop
   1800153fd:	48 8d 0d 84 9f 27 01 	lea    rcx,[rip+0x1279f84]        # 0x18128f388
   180015404:	e8 fb 29 57 00       	call   0x180587e04
   180015409:	48 85 db             	test   rbx,rbx
   18001540c:	74 0e                	je     0x18001541c
   18001540e:	48 8b cb             	mov    rcx,rbx
   180015411:	ff 15 19 a9 5c 00    	call   QWORD PTR [rip+0x5ca919]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   180015417:	48 8b f8             	mov    rdi,rax
   18001541a:	eb 02                	jmp    0x18001541e
   18001541c:	33 ff                	xor    edi,edi
   18001541e:	8b 0d 74 9f 27 01    	mov    ecx,DWORD PTR [rip+0x1279f74]        # 0x18128f398
   180015424:	ff 15 5e a8 5c 00    	call   QWORD PTR [rip+0x5ca85e]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   18001542a:	8b d0                	mov    edx,eax
   18001542c:	4c 8b c7             	mov    r8,rdi
   18001542f:	48 8d 4d 80          	lea    rcx,[rbp-0x80]
   180015433:	ff 15 47 a8 5c 00    	call   QWORD PTR [rip+0x5ca847]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   180015439:	45 33 c0             	xor    r8d,r8d
   18001543c:	48 8d 55 80          	lea    rdx,[rbp-0x80]
   180015440:	8b 0d 52 9f 27 01    	mov    ecx,DWORD PTR [rip+0x1279f52]        # 0x18128f398
   180015446:	ff 15 44 ab 5c 00    	call   QWORD PTR [rip+0x5cab44]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   18001544c:	8b 0d 46 9f 27 01    	mov    ecx,DWORD PTR [rip+0x1279f46]        # 0x18128f398
   180015452:	ff 15 30 a8 5c 00    	call   QWORD PTR [rip+0x5ca830]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   180015458:	8b d0                	mov    edx,eax
   18001545a:	4c 8b c7             	mov    r8,rdi
   18001545d:	48 8d 0d 3c 9f 27 01 	lea    rcx,[rip+0x1279f3c]        # 0x18128f3a0
   180015464:	ff 15 26 a8 5c 00    	call   QWORD PTR [rip+0x5ca826]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   18001546a:	48 8d 3d 57 9f 27 01 	lea    rdi,[rip+0x1279f57]        # 0x18128f3c8
   180015471:	b8 40 00 00 00       	mov    eax,0x40
   180015476:	41 8b 04 07          	mov    eax,DWORD PTR [r15+rax*1]
   18001547a:	39 05 30 9f 27 01    	cmp    DWORD PTR [rip+0x1279f30],eax        # 0x18128f3b0
   180015480:	7e 67                	jle    0x1800154e9
   180015482:	48 8d 0d 27 9f 27 01 	lea    rcx,[rip+0x1279f27]        # 0x18128f3b0
   180015489:	e8 d6 29 57 00       	call   0x180587e64
   18001548e:	83 3d 1b 9f 27 01 ff 	cmp    DWORD PTR [rip+0x1279f1b],0xffffffff        # 0x18128f3b0
   180015495:	75 52                	jne    0x1800154e9
   180015497:	48 c7 05 16 9f 27 01 	mov    QWORD PTR [rip+0x1279f16],0x0        # 0x18128f3b8
   18001549e:	00 00 00 00 
   1800154a2:	48 8d 15 1f a4 60 00 	lea    rdx,[rip+0x60a41f]        # 0x18061f8c8 ; 'g_DLF_rwtColorSource'
   1800154a9:	48 8d 0d 10 9f 27 01 	lea    rcx,[rip+0x1279f10]        # 0x18128f3c0
   1800154b0:	e8 bb ad 1f 00       	call   0x180210270
   1800154b5:	c7 05 05 9f 27 01 09 	mov    DWORD PTR [rip+0x1279f05],0x9        # 0x18128f3c4
   1800154bc:	00 00 00 
   1800154bf:	48 89 3d f2 9e 27 01 	mov    QWORD PTR [rip+0x1279ef2],rdi        # 0x18128f3b8
   1800154c6:	48 8b cf             	mov    rcx,rdi
   1800154c9:	ff 15 a1 a7 5c 00    	call   QWORD PTR [rip+0x5ca7a1]        # 0x1805dfc70 ; ??0ShaderTexture@d3d@@QEAA@XZ
   1800154cf:	90                   	nop
   1800154d0:	48 8d 0d 79 38 5b 00 	lea    rcx,[rip+0x5b3879]        # 0x1805c8d50
   1800154d7:	e8 e8 26 57 00       	call   0x180587bc4
   1800154dc:	90                   	nop
   1800154dd:	48 8d 0d cc 9e 27 01 	lea    rcx,[rip+0x1279ecc]        # 0x18128f3b0
   1800154e4:	e8 1b 29 57 00       	call   0x180587e04
   1800154e9:	48 85 f6             	test   rsi,rsi
   1800154ec:	74 0e                	je     0x1800154fc
   1800154ee:	48 8b ce             	mov    rcx,rsi
   1800154f1:	ff 15 39 a8 5c 00    	call   QWORD PTR [rip+0x5ca839]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   1800154f7:	48 8b f8             	mov    rdi,rax
   1800154fa:	eb 02                	jmp    0x1800154fe
   1800154fc:	33 ff                	xor    edi,edi
   1800154fe:	8b 0d bc 9e 27 01    	mov    ecx,DWORD PTR [rip+0x1279ebc]        # 0x18128f3c0
   180015504:	ff 15 7e a7 5c 00    	call   QWORD PTR [rip+0x5ca77e]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   18001550a:	8b d0                	mov    edx,eax
   18001550c:	4c 8b c7             	mov    r8,rdi
   18001550f:	48 8d 4d 80          	lea    rcx,[rbp-0x80]
   180015513:	ff 15 67 a7 5c 00    	call   QWORD PTR [rip+0x5ca767]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   180015519:	45 33 c0             	xor    r8d,r8d
   18001551c:	48 8d 55 80          	lea    rdx,[rbp-0x80]
   180015520:	8b 0d 9a 9e 27 01    	mov    ecx,DWORD PTR [rip+0x1279e9a]        # 0x18128f3c0
   180015526:	ff 15 64 aa 5c 00    	call   QWORD PTR [rip+0x5caa64]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   18001552c:	8b 0d 8e 9e 27 01    	mov    ecx,DWORD PTR [rip+0x1279e8e]        # 0x18128f3c0
   180015532:	ff 15 50 a7 5c 00    	call   QWORD PTR [rip+0x5ca750]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   180015538:	8b d0                	mov    edx,eax
   18001553a:	4c 8b c7             	mov    r8,rdi
   18001553d:	48 8d 0d 84 9e 27 01 	lea    rcx,[rip+0x1279e84]        # 0x18128f3c8
   180015544:	ff 15 46 a7 5c 00    	call   QWORD PTR [rip+0x5ca746]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   18001554a:	b8 40 00 00 00       	mov    eax,0x40
   18001554f:	41 8b 04 07          	mov    eax,DWORD PTR [r15+rax*1]
   180015553:	39 05 7f 9e 27 01    	cmp    DWORD PTR [rip+0x1279e7f],eax        # 0x18128f3d8
   180015559:	7e 64                	jle    0x1800155bf
   18001555b:	48 8d 0d 76 9e 27 01 	lea    rcx,[rip+0x1279e76]        # 0x18128f3d8
   180015562:	e8 fd 28 57 00       	call   0x180587e64
   180015567:	83 3d 6a 9e 27 01 ff 	cmp    DWORD PTR [rip+0x1279e6a],0xffffffff        # 0x18128f3d8
   18001556e:	75 4f                	jne    0x1800155bf
   180015570:	33 ff                	xor    edi,edi
   180015572:	48 89 3d 67 9e 27 01 	mov    QWORD PTR [rip+0x1279e67],rdi        # 0x18128f3e0
   180015579:	48 8d 15 60 a3 60 00 	lea    rdx,[rip+0x60a360]        # 0x18061f8e0 ; 'g_DLF_iFilterPass'
   180015580:	48 8d 0d 61 9e 27 01 	lea    rcx,[rip+0x1279e61]        # 0x18128f3e8
   180015587:	e8 e4 ac 1f 00       	call   0x180210270
   18001558c:	c7 05 56 9e 27 01 10 	mov    DWORD PTR [rip+0x1279e56],0x10        # 0x18128f3ec
   180015593:	00 00 00 
   180015596:	48 8d 05 53 9e 27 01 	lea    rax,[rip+0x1279e53]        # 0x18128f3f0
   18001559d:	48 89 05 3c 9e 27 01 	mov    QWORD PTR [rip+0x1279e3c],rax        # 0x18128f3e0
   1800155a4:	48 8d 0d 75 37 5b 00 	lea    rcx,[rip+0x5b3775]        # 0x1805c8d20
   1800155ab:	e8 14 26 57 00       	call   0x180587bc4
   1800155b0:	90                   	nop
   1800155b1:	48 8d 0d 20 9e 27 01 	lea    rcx,[rip+0x1279e20]        # 0x18128f3d8
   1800155b8:	e8 47 28 57 00       	call   0x180587e04
   1800155bd:	eb 02                	jmp    0x1800155c1
   1800155bf:	33 ff                	xor    edi,edi
   1800155c1:	89 bd d8 01 00 00    	mov    DWORD PTR [rbp+0x1d8],edi
   1800155c7:	45 33 c0             	xor    r8d,r8d
   1800155ca:	48 8d 95 d8 01 00 00 	lea    rdx,[rbp+0x1d8]
   1800155d1:	8b 0d 11 9e 27 01    	mov    ecx,DWORD PTR [rip+0x1279e11]        # 0x18128f3e8
   1800155d7:	ff 15 b3 a9 5c 00    	call   QWORD PTR [rip+0x5ca9b3]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   1800155dd:	8b 85 d8 01 00 00    	mov    eax,DWORD PTR [rbp+0x1d8]
   1800155e3:	89 05 07 9e 27 01    	mov    DWORD PTR [rip+0x1279e07],eax        # 0x18128f3f0
   1800155e9:	48 8b 05 a8 7e 27 01 	mov    rax,QWORD PTR [rip+0x1277ea8]        # 0x18128d498
   1800155f0:	48 85 c0             	test   rax,rax
   1800155f3:	75 4a                	jne    0x18001563f
   1800155f5:	48 8d 15 fc a2 60 00 	lea    rdx,[rip+0x60a2fc]        # 0x18061f8f8 ; 'deferredlight_filtering_diffuse.rfx'
   1800155fc:	48 8b 0d cd d6 86 00 	mov    rcx,QWORD PTR [rip+0x86d6cd]        # 0x180882cd0
   180015603:	e8 58 43 1c 00       	call   0x1801d9960
   180015608:	48 8b f8             	mov    rdi,rax
   18001560b:	48 8d 15 0e a3 60 00 	lea    rdx,[rip+0x60a30e]        # 0x18061f920 ; 'temporal_feedback'
   180015612:	48 8b c8             	mov    rcx,rax
   180015615:	e8 46 d6 1b 00       	call   0x1801d2c60
   18001561a:	48 85 c0             	test   rax,rax
   18001561d:	0f 84 4f 05 00 00    	je     0x180015b72
   180015623:	48 89 05 6e 7e 27 01 	mov    QWORD PTR [rip+0x1277e6e],rax        # 0x18128d498
   18001562a:	48 8d 0d 67 7e 27 01 	lea    rcx,[rip+0x1277e67]        # 0x18128d498
   180015631:	e8 ca 30 1c 00       	call   0x1801d8700
   180015636:	48 8b 05 5b 7e 27 01 	mov    rax,QWORD PTR [rip+0x1277e5b]        # 0x18128d498
   18001563d:	33 ff                	xor    edi,edi
   18001563f:	44 8b 50 08          	mov    r10d,DWORD PTR [rax+0x8]
   180015643:	41 f7 d2             	not    r10d
   180015646:	44 23 50 04          	and    r10d,DWORD PTR [rax+0x4]
   18001564a:	44 8b cf             	mov    r9d,edi
   18001564d:	44 8b 80 40 02 00 00 	mov    r8d,DWORD PTR [rax+0x240]
   180015654:	41 83 e8 01          	sub    r8d,0x1
   180015658:	78 31                	js     0x18001568b
   18001565a:	4c 8b 98 38 02 00 00 	mov    r11,QWORD PTR [rax+0x238]
   180015661:	43 8d 0c 08          	lea    ecx,[r8+r9*1]
   180015665:	d1 f9                	sar    ecx,1
   180015667:	48 63 c1             	movsxd rax,ecx
   18001566a:	48 69 d0 a8 00 00 00 	imul   rdx,rax,0xa8
   180015671:	49 03 d3             	add    rdx,r11
   180015674:	44 3b 52 04          	cmp    r10d,DWORD PTR [rdx+0x4]
   180015678:	73 06                	jae    0x180015680
   18001567a:	44 8d 41 ff          	lea    r8d,[rcx-0x1]
   18001567e:	eb 06                	jmp    0x180015686
   180015680:	76 0c                	jbe    0x18001568e
   180015682:	44 8d 49 01          	lea    r9d,[rcx+0x1]
   180015686:	45 3b c8             	cmp    r9d,r8d
   180015689:	7e d6                	jle    0x180015661
   18001568b:	48 8b d7             	mov    rdx,rdi
   18001568e:	48 8b ca             	mov    rcx,rdx
   180015691:	e8 ba 60 1c 00       	call   0x1801db750
   180015696:	41 8d 44 24 07       	lea    eax,[r12+0x7]
   18001569b:	99                   	cdq
   18001569c:	83 e2 07             	and    edx,0x7
   18001569f:	8d 0c 02             	lea    ecx,[rdx+rax*1]
   1800156a2:	c1 f9 03             	sar    ecx,0x3
   1800156a5:	89 4d 94             	mov    DWORD PTR [rbp-0x6c],ecx
   1800156a8:	41 8d 45 07          	lea    eax,[r13+0x7]
   1800156ac:	99                   	cdq
   1800156ad:	83 e2 07             	and    edx,0x7
   1800156b0:	03 c2                	add    eax,edx
   1800156b2:	c1 f8 03             	sar    eax,0x3
   1800156b5:	89 45 a8             	mov    DWORD PTR [rbp-0x58],eax
   1800156b8:	89 4d ac             	mov    DWORD PTR [rbp-0x54],ecx
   1800156bb:	f2 0f 10 45 a8       	movsd  xmm0,QWORD PTR [rbp-0x58]
   1800156c0:	f2 0f 11 45 80       	movsd  QWORD PTR [rbp-0x80],xmm0
   1800156c5:	c7 45 88 01 00 00 00 	mov    DWORD PTR [rbp-0x78],0x1
   1800156cc:	48 8d 55 80          	lea    rdx,[rbp-0x80]
   1800156d0:	49 8b ce             	mov    rcx,r14
   1800156d3:	ff 15 e7 a5 5c 00    	call   QWORD PTR [rip+0x5ca5e7]        # 0x1805dfcc0 ; ?dispatch@DeviceUtil@d3d@@SAXAEAVDeviceState@2@V?$Vector3Template@H@m@@@Z
   1800156d9:	4c 8b e6             	mov    r12,rsi
   1800156dc:	4c 8b f3             	mov    r14,rbx
   1800156df:	8b f7                	mov    esi,edi
   1800156e1:	44 8b ad d0 01 00 00 	mov    r13d,DWORD PTR [rbp+0x1d0]
   1800156e8:	45 85 ed             	test   r13d,r13d
   1800156eb:	0f 84 51 03 00 00    	je     0x180015a42
   1800156f1:	b8 40 00 00 00       	mov    eax,0x40
   1800156f6:	41 8b 04 07          	mov    eax,DWORD PTR [r15+rax*1]
   1800156fa:	39 05 f8 9c 27 01    	cmp    DWORD PTR [rip+0x1279cf8],eax        # 0x18128f3f8
   180015700:	7e 6a                	jle    0x18001576c
   180015702:	48 8d 0d ef 9c 27 01 	lea    rcx,[rip+0x1279cef]        # 0x18128f3f8
   180015709:	e8 56 27 57 00       	call   0x180587e64
   18001570e:	83 3d e3 9c 27 01 ff 	cmp    DWORD PTR [rip+0x1279ce3],0xffffffff        # 0x18128f3f8
   180015715:	75 55                	jne    0x18001576c
   180015717:	48 89 3d e2 9c 27 01 	mov    QWORD PTR [rip+0x1279ce2],rdi        # 0x18128f400
   18001571e:	48 8d 15 8b a1 60 00 	lea    rdx,[rip+0x60a18b]        # 0x18061f8b0 ; 'g_DLF_rwtColorTarget'
   180015725:	48 8d 0d dc 9c 27 01 	lea    rcx,[rip+0x1279cdc]        # 0x18128f408
   18001572c:	e8 3f ab 1f 00       	call   0x180210270
   180015731:	c7 05 d1 9c 27 01 09 	mov    DWORD PTR [rip+0x1279cd1],0x9        # 0x18128f40c
   180015738:	00 00 00 
   18001573b:	48 8d 05 ce 9c 27 01 	lea    rax,[rip+0x1279cce]        # 0x18128f410
   180015742:	48 89 05 b7 9c 27 01 	mov    QWORD PTR [rip+0x1279cb7],rax        # 0x18128f400
   180015749:	48 8b c8             	mov    rcx,rax
   18001574c:	ff 15 1e a5 5c 00    	call   QWORD PTR [rip+0x5ca51e]        # 0x1805dfc70 ; ??0ShaderTexture@d3d@@QEAA@XZ
   180015752:	90                   	nop
   180015753:	48 8d 0d 96 35 5b 00 	lea    rcx,[rip+0x5b3596]        # 0x1805c8cf0
   18001575a:	e8 65 24 57 00       	call   0x180587bc4
   18001575f:	90                   	nop
   180015760:	48 8d 0d 91 9c 27 01 	lea    rcx,[rip+0x1279c91]        # 0x18128f3f8
   180015767:	e8 98 26 57 00       	call   0x180587e04
   18001576c:	4d 85 e4             	test   r12,r12
   18001576f:	74 0c                	je     0x18001577d
   180015771:	49 8b cc             	mov    rcx,r12
   180015774:	ff 15 b6 a5 5c 00    	call   QWORD PTR [rip+0x5ca5b6]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   18001577a:	48 8b f8             	mov    rdi,rax
   18001577d:	8b 0d 85 9c 27 01    	mov    ecx,DWORD PTR [rip+0x1279c85]        # 0x18128f408
   180015783:	ff 15 ff a4 5c 00    	call   QWORD PTR [rip+0x5ca4ff]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   180015789:	8b d0                	mov    edx,eax
   18001578b:	4c 8b c7             	mov    r8,rdi
   18001578e:	48 8d 4d 58          	lea    rcx,[rbp+0x58]
   180015792:	ff 15 e8 a4 5c 00    	call   QWORD PTR [rip+0x5ca4e8]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   180015798:	45 33 c0             	xor    r8d,r8d
   18001579b:	48 8d 55 58          	lea    rdx,[rbp+0x58]
   18001579f:	8b 0d 63 9c 27 01    	mov    ecx,DWORD PTR [rip+0x1279c63]        # 0x18128f408
   1800157a5:	ff 15 e5 a7 5c 00    	call   QWORD PTR [rip+0x5ca7e5]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   1800157ab:	8b 0d 57 9c 27 01    	mov    ecx,DWORD PTR [rip+0x1279c57]        # 0x18128f408
   1800157b1:	ff 15 d1 a4 5c 00    	call   QWORD PTR [rip+0x5ca4d1]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   1800157b7:	8b d0                	mov    edx,eax
   1800157b9:	4c 8b c7             	mov    r8,rdi
   1800157bc:	48 8d 0d 4d 9c 27 01 	lea    rcx,[rip+0x1279c4d]        # 0x18128f410
   1800157c3:	ff 15 c7 a4 5c 00    	call   QWORD PTR [rip+0x5ca4c7]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   1800157c9:	b9 40 00 00 00       	mov    ecx,0x40
   1800157ce:	41 8b 04 0f          	mov    eax,DWORD PTR [r15+rcx*1]
   1800157d2:	39 05 48 9c 27 01    	cmp    DWORD PTR [rip+0x1279c48],eax        # 0x18128f420
   1800157d8:	7e 6e                	jle    0x180015848
   1800157da:	48 8d 0d 3f 9c 27 01 	lea    rcx,[rip+0x1279c3f]        # 0x18128f420
   1800157e1:	e8 7e 26 57 00       	call   0x180587e64
   1800157e6:	83 3d 33 9c 27 01 ff 	cmp    DWORD PTR [rip+0x1279c33],0xffffffff        # 0x18128f420
   1800157ed:	75 59                	jne    0x180015848
   1800157ef:	48 c7 05 2e 9c 27 01 	mov    QWORD PTR [rip+0x1279c2e],0x0        # 0x18128f428
   1800157f6:	00 00 00 00 
   1800157fa:	48 8d 15 c7 a0 60 00 	lea    rdx,[rip+0x60a0c7]        # 0x18061f8c8 ; 'g_DLF_rwtColorSource'
   180015801:	48 8d 0d 28 9c 27 01 	lea    rcx,[rip+0x1279c28]        # 0x18128f430
   180015808:	e8 63 aa 1f 00       	call   0x180210270
   18001580d:	c7 05 1d 9c 27 01 09 	mov    DWORD PTR [rip+0x1279c1d],0x9        # 0x18128f434
   180015814:	00 00 00 
   180015817:	48 8d 05 1a 9c 27 01 	lea    rax,[rip+0x1279c1a]        # 0x18128f438
   18001581e:	48 89 05 03 9c 27 01 	mov    QWORD PTR [rip+0x1279c03],rax        # 0x18128f428
   180015825:	48 8b c8             	mov    rcx,rax
   180015828:	ff 15 42 a4 5c 00    	call   QWORD PTR [rip+0x5ca442]        # 0x1805dfc70 ; ??0ShaderTexture@d3d@@QEAA@XZ
   18001582e:	90                   	nop
   18001582f:	48 8d 0d 8a 34 5b 00 	lea    rcx,[rip+0x5b348a]        # 0x1805c8cc0
   180015836:	e8 89 23 57 00       	call   0x180587bc4
   18001583b:	90                   	nop
   18001583c:	48 8d 0d dd 9b 27 01 	lea    rcx,[rip+0x1279bdd]        # 0x18128f420
   180015843:	e8 bc 25 57 00       	call   0x180587e04
   180015848:	4d 85 f6             	test   r14,r14
   18001584b:	74 0e                	je     0x18001585b
   18001584d:	49 8b ce             	mov    rcx,r14
   180015850:	ff 15 da a4 5c 00    	call   QWORD PTR [rip+0x5ca4da]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   180015856:	48 8b f8             	mov    rdi,rax
   180015859:	eb 02                	jmp    0x18001585d
   18001585b:	33 ff                	xor    edi,edi
   18001585d:	8b 0d cd 9b 27 01    	mov    ecx,DWORD PTR [rip+0x1279bcd]        # 0x18128f430
   180015863:	ff 15 1f a4 5c 00    	call   QWORD PTR [rip+0x5ca41f]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   180015869:	8b d0                	mov    edx,eax
   18001586b:	4c 8b c7             	mov    r8,rdi
   18001586e:	48 8d 4d e0          	lea    rcx,[rbp-0x20]
   180015872:	ff 15 08 a4 5c 00    	call   QWORD PTR [rip+0x5ca408]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   180015878:	45 33 c0             	xor    r8d,r8d
   18001587b:	48 8d 55 e0          	lea    rdx,[rbp-0x20]
   18001587f:	8b 0d ab 9b 27 01    	mov    ecx,DWORD PTR [rip+0x1279bab]        # 0x18128f430
   180015885:	ff 15 05 a7 5c 00    	call   QWORD PTR [rip+0x5ca705]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   18001588b:	8b 0d 9f 9b 27 01    	mov    ecx,DWORD PTR [rip+0x1279b9f]        # 0x18128f430
   180015891:	ff 15 f1 a3 5c 00    	call   QWORD PTR [rip+0x5ca3f1]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   180015897:	8b d0                	mov    edx,eax
   180015899:	4c 8b c7             	mov    r8,rdi
   18001589c:	48 8d 0d 95 9b 27 01 	lea    rcx,[rip+0x1279b95]        # 0x18128f438
   1800158a3:	ff 15 e7 a3 5c 00    	call   QWORD PTR [rip+0x5ca3e7]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   1800158a9:	b9 40 00 00 00       	mov    ecx,0x40
   1800158ae:	41 8b 04 0f          	mov    eax,DWORD PTR [r15+rcx*1]
   1800158b2:	39 05 90 9b 27 01    	cmp    DWORD PTR [rip+0x1279b90],eax        # 0x18128f448
   1800158b8:	7e 64                	jle    0x18001591e
   1800158ba:	48 8d 0d 87 9b 27 01 	lea    rcx,[rip+0x1279b87]        # 0x18128f448
   1800158c1:	e8 9e 25 57 00       	call   0x180587e64
   1800158c6:	83 3d 7b 9b 27 01 ff 	cmp    DWORD PTR [rip+0x1279b7b],0xffffffff        # 0x18128f448
   1800158cd:	75 4f                	jne    0x18001591e
   1800158cf:	48 c7 05 76 9b 27 01 	mov    QWORD PTR [rip+0x1279b76],0x0        # 0x18128f450
   1800158d6:	00 00 00 00 
   1800158da:	48 8d 15 ff 9f 60 00 	lea    rdx,[rip+0x609fff]        # 0x18061f8e0 ; 'g_DLF_iFilterPass'
   1800158e1:	48 8d 0d 70 9b 27 01 	lea    rcx,[rip+0x1279b70]        # 0x18128f458
   1800158e8:	e8 83 a9 1f 00       	call   0x180210270
   1800158ed:	c7 05 65 9b 27 01 10 	mov    DWORD PTR [rip+0x1279b65],0x10        # 0x18128f45c
   1800158f4:	00 00 00 
   1800158f7:	48 8d 05 62 9b 27 01 	lea    rax,[rip+0x1279b62]        # 0x18128f460
   1800158fe:	48 89 05 4b 9b 27 01 	mov    QWORD PTR [rip+0x1279b4b],rax        # 0x18128f450
   180015905:	48 8d 0d 84 33 5b 00 	lea    rcx,[rip+0x5b3384]        # 0x1805c8c90
   18001590c:	e8 b3 22 57 00       	call   0x180587bc4
   180015911:	90                   	nop
   180015912:	48 8d 0d 2f 9b 27 01 	lea    rcx,[rip+0x1279b2f]        # 0x18128f448
   180015919:	e8 e6 24 57 00       	call   0x180587e04
   18001591e:	89 b5 d8 01 00 00    	mov    DWORD PTR [rbp+0x1d8],esi
   180015924:	45 33 c0             	xor    r8d,r8d
   180015927:	48 8d 95 d8 01 00 00 	lea    rdx,[rbp+0x1d8]
   18001592e:	8b 0d 24 9b 27 01    	mov    ecx,DWORD PTR [rip+0x1279b24]        # 0x18128f458
   180015934:	ff 15 56 a6 5c 00    	call   QWORD PTR [rip+0x5ca656]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   18001593a:	8b 85 d8 01 00 00    	mov    eax,DWORD PTR [rbp+0x1d8]
   180015940:	89 05 1a 9b 27 01    	mov    DWORD PTR [rip+0x1279b1a],eax        # 0x18128f460
   180015946:	8b fe                	mov    edi,esi
   180015948:	83 e7 01             	and    edi,0x1
   18001594b:	48 8b 0d 3e 7b 27 01 	mov    rcx,QWORD PTR [rip+0x1277b3e]        # 0x18128d490
   180015952:	48 85 c9             	test   rcx,rcx
   180015955:	75 4f                	jne    0x1800159a6
   180015957:	48 8d 15 9a 9f 60 00 	lea    rdx,[rip+0x609f9a]        # 0x18061f8f8 ; 'deferredlight_filtering_diffuse.rfx'
   18001595e:	48 8b 0d 6b d3 86 00 	mov    rcx,QWORD PTR [rip+0x86d36b]        # 0x180882cd0
   180015965:	e8 f6 3f 1c 00       	call   0x1801d9960
   18001596a:	4c 8b e8             	mov    r13,rax
   18001596d:	48 8d 15 c4 9f 60 00 	lea    rdx,[rip+0x609fc4]        # 0x18061f938 ; 'filter_color'
   180015974:	48 8b c8             	mov    rcx,rax
   180015977:	e8 e4 d2 1b 00       	call   0x1801d2c60
   18001597c:	48 85 c0             	test   rax,rax
   18001597f:	0f 84 1b 02 00 00    	je     0x180015ba0
   180015985:	48 89 05 04 7b 27 01 	mov    QWORD PTR [rip+0x1277b04],rax        # 0x18128d490
   18001598c:	48 8d 0d fd 7a 27 01 	lea    rcx,[rip+0x1277afd]        # 0x18128d490
   180015993:	e8 68 2d 1c 00       	call   0x1801d8700
   180015998:	48 8b 0d f1 7a 27 01 	mov    rcx,QWORD PTR [rip+0x1277af1]        # 0x18128d490
   18001599f:	44 8b ad d0 01 00 00 	mov    r13d,DWORD PTR [rbp+0x1d0]
   1800159a6:	44 8b 51 04          	mov    r10d,DWORD PTR [rcx+0x4]
   1800159aa:	44 0b d7             	or     r10d,edi
   1800159ad:	8b 41 08             	mov    eax,DWORD PTR [rcx+0x8]
   1800159b0:	f7 d0                	not    eax
   1800159b2:	44 23 d0             	and    r10d,eax
   1800159b5:	33 ff                	xor    edi,edi
   1800159b7:	44 8b cf             	mov    r9d,edi
   1800159ba:	44 8b 81 40 02 00 00 	mov    r8d,DWORD PTR [rcx+0x240]
   1800159c1:	41 83 e8 01          	sub    r8d,0x1
   1800159c5:	78 31                	js     0x1800159f8
   1800159c7:	4c 8b 99 38 02 00 00 	mov    r11,QWORD PTR [rcx+0x238]
   1800159ce:	43 8d 0c 01          	lea    ecx,[r9+r8*1]
   1800159d2:	d1 f9                	sar    ecx,1
   1800159d4:	48 63 c1             	movsxd rax,ecx
   1800159d7:	48 69 d0 a8 00 00 00 	imul   rdx,rax,0xa8
   1800159de:	49 03 d3             	add    rdx,r11
   1800159e1:	44 3b 52 04          	cmp    r10d,DWORD PTR [rdx+0x4]
   1800159e5:	73 06                	jae    0x1800159ed
   1800159e7:	44 8d 41 ff          	lea    r8d,[rcx-0x1]
   1800159eb:	eb 06                	jmp    0x1800159f3
   1800159ed:	76 0c                	jbe    0x1800159fb
   1800159ef:	44 8d 49 01          	lea    r9d,[rcx+0x1]
   1800159f3:	45 3b c8             	cmp    r9d,r8d
   1800159f6:	7e d6                	jle    0x1800159ce
   1800159f8:	48 8b d7             	mov    rdx,rdi
   1800159fb:	48 8b ca             	mov    rcx,rdx
   1800159fe:	e8 4d 5d 1c 00       	call   0x1801db750
   180015a03:	8b 45 a8             	mov    eax,DWORD PTR [rbp-0x58]
   180015a06:	89 45 80             	mov    DWORD PTR [rbp-0x80],eax
   180015a09:	8b 45 94             	mov    eax,DWORD PTR [rbp-0x6c]
   180015a0c:	89 45 84             	mov    DWORD PTR [rbp-0x7c],eax
   180015a0f:	f2 0f 10 45 80       	movsd  xmm0,QWORD PTR [rbp-0x80]
   180015a14:	f2 0f 11 45 c0       	movsd  QWORD PTR [rbp-0x40],xmm0
   180015a19:	c7 45 c8 01 00 00 00 	mov    DWORD PTR [rbp-0x38],0x1
   180015a20:	48 8d 55 c0          	lea    rdx,[rbp-0x40]
   180015a24:	48 8b 4d 98          	mov    rcx,QWORD PTR [rbp-0x68]
   180015a28:	ff 15 92 a2 5c 00    	call   QWORD PTR [rip+0x5ca292]        # 0x1805dfcc0 ; ?dispatch@DeviceUtil@d3d@@SAXAEAVDeviceState@2@V?$Vector3Template@H@m@@@Z
   180015a2e:	49 8b c4             	mov    rax,r12
   180015a31:	4d 8b e6             	mov    r12,r14
   180015a34:	4c 8b f0             	mov    r14,rax
   180015a37:	ff c6                	inc    esi
   180015a39:	41 3b f5             	cmp    esi,r13d
   180015a3c:	0f 82 af fc ff ff    	jb     0x1800156f1
   180015a42:	48 c7 45 a8 00 00 00 	mov    QWORD PTR [rbp-0x58],0x0
   180015a49:	00 
   180015a4a:	89 7d b0             	mov    DWORD PTR [rbp-0x50],edi
   180015a4d:	48 c7 45 98 00 00 00 	mov    QWORD PTR [rbp-0x68],0x0
   180015a54:	00 
   180015a55:	89 7d a0             	mov    DWORD PTR [rbp-0x60],edi
   180015a58:	48 8b bd c8 01 00 00 	mov    rdi,QWORD PTR [rbp+0x1c8]
   180015a5f:	48 8b 3f             	mov    rdi,QWORD PTR [rdi]
   180015a62:	49 8b ce             	mov    rcx,r14
   180015a65:	ff 15 c5 a2 5c 00    	call   QWORD PTR [rip+0x5ca2c5]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   180015a6b:	48 8b f0             	mov    rsi,rax
   180015a6e:	48 8b cf             	mov    rcx,rdi
   180015a71:	ff 15 b9 a2 5c 00    	call   QWORD PTR [rip+0x5ca2b9]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   180015a77:	48 8b c8             	mov    rcx,rax
   180015a7a:	45 33 ff             	xor    r15d,r15d
   180015a7d:	4c 89 7c 24 40       	mov    QWORD PTR [rsp+0x40],r15
   180015a82:	48 8d 45 a8          	lea    rax,[rbp-0x58]
   180015a86:	48 89 44 24 38       	mov    QWORD PTR [rsp+0x38],rax
   180015a8b:	44 89 7c 24 30       	mov    DWORD PTR [rsp+0x30],r15d
   180015a90:	44 89 7c 24 28       	mov    DWORD PTR [rsp+0x28],r15d
   180015a95:	48 89 74 24 20       	mov    QWORD PTR [rsp+0x20],rsi
   180015a9a:	4c 8d 4d 98          	lea    r9,[rbp-0x68]
   180015a9e:	45 33 c0             	xor    r8d,r8d
   180015aa1:	33 d2                	xor    edx,edx
   180015aa3:	ff 15 9f a2 5c 00    	call   QWORD PTR [rip+0x5ca29f]        # 0x1805dfd48 ; ?copyRegion@NativeTextureUtil@d3d@@SAXPEAVNativeTexture@2@HHAEBV?$Vector3Template@H@m@@0HH1PEBV45@@Z
   180015aa9:	48 8b b5 c0 01 00 00 	mov    rsi,QWORD PTR [rbp+0x1c0]
   180015ab0:	4c 3b f6             	cmp    r14,rsi
   180015ab3:	74 55                	je     0x180015b0a
   180015ab5:	4c 89 7d c0          	mov    QWORD PTR [rbp-0x40],r15
   180015ab9:	44 89 7d c8          	mov    DWORD PTR [rbp-0x38],r15d
   180015abd:	4c 89 7d 80          	mov    QWORD PTR [rbp-0x80],r15
   180015ac1:	44 89 7d 88          	mov    DWORD PTR [rbp-0x78],r15d
   180015ac5:	48 8b cb             	mov    rcx,rbx
   180015ac8:	ff 15 62 a2 5c 00    	call   QWORD PTR [rip+0x5ca262]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   180015ace:	48 8b f8             	mov    rdi,rax
   180015ad1:	48 8b ce             	mov    rcx,rsi
   180015ad4:	ff 15 56 a2 5c 00    	call   QWORD PTR [rip+0x5ca256]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   180015ada:	48 8b c8             	mov    rcx,rax
   180015add:	4c 89 7c 24 40       	mov    QWORD PTR [rsp+0x40],r15
   180015ae2:	48 8d 45 c0          	lea    rax,[rbp-0x40]
   180015ae6:	48 89 44 24 38       	mov    QWORD PTR [rsp+0x38],rax
   180015aeb:	44 89 7c 24 30       	mov    DWORD PTR [rsp+0x30],r15d
   180015af0:	44 89 7c 24 28       	mov    DWORD PTR [rsp+0x28],r15d
   180015af5:	48 89 7c 24 20       	mov    QWORD PTR [rsp+0x20],rdi
   180015afa:	4c 8d 4d 80          	lea    r9,[rbp-0x80]
   180015afe:	45 33 c0             	xor    r8d,r8d
   180015b01:	33 d2                	xor    edx,edx
   180015b03:	ff 15 3f a2 5c 00    	call   QWORD PTR [rip+0x5ca23f]        # 0x1805dfd48 ; ?copyRegion@NativeTextureUtil@d3d@@SAXPEAVNativeTexture@2@HHAEBV?$Vector3Template@H@m@@0HH1PEBV45@@Z
   180015b09:	90                   	nop
   180015b0a:	48 85 db             	test   rbx,rbx
   180015b0d:	74 10                	je     0x180015b1f
   180015b0f:	48 8b d3             	mov    rdx,rbx
   180015b12:	48 8b 0d bf cd 7e 00 	mov    rcx,QWORD PTR [rip+0x7ecdbf]        # 0x1808028d8
   180015b19:	e8 e2 42 09 00       	call   0x1800a9e00
   180015b1e:	90                   	nop
   180015b1f:	4c 8d 9c 24 78 02 00 	lea    r11,[rsp+0x278]
   180015b26:	00 
   180015b27:	41 0f 28 73 e8       	movaps xmm6,XMMWORD PTR [r11-0x18]
   180015b2c:	41 0f 28 7b d8       	movaps xmm7,XMMWORD PTR [r11-0x28]
   180015b31:	45 0f 28 43 c8       	movaps xmm8,XMMWORD PTR [r11-0x38]
   180015b36:	45 0f 28 4b b8       	movaps xmm9,XMMWORD PTR [r11-0x48]
   180015b3b:	45 0f 28 53 a8       	movaps xmm10,XMMWORD PTR [r11-0x58]
   180015b40:	45 0f 28 5b 98       	movaps xmm11,XMMWORD PTR [r11-0x68]
   180015b45:	45 0f 28 63 88       	movaps xmm12,XMMWORD PTR [r11-0x78]
   180015b4a:	45 0f 28 ab 78 ff ff 	movaps xmm13,XMMWORD PTR [r11-0x88]
   180015b51:	ff 
   180015b52:	45 0f 28 b3 68 ff ff 	movaps xmm14,XMMWORD PTR [r11-0x98]
   180015b59:	ff 
   180015b5a:	45 0f 28 bb 58 ff ff 	movaps xmm15,XMMWORD PTR [r11-0xa8]
   180015b61:	ff 
   180015b62:	49 8b e3             	mov    rsp,r11
   180015b65:	41 5f                	pop    r15
   180015b67:	41 5e                	pop    r14
   180015b69:	41 5d                	pop    r13
   180015b6b:	41 5c                	pop    r12
   180015b6d:	5f                   	pop    rdi
   180015b6e:	5e                   	pop    rsi
   180015b6f:	5b                   	pop    rbx
   180015b70:	5d                   	pop    rbp
   180015b71:	c3                   	ret
   180015b72:	4c 8d 05 a7 9d 60 00 	lea    r8,[rip+0x609da7]        # 0x18061f920 ; 'temporal_feedback'
   180015b79:	48 8b 57 08          	mov    rdx,QWORD PTR [rdi+0x8]
   180015b7d:	48 8d 0d c4 2c 62 00 	lea    rcx,[rip+0x622cc4]        # 0x180638848 ; 'Effect "%s" could not find technique "%s"'
   180015b84:	ff 15 6e b7 5c 00    	call   QWORD PTR [rip+0x5cb76e]        # 0x1805e12f8 ; ?str@r@@YAPEBDPEBDZZ
   180015b8a:	48 8b d0             	mov    rdx,rax
   180015b8d:	48 8d 4d e0          	lea    rcx,[rbp-0x20]
   180015b91:	e8 da 4a 0b 00       	call   0x1800ca670
   180015b96:	90                   	nop
   180015b97:	48 8b c8             	mov    rcx,rax
   180015b9a:	e8 21 c4 0b 00       	call   0x1800d1fc0
   180015b9f:	90                   	nop
   180015ba0:	4c 8d 05 91 9d 60 00 	lea    r8,[rip+0x609d91]        # 0x18061f938 ; 'filter_color'
   180015ba7:	49 8b 55 08          	mov    rdx,QWORD PTR [r13+0x8]
   180015bab:	48 8d 0d 96 2c 62 00 	lea    rcx,[rip+0x622c96]        # 0x180638848 ; 'Effect "%s" could not find technique "%s"'
   180015bb2:	ff 15 40 b7 5c 00    	call   QWORD PTR [rip+0x5cb740]        # 0x1805e12f8 ; ?str@r@@YAPEBDPEBDZZ
   180015bb8:	48 8b d0             	mov    rdx,rax
   180015bbb:	48 8d 4d e0          	lea    rcx,[rbp-0x20]
   180015bbf:	e8 ac 4a 0b 00       	call   0x1800ca670
   180015bc4:	90                   	nop
   180015bc5:	48 8b c8             	mov    rcx,rax
   180015bc8:	e8 f3 c3 0b 00       	call   0x1800d1fc0
   180015bcd:	cc                   	int3
