
upload/renderer_rmdwin10_f(4).dll:     file format pei-x86-64


Disassembly of section .text:

0000000180015bd0 <.text+0x14bd0>:
   180015bd0:	48 8b c4             	mov    rax,rsp
   180015bd3:	44 88 48 20          	mov    BYTE PTR [rax+0x20],r9b
   180015bd7:	44 89 40 18          	mov    DWORD PTR [rax+0x18],r8d
   180015bdb:	48 89 50 10          	mov    QWORD PTR [rax+0x10],rdx
   180015bdf:	48 89 48 08          	mov    QWORD PTR [rax+0x8],rcx
   180015be3:	55                   	push   rbp
   180015be4:	53                   	push   rbx
   180015be5:	56                   	push   rsi
   180015be6:	57                   	push   rdi
   180015be7:	41 54                	push   r12
   180015be9:	41 55                	push   r13
   180015beb:	41 56                	push   r14
   180015bed:	41 57                	push   r15
   180015bef:	48 8d a8 38 fe ff ff 	lea    rbp,[rax-0x1c8]
   180015bf6:	48 81 ec 88 02 00 00 	sub    rsp,0x288
   180015bfd:	48 c7 45 60 fe ff ff 	mov    QWORD PTR [rbp+0x60],0xfffffffffffffffe
   180015c04:	ff 
   180015c05:	0f 29 70 a8          	movaps XMMWORD PTR [rax-0x58],xmm6
   180015c09:	0f 29 78 98          	movaps XMMWORD PTR [rax-0x68],xmm7
   180015c0d:	44 0f 29 40 88       	movaps XMMWORD PTR [rax-0x78],xmm8
   180015c12:	44 0f 29 88 78 ff ff 	movaps XMMWORD PTR [rax-0x88],xmm9
   180015c19:	ff 
   180015c1a:	44 0f 29 90 68 ff ff 	movaps XMMWORD PTR [rax-0x98],xmm10
   180015c21:	ff 
   180015c22:	44 0f 29 98 58 ff ff 	movaps XMMWORD PTR [rax-0xa8],xmm11
   180015c29:	ff 
   180015c2a:	44 0f 29 a0 48 ff ff 	movaps XMMWORD PTR [rax-0xb8],xmm12
   180015c31:	ff 
   180015c32:	44 0f 29 a8 38 ff ff 	movaps XMMWORD PTR [rax-0xc8],xmm13
   180015c39:	ff 
   180015c3a:	44 0f 29 b0 28 ff ff 	movaps XMMWORD PTR [rax-0xd8],xmm14
   180015c41:	ff 
   180015c42:	44 0f 29 b8 18 ff ff 	movaps XMMWORD PTR [rax-0xe8],xmm15
   180015c49:	ff 
   180015c4a:	48 8b f1             	mov    rsi,rcx
   180015c4d:	44 8b 2d 34 88 8f 00 	mov    r13d,DWORD PTR [rip+0x8f8834]        # 0x18090e488
   180015c54:	44 8b 25 31 88 8f 00 	mov    r12d,DWORD PTR [rip+0x8f8831]        # 0x18090e48c
   180015c5b:	8b 15 db cb 7e 00    	mov    edx,DWORD PTR [rip+0x7ecbdb]        # 0x18080283c
   180015c61:	65 48 8b 04 25 58 00 	mov    rax,QWORD PTR gs:0x58
   180015c68:	00 00 
   180015c6a:	b9 40 00 00 00       	mov    ecx,0x40
   180015c6f:	4c 8b 3c d0          	mov    r15,QWORD PTR [rax+rdx*8]
   180015c73:	33 ff                	xor    edi,edi
   180015c75:	41 8b 04 0f          	mov    eax,DWORD PTR [r15+rcx*1]
   180015c79:	39 05 e9 97 27 01    	cmp    DWORD PTR [rip+0x12797e9],eax        # 0x18128f468
   180015c7f:	7e 6a                	jle    0x180015ceb
   180015c81:	48 8d 0d e0 97 27 01 	lea    rcx,[rip+0x12797e0]        # 0x18128f468
   180015c88:	e8 d7 21 57 00       	call   0x180587e64
   180015c8d:	83 3d d4 97 27 01 ff 	cmp    DWORD PTR [rip+0x12797d4],0xffffffff        # 0x18128f468
   180015c94:	75 55                	jne    0x180015ceb
   180015c96:	48 89 3d d3 97 27 01 	mov    QWORD PTR [rip+0x12797d3],rdi        # 0x18128f470
   180015c9d:	48 8d 15 7c 9b 60 00 	lea    rdx,[rip+0x609b7c]        # 0x18061f820 ; 'g_DLF_vScreenToView'
   180015ca4:	48 8d 0d cd 97 27 01 	lea    rcx,[rip+0x12797cd]        # 0x18128f478
   180015cab:	e8 c0 a5 1f 00       	call   0x180210270
   180015cb0:	c7 05 c2 97 27 01 03 	mov    DWORD PTR [rip+0x12797c2],0x3        # 0x18128f47c
   180015cb7:	00 00 00 
   180015cba:	48 8d 05 bf 97 27 01 	lea    rax,[rip+0x12797bf]        # 0x18128f480
   180015cc1:	48 89 05 a8 97 27 01 	mov    QWORD PTR [rip+0x12797a8],rax        # 0x18128f470
   180015cc8:	0f 57 c0             	xorps  xmm0,xmm0
   180015ccb:	0f 11 05 ae 97 27 01 	movups XMMWORD PTR [rip+0x12797ae],xmm0        # 0x18128f480
   180015cd2:	48 8d 0d a7 33 5b 00 	lea    rcx,[rip+0x5b33a7]        # 0x1805c9080
   180015cd9:	e8 e6 1e 57 00       	call   0x180587bc4
   180015cde:	90                   	nop
   180015cdf:	48 8d 0d 82 97 27 01 	lea    rcx,[rip+0x1279782]        # 0x18128f468
   180015ce6:	e8 19 21 57 00       	call   0x180587e04
   180015ceb:	66 41 0f 6e c5       	movd   xmm0,r13d
   180015cf0:	0f 5b c0             	cvtdq2ps xmm0,xmm0
   180015cf3:	f3 0f 11 45 f8       	movss  DWORD PTR [rbp-0x8],xmm0
   180015cf8:	66 41 0f 6e c4       	movd   xmm0,r12d
   180015cfd:	0f 5b c0             	cvtdq2ps xmm0,xmm0
   180015d00:	f3 0f 11 45 d0       	movss  DWORD PTR [rbp-0x30],xmm0
   180015d05:	48 8b 15 cc cc 7e 00 	mov    rdx,QWORD PTR [rip+0x7ecccc]        # 0x1808029d8
   180015d0c:	48 81 c2 50 01 00 00 	add    rdx,0x150
   180015d13:	48 8d 4d 00          	lea    rcx,[rbp+0x0]
   180015d17:	e8 34 2d 00 00       	call   0x180018a50
   180015d1c:	f3 0f 10 55 20       	movss  xmm2,DWORD PTR [rbp+0x20]
   180015d21:	0f 57 c9             	xorps  xmm1,xmm1
   180015d24:	f3 0f 59 d1          	mulss  xmm2,xmm1
   180015d28:	f3 0f 11 55 b0       	movss  DWORD PTR [rbp-0x50],xmm2
   180015d2d:	f3 0f 10 45 00       	movss  xmm0,DWORD PTR [rbp+0x0]
   180015d32:	f3 44 0f 10 3d 4d 92 	movss  xmm15,DWORD PTR [rip+0x66924d]        # 0x18067ef88
   180015d39:	66 00 
   180015d3b:	f3 41 0f 59 c7       	mulss  xmm0,xmm15
   180015d40:	f3 44 0f 10 6d 10    	movss  xmm13,DWORD PTR [rbp+0x10]
   180015d46:	41 0f 28 dd          	movaps xmm3,xmm13
   180015d4a:	f3 0f 5c d8          	subss  xmm3,xmm0
   180015d4e:	f3 0f 58 da          	addss  xmm3,xmm2
   180015d52:	f3 0f 58 5d 30       	addss  xmm3,DWORD PTR [rbp+0x30]
   180015d57:	f3 0f 11 5d f0       	movss  DWORD PTR [rbp-0x10],xmm3
   180015d5c:	f3 44 0f 10 75 24    	movss  xmm14,DWORD PTR [rbp+0x24]
   180015d62:	f3 44 0f 59 f1       	mulss  xmm14,xmm1
   180015d67:	f3 0f 10 45 04       	movss  xmm0,DWORD PTR [rbp+0x4]
   180015d6c:	f3 41 0f 59 c7       	mulss  xmm0,xmm15
   180015d71:	f3 44 0f 10 55 14    	movss  xmm10,DWORD PTR [rbp+0x14]
   180015d77:	41 0f 28 d2          	movaps xmm2,xmm10
   180015d7b:	f3 0f 5c d0          	subss  xmm2,xmm0
   180015d7f:	f3 41 0f 58 d6       	addss  xmm2,xmm14
   180015d84:	f3 0f 58 55 34       	addss  xmm2,DWORD PTR [rbp+0x34]
   180015d89:	f3 0f 11 95 d8 01 00 	movss  DWORD PTR [rbp+0x1d8],xmm2
   180015d90:	00 
   180015d91:	f3 44 0f 10 5d 28    	movss  xmm11,DWORD PTR [rbp+0x28]
   180015d97:	f3 44 0f 59 d9       	mulss  xmm11,xmm1
   180015d9c:	f3 0f 10 45 08       	movss  xmm0,DWORD PTR [rbp+0x8]
   180015da1:	f3 41 0f 59 c7       	mulss  xmm0,xmm15
   180015da6:	f3 0f 10 75 18       	movss  xmm6,DWORD PTR [rbp+0x18]
   180015dab:	0f 28 d6             	movaps xmm2,xmm6
   180015dae:	f3 0f 5c d0          	subss  xmm2,xmm0
   180015db2:	f3 41 0f 58 d3       	addss  xmm2,xmm11
   180015db7:	f3 0f 58 55 38       	addss  xmm2,DWORD PTR [rbp+0x38]
   180015dbc:	f3 0f 11 55 90       	movss  DWORD PTR [rbp-0x70],xmm2
   180015dc1:	f3 0f 10 7d 2c       	movss  xmm7,DWORD PTR [rbp+0x2c]
   180015dc6:	f3 0f 59 f9          	mulss  xmm7,xmm1
   180015dca:	f3 44 0f 10 45 0c    	movss  xmm8,DWORD PTR [rbp+0xc]
   180015dd0:	41 0f 28 c0          	movaps xmm0,xmm8
   180015dd4:	f3 41 0f 59 c7       	mulss  xmm0,xmm15
   180015dd9:	f3 0f 10 65 1c       	movss  xmm4,DWORD PTR [rbp+0x1c]
   180015dde:	0f 28 cc             	movaps xmm1,xmm4
   180015de1:	f3 0f 5c c8          	subss  xmm1,xmm0
   180015de5:	f3 0f 58 cf          	addss  xmm1,xmm7
   180015de9:	f3 0f 58 4d 3c       	addss  xmm1,DWORD PTR [rbp+0x3c]
   180015dee:	0f 28 c1             	movaps xmm0,xmm1
   180015df1:	0f 28 d0             	movaps xmm2,xmm0
   180015df4:	41 0f 28 c7          	movaps xmm0,xmm15
   180015df8:	44 0f 28 f8          	movaps xmm15,xmm0
   180015dfc:	0f 29 45 c0          	movaps XMMWORD PTR [rbp-0x40],xmm0
   180015e00:	f3 44 0f 5e fa       	divss  xmm15,xmm2
   180015e05:	f3 0f 10 55 90       	movss  xmm2,DWORD PTR [rbp-0x70]
   180015e0a:	f3 41 0f 59 d7       	mulss  xmm2,xmm15
   180015e0f:	f3 0f 10 85 d8 01 00 	movss  xmm0,DWORD PTR [rbp+0x1d8]
   180015e16:	00 
   180015e17:	f3 41 0f 59 c7       	mulss  xmm0,xmm15
   180015e1c:	f3 0f 11 85 d8 01 00 	movss  DWORD PTR [rbp+0x1d8],xmm0
   180015e23:	00 
   180015e24:	f3 44 0f 59 2d 5b 91 	mulss  xmm13,DWORD PTR [rip+0x66915b]        # 0x18067ef88
   180015e2b:	66 00 
   180015e2d:	f3 0f 10 45 00       	movss  xmm0,DWORD PTR [rbp+0x0]
   180015e32:	f3 41 0f 5c c5       	subss  xmm0,xmm13
   180015e37:	f3 0f 58 45 b0       	addss  xmm0,DWORD PTR [rbp-0x50]
   180015e3c:	f3 0f 58 45 30       	addss  xmm0,DWORD PTR [rbp+0x30]
   180015e41:	f3 0f 11 45 b0       	movss  DWORD PTR [rbp-0x50],xmm0
   180015e46:	f3 44 0f 10 2d 39 91 	movss  xmm13,DWORD PTR [rip+0x669139]        # 0x18067ef88
   180015e4d:	66 00 
   180015e4f:	f3 45 0f 59 d5       	mulss  xmm10,xmm13
   180015e54:	f3 44 0f 10 65 04    	movss  xmm12,DWORD PTR [rbp+0x4]
   180015e5a:	f3 45 0f 5c e2       	subss  xmm12,xmm10
   180015e5f:	f3 45 0f 58 e6       	addss  xmm12,xmm14
   180015e64:	f3 44 0f 58 65 34    	addss  xmm12,DWORD PTR [rbp+0x34]
   180015e6a:	f3 41 0f 59 f5       	mulss  xmm6,xmm13
   180015e6f:	f3 44 0f 10 4d 08    	movss  xmm9,DWORD PTR [rbp+0x8]
   180015e75:	f3 44 0f 5c ce       	subss  xmm9,xmm6
   180015e7a:	f3 45 0f 58 cb       	addss  xmm9,xmm11
   180015e7f:	f3 44 0f 58 4d 38    	addss  xmm9,DWORD PTR [rbp+0x38]
   180015e85:	f3 41 0f 59 e5       	mulss  xmm4,xmm13
   180015e8a:	f3 44 0f 5c c4       	subss  xmm8,xmm4
   180015e8f:	f3 44 0f 58 c7       	addss  xmm8,xmm7
   180015e94:	f3 44 0f 58 45 3c    	addss  xmm8,DWORD PTR [rbp+0x3c]
   180015e9a:	41 0f 28 c0          	movaps xmm0,xmm8
   180015e9e:	0f 28 c8             	movaps xmm1,xmm0
   180015ea1:	0f 28 5d c0          	movaps xmm3,XMMWORD PTR [rbp-0x40]
   180015ea5:	0f 28 c3             	movaps xmm0,xmm3
   180015ea8:	f3 0f 5e c1          	divss  xmm0,xmm1
   180015eac:	f3 44 0f 59 c8       	mulss  xmm9,xmm0
   180015eb1:	f3 44 0f 59 e0       	mulss  xmm12,xmm0
   180015eb6:	f3 0f 10 6d b0       	movss  xmm5,DWORD PTR [rbp-0x50]
   180015ebb:	f3 0f 59 e8          	mulss  xmm5,xmm0
   180015ebf:	0f 28 ca             	movaps xmm1,xmm2
   180015ec2:	0f 28 c1             	movaps xmm0,xmm1
   180015ec5:	0f 57 c9             	xorps  xmm1,xmm1
   180015ec8:	0f 28 d3             	movaps xmm2,xmm3
   180015ecb:	f3 0f 5e d0          	divss  xmm2,xmm0
   180015ecf:	f3 0f c2 c1 00       	cmpeqss xmm0,xmm1
   180015ed4:	66 0f 38 14 d1       	blendvps xmm2,xmm1,xmm0
   180015ed9:	f3 0f 10 65 f0       	movss  xmm4,DWORD PTR [rbp-0x10]
   180015ede:	f3 41 0f 59 e7       	mulss  xmm4,xmm15
   180015ee3:	f3 0f 59 e2          	mulss  xmm4,xmm2
   180015ee7:	f3 0f 59 95 d8 01 00 	mulss  xmm2,DWORD PTR [rbp+0x1d8]
   180015eee:	00 
   180015eef:	41 0f 28 c9          	movaps xmm1,xmm9
   180015ef3:	0f 28 c1             	movaps xmm0,xmm1
   180015ef6:	0f 57 c9             	xorps  xmm1,xmm1
   180015ef9:	f3 0f 5e d8          	divss  xmm3,xmm0
   180015efd:	f3 0f c2 c1 00       	cmpeqss xmm0,xmm1
   180015f02:	66 0f 38 14 d9       	blendvps xmm3,xmm1,xmm0
   180015f07:	f3 0f 59 eb          	mulss  xmm5,xmm3
   180015f0b:	f3 41 0f 59 dc       	mulss  xmm3,xmm12
   180015f10:	f3 0f 5c d3          	subss  xmm2,xmm3
   180015f14:	f3 0f 5c ec          	subss  xmm5,xmm4
   180015f18:	f3 0f 5e 6d f8       	divss  xmm5,DWORD PTR [rbp-0x8]
   180015f1d:	f3 0f 5e 55 d0       	divss  xmm2,DWORD PTR [rbp-0x30]
   180015f22:	41 0f 28 c5          	movaps xmm0,xmm13
   180015f26:	f3 0f 5c 45 f8       	subss  xmm0,DWORD PTR [rbp-0x8]
   180015f2b:	f3 0f 59 c5          	mulss  xmm0,xmm5
   180015f2f:	f3 0f 10 35 81 8f 66 	movss  xmm6,DWORD PTR [rip+0x668f81]        # 0x18067eeb8
   180015f36:	00 
   180015f37:	f3 0f 59 c6          	mulss  xmm0,xmm6
   180015f3b:	f3 44 0f 5c 6d d0    	subss  xmm13,DWORD PTR [rbp-0x30]
   180015f41:	f3 44 0f 59 ea       	mulss  xmm13,xmm2
   180015f46:	f3 44 0f 59 ee       	mulss  xmm13,xmm6
   180015f4b:	f3 0f 11 45 80       	movss  DWORD PTR [rbp-0x80],xmm0
   180015f50:	f3 0f 11 6d 84       	movss  DWORD PTR [rbp-0x7c],xmm5
   180015f55:	f3 44 0f 11 6d 88    	movss  DWORD PTR [rbp-0x78],xmm13
   180015f5b:	f3 0f 11 55 8c       	movss  DWORD PTR [rbp-0x74],xmm2
   180015f60:	45 33 c0             	xor    r8d,r8d
   180015f63:	48 8d 55 80          	lea    rdx,[rbp-0x80]
   180015f67:	8b 0d 0b 95 27 01    	mov    ecx,DWORD PTR [rip+0x127950b]        # 0x18128f478
   180015f6d:	ff 15 1d a0 5c 00    	call   QWORD PTR [rip+0x5ca01d]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   180015f73:	0f 10 45 80          	movups xmm0,XMMWORD PTR [rbp-0x80]
   180015f77:	0f 11 05 02 95 27 01 	movups XMMWORD PTR [rip+0x1279502],xmm0        # 0x18128f480
   180015f7e:	48 8b ce             	mov    rcx,rsi
   180015f81:	ff 15 b1 9d 5c 00    	call   QWORD PTR [rip+0x5c9db1]        # 0x1805dfd38 ; ?getFormat@Texture2D@d3d@@QEBA?AW4PixelFormat@2@XZ
   180015f87:	0f b6 f0             	movzx  esi,al
   180015f8a:	4c 8b 35 47 c9 7e 00 	mov    r14,QWORD PTR [rip+0x7ec947]        # 0x1808028d8
   180015f91:	0f 57 c0             	xorps  xmm0,xmm0
   180015f94:	0f 29 45 80          	movaps XMMWORD PTR [rbp-0x80],xmm0
   180015f98:	c7 44 24 70 ff ff ff 	mov    DWORD PTR [rsp+0x70],0xffffffff
   180015f9f:	ff 
   180015fa0:	48 8d 45 80          	lea    rax,[rbp-0x80]
   180015fa4:	48 89 44 24 68       	mov    QWORD PTR [rsp+0x68],rax
   180015fa9:	c7 44 24 60 01 00 00 	mov    DWORD PTR [rsp+0x60],0x1
   180015fb0:	00 
   180015fb1:	f3 0f 11 74 24 58    	movss  DWORD PTR [rsp+0x58],xmm6
   180015fb7:	89 7c 24 50          	mov    DWORD PTR [rsp+0x50],edi
   180015fbb:	89 7c 24 48          	mov    DWORD PTR [rsp+0x48],edi
   180015fbf:	48 89 7c 24 40       	mov    QWORD PTR [rsp+0x40],rdi
   180015fc4:	48 89 7c 24 38       	mov    QWORD PTR [rsp+0x38],rdi
   180015fc9:	89 7c 24 30          	mov    DWORD PTR [rsp+0x30],edi
   180015fcd:	c7 44 24 28 05 00 00 	mov    DWORD PTR [rsp+0x28],0x5
   180015fd4:	00 
   180015fd5:	40 88 74 24 20       	mov    BYTE PTR [rsp+0x20],sil
   180015fda:	41 b9 01 00 00 00    	mov    r9d,0x1
   180015fe0:	45 8b c4             	mov    r8d,r12d
   180015fe3:	41 8b d5             	mov    edx,r13d
   180015fe6:	48 8d 8d 80 00 00 00 	lea    rcx,[rbp+0x80]
   180015fed:	ff 15 35 9d 5c 00    	call   QWORD PTR [rip+0x5c9d35]        # 0x1805dfd28 ; ??0Texture2DInit@d3d@@QEAA@HHHW4PixelFormat@1@HHPEBX_KW4MiscFlags@1@HMHV?$Vector4Template@M@m@@W4TileMode@1@@Z
   180015ff3:	4c 8d 05 4e 99 60 00 	lea    r8,[rip+0x60994e]        # 0x18061f948 ; 'rend::DeferredLightFiltering::filterSpecularNoise - tmp'
   180015ffa:	48 8d 95 80 00 00 00 	lea    rdx,[rbp+0x80]
   180016001:	49 8b ce             	mov    rcx,r14
   180016004:	e8 57 37 09 00       	call   0x1800a9760
   180016009:	48 8b d8             	mov    rbx,rax
   18001600c:	48 89 45 f8          	mov    QWORD PTR [rbp-0x8],rax
   180016010:	48 8b 0d 01 cb 8f 00 	mov    rcx,QWORD PTR [rip+0x8fcb01]        # 0x180912b18
   180016017:	48 85 c9             	test   rcx,rcx
   18001601a:	74 3b                	je     0x180016057
   18001601c:	8b 3d 66 84 8f 00    	mov    edi,DWORD PTR [rip+0x8f8466]        # 0x18090e488
   180016022:	ff 15 c0 9c 5c 00    	call   QWORD PTR [rip+0x5c9cc0]        # 0x1805dfce8 ; ?getWidth@Texture2D@d3d@@QEBAHXZ
   180016028:	3b c7                	cmp    eax,edi
   18001602a:	75 17                	jne    0x180016043
   18001602c:	8b 3d 5a 84 8f 00    	mov    edi,DWORD PTR [rip+0x8f845a]        # 0x18090e48c
   180016032:	48 8b 0d df ca 8f 00 	mov    rcx,QWORD PTR [rip+0x8fcadf]        # 0x180912b18
   180016039:	ff 15 a1 9c 5c 00    	call   QWORD PTR [rip+0x5c9ca1]        # 0x1805dfce0 ; ?getHeight@Texture2D@d3d@@QEBAHXZ
   18001603f:	3b c7                	cmp    eax,edi
   180016041:	74 05                	je     0x180016048
   180016043:	e8 d8 dd ff ff       	call   0x180013e20
   180016048:	33 ff                	xor    edi,edi
   18001604a:	48 39 3d c7 ca 8f 00 	cmp    QWORD PTR [rip+0x8fcac7],rdi        # 0x180912b18
   180016051:	0f 85 72 01 00 00    	jne    0x1800161c9
   180016057:	0f 57 c0             	xorps  xmm0,xmm0
   18001605a:	0f 29 45 80          	movaps XMMWORD PTR [rbp-0x80],xmm0
   18001605e:	c7 44 24 70 ff ff ff 	mov    DWORD PTR [rsp+0x70],0xffffffff
   180016065:	ff 
   180016066:	48 8d 45 80          	lea    rax,[rbp-0x80]
   18001606a:	48 89 44 24 68       	mov    QWORD PTR [rsp+0x68],rax
   18001606f:	c7 44 24 60 01 00 00 	mov    DWORD PTR [rsp+0x60],0x1
   180016076:	00 
   180016077:	f3 0f 11 74 24 58    	movss  DWORD PTR [rsp+0x58],xmm6
   18001607d:	89 7c 24 50          	mov    DWORD PTR [rsp+0x50],edi
   180016081:	89 7c 24 48          	mov    DWORD PTR [rsp+0x48],edi
   180016085:	48 89 7c 24 40       	mov    QWORD PTR [rsp+0x40],rdi
   18001608a:	48 89 7c 24 38       	mov    QWORD PTR [rsp+0x38],rdi
   18001608f:	89 7c 24 30          	mov    DWORD PTR [rsp+0x30],edi
   180016093:	c7 44 24 28 05 00 00 	mov    DWORD PTR [rsp+0x28],0x5
   18001609a:	00 
   18001609b:	40 88 74 24 20       	mov    BYTE PTR [rsp+0x20],sil
   1800160a0:	41 b9 01 00 00 00    	mov    r9d,0x1
   1800160a6:	44 8b 05 df 83 8f 00 	mov    r8d,DWORD PTR [rip+0x8f83df]        # 0x18090e48c
   1800160ad:	8b 15 d5 83 8f 00    	mov    edx,DWORD PTR [rip+0x8f83d5]        # 0x18090e488
   1800160b3:	48 8d 4d 00          	lea    rcx,[rbp+0x0]
   1800160b7:	ff 15 6b 9c 5c 00    	call   QWORD PTR [rip+0x5c9c6b]        # 0x1805dfd28 ; ??0Texture2DInit@d3d@@QEAA@HHHW4PixelFormat@1@HHPEBX_KW4MiscFlags@1@HMHV?$Vector4Template@M@m@@W4TileMode@1@@Z
   1800160bd:	48 8b d0             	mov    rdx,rax
   1800160c0:	4c 8d 05 a9 97 60 00 	lea    r8,[rip+0x6097a9]        # 0x18061f870 ; 'm_pColorHistory'
   1800160c7:	49 8b ce             	mov    rcx,r14
   1800160ca:	e8 91 36 09 00       	call   0x1800a9760
   1800160cf:	48 8b 15 42 ca 8f 00 	mov    rdx,QWORD PTR [rip+0x8fca42]        # 0x180912b18
   1800160d6:	48 89 05 3b ca 8f 00 	mov    QWORD PTR [rip+0x8fca3b],rax        # 0x180912b18
   1800160dd:	48 85 d2             	test   rdx,rdx
   1800160e0:	74 0d                	je     0x1800160ef
   1800160e2:	48 8b 0d ef c7 7e 00 	mov    rcx,QWORD PTR [rip+0x7ec7ef]        # 0x1808028d8
   1800160e9:	e8 12 3d 09 00       	call   0x1800a9e00
   1800160ee:	90                   	nop
   1800160ef:	0f 57 c0             	xorps  xmm0,xmm0
   1800160f2:	0f 29 45 80          	movaps XMMWORD PTR [rbp-0x80],xmm0
   1800160f6:	c7 44 24 70 ff ff ff 	mov    DWORD PTR [rsp+0x70],0xffffffff
   1800160fd:	ff 
   1800160fe:	48 8d 45 80          	lea    rax,[rbp-0x80]
   180016102:	48 89 44 24 68       	mov    QWORD PTR [rsp+0x68],rax
   180016107:	c7 44 24 60 01 00 00 	mov    DWORD PTR [rsp+0x60],0x1
   18001610e:	00 
   18001610f:	f3 0f 11 74 24 58    	movss  DWORD PTR [rsp+0x58],xmm6
   180016115:	89 7c 24 50          	mov    DWORD PTR [rsp+0x50],edi
   180016119:	89 7c 24 48          	mov    DWORD PTR [rsp+0x48],edi
   18001611d:	48 89 7c 24 40       	mov    QWORD PTR [rsp+0x40],rdi
   180016122:	48 89 7c 24 38       	mov    QWORD PTR [rsp+0x38],rdi
   180016127:	89 7c 24 30          	mov    DWORD PTR [rsp+0x30],edi
   18001612b:	c7 44 24 28 05 00 00 	mov    DWORD PTR [rsp+0x28],0x5
   180016132:	00 
   180016133:	40 88 74 24 20       	mov    BYTE PTR [rsp+0x20],sil
   180016138:	41 b9 01 00 00 00    	mov    r9d,0x1
   18001613e:	44 8b 05 47 83 8f 00 	mov    r8d,DWORD PTR [rip+0x8f8347]        # 0x18090e48c
   180016145:	8b 15 3d 83 8f 00    	mov    edx,DWORD PTR [rip+0x8f833d]        # 0x18090e488
   18001614b:	48 8d 4d 00          	lea    rcx,[rbp+0x0]
   18001614f:	ff 15 d3 9b 5c 00    	call   QWORD PTR [rip+0x5c9bd3]        # 0x1805dfd28 ; ??0Texture2DInit@d3d@@QEAA@HHHW4PixelFormat@1@HHPEBX_KW4MiscFlags@1@HMHV?$Vector4Template@M@m@@W4TileMode@1@@Z
   180016155:	48 8b d0             	mov    rdx,rax
   180016158:	4c 8d 05 11 97 60 00 	lea    r8,[rip+0x609711]        # 0x18061f870 ; 'm_pColorHistory'
   18001615f:	49 8b ce             	mov    rcx,r14
   180016162:	e8 f9 35 09 00       	call   0x1800a9760
   180016167:	48 8b 15 b2 c9 8f 00 	mov    rdx,QWORD PTR [rip+0x8fc9b2]        # 0x180912b20
   18001616e:	48 89 05 ab c9 8f 00 	mov    QWORD PTR [rip+0x8fc9ab],rax        # 0x180912b20
   180016175:	48 85 d2             	test   rdx,rdx
   180016178:	74 0d                	je     0x180016187
   18001617a:	48 8b 0d 57 c7 7e 00 	mov    rcx,QWORD PTR [rip+0x7ec757]        # 0x1808028d8
   180016181:	e8 7a 3c 09 00       	call   0x1800a9e00
   180016186:	90                   	nop
   180016187:	0f 57 c0             	xorps  xmm0,xmm0
   18001618a:	0f 29 45 80          	movaps XMMWORD PTR [rbp-0x80],xmm0
   18001618e:	48 8b 0d 83 c9 8f 00 	mov    rcx,QWORD PTR [rip+0x8fc983]        # 0x180912b18
   180016195:	ff 15 95 9b 5c 00    	call   QWORD PTR [rip+0x5c9b95]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   18001619b:	48 8b c8             	mov    rcx,rax
   18001619e:	48 8d 55 80          	lea    rdx,[rbp-0x80]
   1800161a2:	ff 15 98 9b 5c 00    	call   QWORD PTR [rip+0x5c9b98]        # 0x1805dfd40 ; ?clearUAV@DeviceUtil@d3d@@SAXPEAVNativeTexture@2@V?$Vector4Template@M@m@@@Z
   1800161a8:	0f 57 c0             	xorps  xmm0,xmm0
   1800161ab:	0f 29 45 80          	movaps XMMWORD PTR [rbp-0x80],xmm0
   1800161af:	48 8b 0d 6a c9 8f 00 	mov    rcx,QWORD PTR [rip+0x8fc96a]        # 0x180912b20
   1800161b6:	ff 15 74 9b 5c 00    	call   QWORD PTR [rip+0x5c9b74]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   1800161bc:	48 8b c8             	mov    rcx,rax
   1800161bf:	48 8d 55 80          	lea    rdx,[rbp-0x80]
   1800161c3:	ff 15 77 9b 5c 00    	call   QWORD PTR [rip+0x5c9b77]        # 0x1805dfd40 ; ?clearUAV@DeviceUtil@d3d@@SAXPEAVNativeTexture@2@V?$Vector4Template@M@m@@@Z
   1800161c9:	4c 8d 35 d8 92 27 01 	lea    r14,[rip+0x12792d8]        # 0x18128f4a8
   1800161d0:	be 40 00 00 00       	mov    esi,0x40
   1800161d5:	41 8b 04 37          	mov    eax,DWORD PTR [r15+rsi*1]
   1800161d9:	39 05 b1 92 27 01    	cmp    DWORD PTR [rip+0x12792b1],eax        # 0x18128f490
   1800161df:	7e 63                	jle    0x180016244
   1800161e1:	48 8d 0d a8 92 27 01 	lea    rcx,[rip+0x12792a8]        # 0x18128f490
   1800161e8:	e8 77 1c 57 00       	call   0x180587e64
   1800161ed:	83 3d 9c 92 27 01 ff 	cmp    DWORD PTR [rip+0x127929c],0xffffffff        # 0x18128f490
   1800161f4:	75 4e                	jne    0x180016244
   1800161f6:	48 89 3d 9b 92 27 01 	mov    QWORD PTR [rip+0x127929b],rdi        # 0x18128f498
   1800161fd:	48 8d 15 7c 96 60 00 	lea    rdx,[rip+0x60967c]        # 0x18061f880 ; 'g_DLF_tOutputHistory'
   180016204:	48 8d 0d 95 92 27 01 	lea    rcx,[rip+0x1279295]        # 0x18128f4a0
   18001620b:	e8 60 a0 1f 00       	call   0x180210270
   180016210:	c7 05 8a 92 27 01 09 	mov    DWORD PTR [rip+0x127928a],0x9        # 0x18128f4a4
   180016217:	00 00 00 
   18001621a:	4c 89 35 77 92 27 01 	mov    QWORD PTR [rip+0x1279277],r14        # 0x18128f498
   180016221:	49 8b ce             	mov    rcx,r14
   180016224:	ff 15 46 9a 5c 00    	call   QWORD PTR [rip+0x5c9a46]        # 0x1805dfc70 ; ??0ShaderTexture@d3d@@QEAA@XZ
   18001622a:	90                   	nop
   18001622b:	48 8d 0d 1e 2e 5b 00 	lea    rcx,[rip+0x5b2e1e]        # 0x1805c9050
   180016232:	e8 8d 19 57 00       	call   0x180587bc4
   180016237:	90                   	nop
   180016238:	48 8d 0d 51 92 27 01 	lea    rcx,[rip+0x1279251]        # 0x18128f490
   18001623f:	e8 c0 1b 57 00       	call   0x180587e04
   180016244:	48 8b 0d d5 c8 8f 00 	mov    rcx,QWORD PTR [rip+0x8fc8d5]        # 0x180912b20
   18001624b:	48 85 c9             	test   rcx,rcx
   18001624e:	74 09                	je     0x180016259
   180016250:	ff 15 da 9a 5c 00    	call   QWORD PTR [rip+0x5c9ada]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   180016256:	48 8b f8             	mov    rdi,rax
   180016259:	8b 0d 41 92 27 01    	mov    ecx,DWORD PTR [rip+0x1279241]        # 0x18128f4a0
   18001625f:	ff 15 23 9a 5c 00    	call   QWORD PTR [rip+0x5c9a23]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   180016265:	8b d0                	mov    edx,eax
   180016267:	4c 8b c7             	mov    r8,rdi
   18001626a:	48 8d 4d 80          	lea    rcx,[rbp-0x80]
   18001626e:	ff 15 0c 9a 5c 00    	call   QWORD PTR [rip+0x5c9a0c]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   180016274:	45 33 c0             	xor    r8d,r8d
   180016277:	48 8d 55 80          	lea    rdx,[rbp-0x80]
   18001627b:	8b 0d 1f 92 27 01    	mov    ecx,DWORD PTR [rip+0x127921f]        # 0x18128f4a0
   180016281:	ff 15 09 9d 5c 00    	call   QWORD PTR [rip+0x5c9d09]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   180016287:	8b 0d 13 92 27 01    	mov    ecx,DWORD PTR [rip+0x1279213]        # 0x18128f4a0
   18001628d:	ff 15 f5 99 5c 00    	call   QWORD PTR [rip+0x5c99f5]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   180016293:	8b d0                	mov    edx,eax
   180016295:	4c 8b c7             	mov    r8,rdi
   180016298:	49 8b ce             	mov    rcx,r14
   18001629b:	ff 15 ef 99 5c 00    	call   QWORD PTR [rip+0x5c99ef]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   1800162a1:	4c 8d 35 28 92 27 01 	lea    r14,[rip+0x1279228]        # 0x18128f4d0
   1800162a8:	41 8b 04 37          	mov    eax,DWORD PTR [r15+rsi*1]
   1800162ac:	39 05 06 92 27 01    	cmp    DWORD PTR [rip+0x1279206],eax        # 0x18128f4b8
   1800162b2:	7e 67                	jle    0x18001631b
   1800162b4:	48 8d 0d fd 91 27 01 	lea    rcx,[rip+0x12791fd]        # 0x18128f4b8
   1800162bb:	e8 a4 1b 57 00       	call   0x180587e64
   1800162c0:	83 3d f1 91 27 01 ff 	cmp    DWORD PTR [rip+0x12791f1],0xffffffff        # 0x18128f4b8
   1800162c7:	75 52                	jne    0x18001631b
   1800162c9:	48 c7 05 ec 91 27 01 	mov    QWORD PTR [rip+0x12791ec],0x0        # 0x18128f4c0
   1800162d0:	00 00 00 00 
   1800162d4:	48 8d 15 a5 96 60 00 	lea    rdx,[rip+0x6096a5]        # 0x18061f980 ; 'g_DLF_tInputHistory'
   1800162db:	48 8d 0d e6 91 27 01 	lea    rcx,[rip+0x12791e6]        # 0x18128f4c8
   1800162e2:	e8 89 9f 1f 00       	call   0x180210270
   1800162e7:	c7 05 db 91 27 01 09 	mov    DWORD PTR [rip+0x12791db],0x9        # 0x18128f4cc
   1800162ee:	00 00 00 
   1800162f1:	4c 89 35 c8 91 27 01 	mov    QWORD PTR [rip+0x12791c8],r14        # 0x18128f4c0
   1800162f8:	49 8b ce             	mov    rcx,r14
   1800162fb:	ff 15 6f 99 5c 00    	call   QWORD PTR [rip+0x5c996f]        # 0x1805dfc70 ; ??0ShaderTexture@d3d@@QEAA@XZ
   180016301:	90                   	nop
   180016302:	48 8d 0d 17 2d 5b 00 	lea    rcx,[rip+0x5b2d17]        # 0x1805c9020
   180016309:	e8 b6 18 57 00       	call   0x180587bc4
   18001630e:	90                   	nop
   18001630f:	48 8d 0d a2 91 27 01 	lea    rcx,[rip+0x12791a2]        # 0x18128f4b8
   180016316:	e8 e9 1a 57 00       	call   0x180587e04
   18001631b:	48 8b 0d f6 c7 8f 00 	mov    rcx,QWORD PTR [rip+0x8fc7f6]        # 0x180912b18
   180016322:	48 85 c9             	test   rcx,rcx
   180016325:	74 0b                	je     0x180016332
   180016327:	ff 15 03 9a 5c 00    	call   QWORD PTR [rip+0x5c9a03]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   18001632d:	48 8b f8             	mov    rdi,rax
   180016330:	eb 02                	jmp    0x180016334
   180016332:	33 ff                	xor    edi,edi
   180016334:	8b 0d 8e 91 27 01    	mov    ecx,DWORD PTR [rip+0x127918e]        # 0x18128f4c8
   18001633a:	ff 15 48 99 5c 00    	call   QWORD PTR [rip+0x5c9948]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   180016340:	8b d0                	mov    edx,eax
   180016342:	4c 8b c7             	mov    r8,rdi
   180016345:	48 8d 4d 80          	lea    rcx,[rbp-0x80]
   180016349:	ff 15 31 99 5c 00    	call   QWORD PTR [rip+0x5c9931]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   18001634f:	45 33 c0             	xor    r8d,r8d
   180016352:	48 8d 55 80          	lea    rdx,[rbp-0x80]
   180016356:	8b 0d 6c 91 27 01    	mov    ecx,DWORD PTR [rip+0x127916c]        # 0x18128f4c8
   18001635c:	ff 15 2e 9c 5c 00    	call   QWORD PTR [rip+0x5c9c2e]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   180016362:	8b 0d 60 91 27 01    	mov    ecx,DWORD PTR [rip+0x1279160]        # 0x18128f4c8
   180016368:	ff 15 1a 99 5c 00    	call   QWORD PTR [rip+0x5c991a]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   18001636e:	8b d0                	mov    edx,eax
   180016370:	4c 8b c7             	mov    r8,rdi
   180016373:	49 8b ce             	mov    rcx,r14
   180016376:	ff 15 14 99 5c 00    	call   QWORD PTR [rip+0x5c9914]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   18001637c:	41 8b 04 37          	mov    eax,DWORD PTR [r15+rsi*1]
   180016380:	39 05 5a 91 27 01    	cmp    DWORD PTR [rip+0x127915a],eax        # 0x18128f4e0
   180016386:	7e 64                	jle    0x1800163ec
   180016388:	48 8d 0d 51 91 27 01 	lea    rcx,[rip+0x1279151]        # 0x18128f4e0
   18001638f:	e8 d0 1a 57 00       	call   0x180587e64
   180016394:	83 3d 45 91 27 01 ff 	cmp    DWORD PTR [rip+0x1279145],0xffffffff        # 0x18128f4e0
   18001639b:	75 4f                	jne    0x1800163ec
   18001639d:	48 c7 05 40 91 27 01 	mov    QWORD PTR [rip+0x1279140],0x0        # 0x18128f4e8
   1800163a4:	00 00 00 00 
   1800163a8:	48 8d 15 e9 94 60 00 	lea    rdx,[rip+0x6094e9]        # 0x18061f898 ; 'g_DLF_iFilterPassCount'
   1800163af:	48 8d 0d 3a 91 27 01 	lea    rcx,[rip+0x127913a]        # 0x18128f4f0
   1800163b6:	e8 b5 9e 1f 00       	call   0x180210270
   1800163bb:	c7 05 2f 91 27 01 10 	mov    DWORD PTR [rip+0x127912f],0x10        # 0x18128f4f4
   1800163c2:	00 00 00 
   1800163c5:	48 8d 05 2c 91 27 01 	lea    rax,[rip+0x127912c]        # 0x18128f4f8
   1800163cc:	48 89 05 15 91 27 01 	mov    QWORD PTR [rip+0x1279115],rax        # 0x18128f4e8
   1800163d3:	48 8d 0d 16 2c 5b 00 	lea    rcx,[rip+0x5b2c16]        # 0x1805c8ff0
   1800163da:	e8 e5 17 57 00       	call   0x180587bc4
   1800163df:	90                   	nop
   1800163e0:	48 8d 0d f9 90 27 01 	lea    rcx,[rip+0x12790f9]        # 0x18128f4e0
   1800163e7:	e8 18 1a 57 00       	call   0x180587e04
   1800163ec:	8b 85 e0 01 00 00    	mov    eax,DWORD PTR [rbp+0x1e0]
   1800163f2:	89 85 d8 01 00 00    	mov    DWORD PTR [rbp+0x1d8],eax
   1800163f8:	45 33 c0             	xor    r8d,r8d
   1800163fb:	48 8d 95 d8 01 00 00 	lea    rdx,[rbp+0x1d8]
   180016402:	8b 0d e8 90 27 01    	mov    ecx,DWORD PTR [rip+0x12790e8]        # 0x18128f4f0
   180016408:	ff 15 82 9b 5c 00    	call   QWORD PTR [rip+0x5c9b82]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   18001640e:	8b 85 d8 01 00 00    	mov    eax,DWORD PTR [rbp+0x1d8]
   180016414:	89 05 de 90 27 01    	mov    DWORD PTR [rip+0x12790de],eax        # 0x18128f4f8
   18001641a:	ff 15 90 98 5c 00    	call   QWORD PTR [rip+0x5c9890]        # 0x1805dfcb0 ; ?retrieveForThread@DeviceState@d3d@@SAAEAV12@XZ
   180016420:	4c 8b f0             	mov    r14,rax
   180016423:	48 89 45 d0          	mov    QWORD PTR [rbp-0x30],rax
   180016427:	48 8b b5 d0 01 00 00 	mov    rsi,QWORD PTR [rbp+0x1d0]
   18001642e:	48 8b d6             	mov    rdx,rsi
   180016431:	48 8b 0d b8 c8 8e 00 	mov    rcx,QWORD PTR [rip+0x8ec8b8]        # 0x180902cf0
   180016438:	e8 03 07 1d 00       	call   0x1801e6b40
   18001643d:	48 8d 3d d4 90 27 01 	lea    rdi,[rip+0x12790d4]        # 0x18128f518
   180016444:	b8 40 00 00 00       	mov    eax,0x40
   180016449:	41 8b 0c 07          	mov    ecx,DWORD PTR [r15+rax*1]
   18001644d:	39 0d ad 90 27 01    	cmp    DWORD PTR [rip+0x12790ad],ecx        # 0x18128f500
   180016453:	7e 67                	jle    0x1800164bc
   180016455:	48 8d 0d a4 90 27 01 	lea    rcx,[rip+0x12790a4]        # 0x18128f500
   18001645c:	e8 03 1a 57 00       	call   0x180587e64
   180016461:	83 3d 98 90 27 01 ff 	cmp    DWORD PTR [rip+0x1279098],0xffffffff        # 0x18128f500
   180016468:	75 52                	jne    0x1800164bc
   18001646a:	48 c7 05 93 90 27 01 	mov    QWORD PTR [rip+0x1279093],0x0        # 0x18128f508
   180016471:	00 00 00 00 
   180016475:	48 8d 15 34 94 60 00 	lea    rdx,[rip+0x609434]        # 0x18061f8b0 ; 'g_DLF_rwtColorTarget'
   18001647c:	48 8d 0d 8d 90 27 01 	lea    rcx,[rip+0x127908d]        # 0x18128f510
   180016483:	e8 e8 9d 1f 00       	call   0x180210270
   180016488:	c7 05 82 90 27 01 09 	mov    DWORD PTR [rip+0x1279082],0x9        # 0x18128f514
   18001648f:	00 00 00 
   180016492:	48 89 3d 6f 90 27 01 	mov    QWORD PTR [rip+0x127906f],rdi        # 0x18128f508
   180016499:	48 8b cf             	mov    rcx,rdi
   18001649c:	ff 15 ce 97 5c 00    	call   QWORD PTR [rip+0x5c97ce]        # 0x1805dfc70 ; ??0ShaderTexture@d3d@@QEAA@XZ
   1800164a2:	90                   	nop
   1800164a3:	48 8d 0d 16 2b 5b 00 	lea    rcx,[rip+0x5b2b16]        # 0x1805c8fc0
   1800164aa:	e8 15 17 57 00       	call   0x180587bc4
   1800164af:	90                   	nop
   1800164b0:	48 8d 0d 49 90 27 01 	lea    rcx,[rip+0x1279049]        # 0x18128f500
   1800164b7:	e8 48 19 57 00       	call   0x180587e04
   1800164bc:	48 85 db             	test   rbx,rbx
   1800164bf:	74 0e                	je     0x1800164cf
   1800164c1:	48 8b cb             	mov    rcx,rbx
   1800164c4:	ff 15 66 98 5c 00    	call   QWORD PTR [rip+0x5c9866]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   1800164ca:	48 8b f8             	mov    rdi,rax
   1800164cd:	eb 02                	jmp    0x1800164d1
   1800164cf:	33 ff                	xor    edi,edi
   1800164d1:	8b 0d 39 90 27 01    	mov    ecx,DWORD PTR [rip+0x1279039]        # 0x18128f510
   1800164d7:	ff 15 ab 97 5c 00    	call   QWORD PTR [rip+0x5c97ab]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   1800164dd:	8b d0                	mov    edx,eax
   1800164df:	4c 8b c7             	mov    r8,rdi
   1800164e2:	48 8d 4d 80          	lea    rcx,[rbp-0x80]
   1800164e6:	ff 15 94 97 5c 00    	call   QWORD PTR [rip+0x5c9794]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   1800164ec:	45 33 c0             	xor    r8d,r8d
   1800164ef:	48 8d 55 80          	lea    rdx,[rbp-0x80]
   1800164f3:	8b 0d 17 90 27 01    	mov    ecx,DWORD PTR [rip+0x1279017]        # 0x18128f510
   1800164f9:	ff 15 91 9a 5c 00    	call   QWORD PTR [rip+0x5c9a91]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   1800164ff:	8b 0d 0b 90 27 01    	mov    ecx,DWORD PTR [rip+0x127900b]        # 0x18128f510
   180016505:	ff 15 7d 97 5c 00    	call   QWORD PTR [rip+0x5c977d]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   18001650b:	8b d0                	mov    edx,eax
   18001650d:	4c 8b c7             	mov    r8,rdi
   180016510:	48 8d 0d 01 90 27 01 	lea    rcx,[rip+0x1279001]        # 0x18128f518
   180016517:	ff 15 73 97 5c 00    	call   QWORD PTR [rip+0x5c9773]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   18001651d:	48 8d 3d 1c 90 27 01 	lea    rdi,[rip+0x127901c]        # 0x18128f540
   180016524:	b8 40 00 00 00       	mov    eax,0x40
   180016529:	41 8b 04 07          	mov    eax,DWORD PTR [r15+rax*1]
   18001652d:	39 05 f5 8f 27 01    	cmp    DWORD PTR [rip+0x1278ff5],eax        # 0x18128f528
   180016533:	7e 67                	jle    0x18001659c
   180016535:	48 8d 0d ec 8f 27 01 	lea    rcx,[rip+0x1278fec]        # 0x18128f528
   18001653c:	e8 23 19 57 00       	call   0x180587e64
   180016541:	83 3d e0 8f 27 01 ff 	cmp    DWORD PTR [rip+0x1278fe0],0xffffffff        # 0x18128f528
   180016548:	75 52                	jne    0x18001659c
   18001654a:	48 c7 05 db 8f 27 01 	mov    QWORD PTR [rip+0x1278fdb],0x0        # 0x18128f530
   180016551:	00 00 00 00 
   180016555:	48 8d 15 6c 93 60 00 	lea    rdx,[rip+0x60936c]        # 0x18061f8c8 ; 'g_DLF_rwtColorSource'
   18001655c:	48 8d 0d d5 8f 27 01 	lea    rcx,[rip+0x1278fd5]        # 0x18128f538
   180016563:	e8 08 9d 1f 00       	call   0x180210270
   180016568:	c7 05 ca 8f 27 01 09 	mov    DWORD PTR [rip+0x1278fca],0x9        # 0x18128f53c
   18001656f:	00 00 00 
   180016572:	48 89 3d b7 8f 27 01 	mov    QWORD PTR [rip+0x1278fb7],rdi        # 0x18128f530
   180016579:	48 8b cf             	mov    rcx,rdi
   18001657c:	ff 15 ee 96 5c 00    	call   QWORD PTR [rip+0x5c96ee]        # 0x1805dfc70 ; ??0ShaderTexture@d3d@@QEAA@XZ
   180016582:	90                   	nop
   180016583:	48 8d 0d 06 2a 5b 00 	lea    rcx,[rip+0x5b2a06]        # 0x1805c8f90
   18001658a:	e8 35 16 57 00       	call   0x180587bc4
   18001658f:	90                   	nop
   180016590:	48 8d 0d 91 8f 27 01 	lea    rcx,[rip+0x1278f91]        # 0x18128f528
   180016597:	e8 68 18 57 00       	call   0x180587e04
   18001659c:	48 85 f6             	test   rsi,rsi
   18001659f:	74 0e                	je     0x1800165af
   1800165a1:	48 8b ce             	mov    rcx,rsi
   1800165a4:	ff 15 86 97 5c 00    	call   QWORD PTR [rip+0x5c9786]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   1800165aa:	48 8b f8             	mov    rdi,rax
   1800165ad:	eb 02                	jmp    0x1800165b1
   1800165af:	33 ff                	xor    edi,edi
   1800165b1:	8b 0d 81 8f 27 01    	mov    ecx,DWORD PTR [rip+0x1278f81]        # 0x18128f538
   1800165b7:	ff 15 cb 96 5c 00    	call   QWORD PTR [rip+0x5c96cb]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   1800165bd:	8b d0                	mov    edx,eax
   1800165bf:	4c 8b c7             	mov    r8,rdi
   1800165c2:	48 8d 4d 80          	lea    rcx,[rbp-0x80]
   1800165c6:	ff 15 b4 96 5c 00    	call   QWORD PTR [rip+0x5c96b4]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   1800165cc:	45 33 c0             	xor    r8d,r8d
   1800165cf:	48 8d 55 80          	lea    rdx,[rbp-0x80]
   1800165d3:	8b 0d 5f 8f 27 01    	mov    ecx,DWORD PTR [rip+0x1278f5f]        # 0x18128f538
   1800165d9:	ff 15 b1 99 5c 00    	call   QWORD PTR [rip+0x5c99b1]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   1800165df:	8b 0d 53 8f 27 01    	mov    ecx,DWORD PTR [rip+0x1278f53]        # 0x18128f538
   1800165e5:	ff 15 9d 96 5c 00    	call   QWORD PTR [rip+0x5c969d]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   1800165eb:	8b d0                	mov    edx,eax
   1800165ed:	4c 8b c7             	mov    r8,rdi
   1800165f0:	48 8d 0d 49 8f 27 01 	lea    rcx,[rip+0x1278f49]        # 0x18128f540
   1800165f7:	ff 15 93 96 5c 00    	call   QWORD PTR [rip+0x5c9693]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   1800165fd:	b8 40 00 00 00       	mov    eax,0x40
   180016602:	41 8b 04 07          	mov    eax,DWORD PTR [r15+rax*1]
   180016606:	39 05 44 8f 27 01    	cmp    DWORD PTR [rip+0x1278f44],eax        # 0x18128f550
   18001660c:	7e 64                	jle    0x180016672
   18001660e:	48 8d 0d 3b 8f 27 01 	lea    rcx,[rip+0x1278f3b]        # 0x18128f550
   180016615:	e8 4a 18 57 00       	call   0x180587e64
   18001661a:	83 3d 2f 8f 27 01 ff 	cmp    DWORD PTR [rip+0x1278f2f],0xffffffff        # 0x18128f550
   180016621:	75 4f                	jne    0x180016672
   180016623:	33 ff                	xor    edi,edi
   180016625:	48 89 3d 2c 8f 27 01 	mov    QWORD PTR [rip+0x1278f2c],rdi        # 0x18128f558
   18001662c:	48 8d 15 ad 92 60 00 	lea    rdx,[rip+0x6092ad]        # 0x18061f8e0 ; 'g_DLF_iFilterPass'
   180016633:	48 8d 0d 26 8f 27 01 	lea    rcx,[rip+0x1278f26]        # 0x18128f560
   18001663a:	e8 31 9c 1f 00       	call   0x180210270
   18001663f:	c7 05 1b 8f 27 01 10 	mov    DWORD PTR [rip+0x1278f1b],0x10        # 0x18128f564
   180016646:	00 00 00 
   180016649:	48 8d 05 18 8f 27 01 	lea    rax,[rip+0x1278f18]        # 0x18128f568
   180016650:	48 89 05 01 8f 27 01 	mov    QWORD PTR [rip+0x1278f01],rax        # 0x18128f558
   180016657:	48 8d 0d 02 29 5b 00 	lea    rcx,[rip+0x5b2902]        # 0x1805c8f60
   18001665e:	e8 61 15 57 00       	call   0x180587bc4
   180016663:	90                   	nop
   180016664:	48 8d 0d e5 8e 27 01 	lea    rcx,[rip+0x1278ee5]        # 0x18128f550
   18001666b:	e8 94 17 57 00       	call   0x180587e04
   180016670:	eb 02                	jmp    0x180016674
   180016672:	33 ff                	xor    edi,edi
   180016674:	89 bd d8 01 00 00    	mov    DWORD PTR [rbp+0x1d8],edi
   18001667a:	45 33 c0             	xor    r8d,r8d
   18001667d:	48 8d 95 d8 01 00 00 	lea    rdx,[rbp+0x1d8]
   180016684:	8b 0d d6 8e 27 01    	mov    ecx,DWORD PTR [rip+0x1278ed6]        # 0x18128f560
   18001668a:	ff 15 00 99 5c 00    	call   QWORD PTR [rip+0x5c9900]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   180016690:	8b 85 d8 01 00 00    	mov    eax,DWORD PTR [rbp+0x1d8]
   180016696:	89 05 cc 8e 27 01    	mov    DWORD PTR [rip+0x1278ecc],eax        # 0x18128f568
   18001669c:	48 8b 05 e5 6d 27 01 	mov    rax,QWORD PTR [rip+0x1276de5]        # 0x18128d488
   1800166a3:	48 85 c0             	test   rax,rax
   1800166a6:	75 4a                	jne    0x1800166f2
   1800166a8:	48 8d 15 e9 92 60 00 	lea    rdx,[rip+0x6092e9]        # 0x18061f998 ; 'deferredlight_filtering_specular.rfx'
   1800166af:	48 8b 0d 1a c6 86 00 	mov    rcx,QWORD PTR [rip+0x86c61a]        # 0x180882cd0
   1800166b6:	e8 a5 32 1c 00       	call   0x1801d9960
   1800166bb:	48 8b f8             	mov    rdi,rax
   1800166be:	48 8d 15 5b 92 60 00 	lea    rdx,[rip+0x60925b]        # 0x18061f920 ; 'temporal_feedback'
   1800166c5:	48 8b c8             	mov    rcx,rax
   1800166c8:	e8 93 c5 1b 00       	call   0x1801d2c60
   1800166cd:	48 85 c0             	test   rax,rax
   1800166d0:	0f 84 10 09 00 00    	je     0x180016fe6
   1800166d6:	48 89 05 ab 6d 27 01 	mov    QWORD PTR [rip+0x1276dab],rax        # 0x18128d488
   1800166dd:	48 8d 0d a4 6d 27 01 	lea    rcx,[rip+0x1276da4]        # 0x18128d488
   1800166e4:	e8 17 20 1c 00       	call   0x1801d8700
   1800166e9:	48 8b 05 98 6d 27 01 	mov    rax,QWORD PTR [rip+0x1276d98]        # 0x18128d488
   1800166f0:	33 ff                	xor    edi,edi
   1800166f2:	44 8b 50 08          	mov    r10d,DWORD PTR [rax+0x8]
   1800166f6:	41 f7 d2             	not    r10d
   1800166f9:	44 23 50 04          	and    r10d,DWORD PTR [rax+0x4]
   1800166fd:	44 8b cf             	mov    r9d,edi
   180016700:	44 8b 80 40 02 00 00 	mov    r8d,DWORD PTR [rax+0x240]
   180016707:	41 83 e8 01          	sub    r8d,0x1
   18001670b:	78 31                	js     0x18001673e
   18001670d:	4c 8b 98 38 02 00 00 	mov    r11,QWORD PTR [rax+0x238]
   180016714:	43 8d 0c 08          	lea    ecx,[r8+r9*1]
   180016718:	d1 f9                	sar    ecx,1
   18001671a:	48 63 c1             	movsxd rax,ecx
   18001671d:	48 69 d0 a8 00 00 00 	imul   rdx,rax,0xa8
   180016724:	49 03 d3             	add    rdx,r11
   180016727:	44 3b 52 04          	cmp    r10d,DWORD PTR [rdx+0x4]
   18001672b:	73 06                	jae    0x180016733
   18001672d:	44 8d 41 ff          	lea    r8d,[rcx-0x1]
   180016731:	eb 06                	jmp    0x180016739
   180016733:	76 0c                	jbe    0x180016741
   180016735:	44 8d 49 01          	lea    r9d,[rcx+0x1]
   180016739:	45 3b c8             	cmp    r9d,r8d
   18001673c:	7e d6                	jle    0x180016714
   18001673e:	48 8b d7             	mov    rdx,rdi
   180016741:	48 8b ca             	mov    rcx,rdx
   180016744:	e8 07 50 1c 00       	call   0x1801db750
   180016749:	41 8d 44 24 07       	lea    eax,[r12+0x7]
   18001674e:	99                   	cdq
   18001674f:	83 e2 07             	and    edx,0x7
   180016752:	8d 0c 02             	lea    ecx,[rdx+rax*1]
   180016755:	c1 f9 03             	sar    ecx,0x3
   180016758:	89 4d 90             	mov    DWORD PTR [rbp-0x70],ecx
   18001675b:	41 8d 45 07          	lea    eax,[r13+0x7]
   18001675f:	99                   	cdq
   180016760:	83 e2 07             	and    edx,0x7
   180016763:	03 c2                	add    eax,edx
   180016765:	c1 f8 03             	sar    eax,0x3
   180016768:	89 45 a0             	mov    DWORD PTR [rbp-0x60],eax
   18001676b:	89 4d a4             	mov    DWORD PTR [rbp-0x5c],ecx
   18001676e:	f2 0f 10 45 a0       	movsd  xmm0,QWORD PTR [rbp-0x60]
   180016773:	f2 0f 11 45 80       	movsd  QWORD PTR [rbp-0x80],xmm0
   180016778:	c7 45 88 01 00 00 00 	mov    DWORD PTR [rbp-0x78],0x1
   18001677f:	48 8d 55 80          	lea    rdx,[rbp-0x80]
   180016783:	49 8b ce             	mov    rcx,r14
   180016786:	ff 15 34 95 5c 00    	call   QWORD PTR [rip+0x5c9534]        # 0x1805dfcc0 ; ?dispatch@DeviceUtil@d3d@@SAXAEAVDeviceState@2@V?$Vector3Template@H@m@@@Z
   18001678c:	4c 8b e6             	mov    r12,rsi
   18001678f:	4c 8b f3             	mov    r14,rbx
   180016792:	48 c7 45 c0 00 00 00 	mov    QWORD PTR [rbp-0x40],0x0
   180016799:	00 
   18001679a:	89 7d c8             	mov    DWORD PTR [rbp-0x38],edi
   18001679d:	48 c7 45 80 00 00 00 	mov    QWORD PTR [rbp-0x80],0x0
   1800167a4:	00 
   1800167a5:	89 7d 88             	mov    DWORD PTR [rbp-0x78],edi
   1800167a8:	48 8b 3d 69 c3 8f 00 	mov    rdi,QWORD PTR [rip+0x8fc369]        # 0x180912b18
   1800167af:	48 8b cb             	mov    rcx,rbx
   1800167b2:	ff 15 78 95 5c 00    	call   QWORD PTR [rip+0x5c9578]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   1800167b8:	48 8b f0             	mov    rsi,rax
   1800167bb:	48 8b cf             	mov    rcx,rdi
   1800167be:	ff 15 6c 95 5c 00    	call   QWORD PTR [rip+0x5c956c]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   1800167c4:	48 8b c8             	mov    rcx,rax
   1800167c7:	33 ff                	xor    edi,edi
   1800167c9:	48 89 7c 24 40       	mov    QWORD PTR [rsp+0x40],rdi
   1800167ce:	48 8d 45 c0          	lea    rax,[rbp-0x40]
   1800167d2:	48 89 44 24 38       	mov    QWORD PTR [rsp+0x38],rax
   1800167d7:	89 7c 24 30          	mov    DWORD PTR [rsp+0x30],edi
   1800167db:	89 7c 24 28          	mov    DWORD PTR [rsp+0x28],edi
   1800167df:	48 89 74 24 20       	mov    QWORD PTR [rsp+0x20],rsi
   1800167e4:	4c 8d 4d 80          	lea    r9,[rbp-0x80]
   1800167e8:	45 33 c0             	xor    r8d,r8d
   1800167eb:	33 d2                	xor    edx,edx
   1800167ed:	ff 15 55 95 5c 00    	call   QWORD PTR [rip+0x5c9555]        # 0x1805dfd48 ; ?copyRegion@NativeTextureUtil@d3d@@SAXPEAVNativeTexture@2@HHAEBV?$Vector3Template@H@m@@0HH1PEBV45@@Z
   1800167f3:	8b f7                	mov    esi,edi
   1800167f5:	44 8b ad e0 01 00 00 	mov    r13d,DWORD PTR [rbp+0x1e0]
   1800167fc:	45 85 ed             	test   r13d,r13d
   1800167ff:	0f 84 51 03 00 00    	je     0x180016b56
   180016805:	b8 40 00 00 00       	mov    eax,0x40
   18001680a:	41 8b 04 07          	mov    eax,DWORD PTR [r15+rax*1]
   18001680e:	39 05 5c 8d 27 01    	cmp    DWORD PTR [rip+0x1278d5c],eax        # 0x18128f570
   180016814:	7e 6a                	jle    0x180016880
   180016816:	48 8d 0d 53 8d 27 01 	lea    rcx,[rip+0x1278d53]        # 0x18128f570
   18001681d:	e8 42 16 57 00       	call   0x180587e64
   180016822:	83 3d 47 8d 27 01 ff 	cmp    DWORD PTR [rip+0x1278d47],0xffffffff        # 0x18128f570
   180016829:	75 55                	jne    0x180016880
   18001682b:	48 89 3d 46 8d 27 01 	mov    QWORD PTR [rip+0x1278d46],rdi        # 0x18128f578
   180016832:	48 8d 15 77 90 60 00 	lea    rdx,[rip+0x609077]        # 0x18061f8b0 ; 'g_DLF_rwtColorTarget'
   180016839:	48 8d 0d 40 8d 27 01 	lea    rcx,[rip+0x1278d40]        # 0x18128f580
   180016840:	e8 2b 9a 1f 00       	call   0x180210270
   180016845:	c7 05 35 8d 27 01 09 	mov    DWORD PTR [rip+0x1278d35],0x9        # 0x18128f584
   18001684c:	00 00 00 
   18001684f:	48 8d 05 32 8d 27 01 	lea    rax,[rip+0x1278d32]        # 0x18128f588
   180016856:	48 89 05 1b 8d 27 01 	mov    QWORD PTR [rip+0x1278d1b],rax        # 0x18128f578
   18001685d:	48 8b c8             	mov    rcx,rax
   180016860:	ff 15 0a 94 5c 00    	call   QWORD PTR [rip+0x5c940a]        # 0x1805dfc70 ; ??0ShaderTexture@d3d@@QEAA@XZ
   180016866:	90                   	nop
   180016867:	48 8d 0d c2 26 5b 00 	lea    rcx,[rip+0x5b26c2]        # 0x1805c8f30
   18001686e:	e8 51 13 57 00       	call   0x180587bc4
   180016873:	90                   	nop
   180016874:	48 8d 0d f5 8c 27 01 	lea    rcx,[rip+0x1278cf5]        # 0x18128f570
   18001687b:	e8 84 15 57 00       	call   0x180587e04
   180016880:	4d 85 e4             	test   r12,r12
   180016883:	74 0c                	je     0x180016891
   180016885:	49 8b cc             	mov    rcx,r12
   180016888:	ff 15 a2 94 5c 00    	call   QWORD PTR [rip+0x5c94a2]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   18001688e:	48 8b f8             	mov    rdi,rax
   180016891:	8b 0d e9 8c 27 01    	mov    ecx,DWORD PTR [rip+0x1278ce9]        # 0x18128f580
   180016897:	ff 15 eb 93 5c 00    	call   QWORD PTR [rip+0x5c93eb]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   18001689d:	8b d0                	mov    edx,eax
   18001689f:	4c 8b c7             	mov    r8,rdi
   1800168a2:	48 8d 4d 68          	lea    rcx,[rbp+0x68]
   1800168a6:	ff 15 d4 93 5c 00    	call   QWORD PTR [rip+0x5c93d4]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   1800168ac:	45 33 c0             	xor    r8d,r8d
   1800168af:	48 8d 55 68          	lea    rdx,[rbp+0x68]
   1800168b3:	8b 0d c7 8c 27 01    	mov    ecx,DWORD PTR [rip+0x1278cc7]        # 0x18128f580
   1800168b9:	ff 15 d1 96 5c 00    	call   QWORD PTR [rip+0x5c96d1]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   1800168bf:	8b 0d bb 8c 27 01    	mov    ecx,DWORD PTR [rip+0x1278cbb]        # 0x18128f580
   1800168c5:	ff 15 bd 93 5c 00    	call   QWORD PTR [rip+0x5c93bd]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   1800168cb:	8b d0                	mov    edx,eax
   1800168cd:	4c 8b c7             	mov    r8,rdi
   1800168d0:	48 8d 0d b1 8c 27 01 	lea    rcx,[rip+0x1278cb1]        # 0x18128f588
   1800168d7:	ff 15 b3 93 5c 00    	call   QWORD PTR [rip+0x5c93b3]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   1800168dd:	b9 40 00 00 00       	mov    ecx,0x40
   1800168e2:	41 8b 04 0f          	mov    eax,DWORD PTR [r15+rcx*1]
   1800168e6:	39 05 ac 8c 27 01    	cmp    DWORD PTR [rip+0x1278cac],eax        # 0x18128f598
   1800168ec:	7e 6e                	jle    0x18001695c
   1800168ee:	48 8d 0d a3 8c 27 01 	lea    rcx,[rip+0x1278ca3]        # 0x18128f598
   1800168f5:	e8 6a 15 57 00       	call   0x180587e64
   1800168fa:	83 3d 97 8c 27 01 ff 	cmp    DWORD PTR [rip+0x1278c97],0xffffffff        # 0x18128f598
   180016901:	75 59                	jne    0x18001695c
   180016903:	48 c7 05 92 8c 27 01 	mov    QWORD PTR [rip+0x1278c92],0x0        # 0x18128f5a0
   18001690a:	00 00 00 00 
   18001690e:	48 8d 15 b3 8f 60 00 	lea    rdx,[rip+0x608fb3]        # 0x18061f8c8 ; 'g_DLF_rwtColorSource'
   180016915:	48 8d 0d 8c 8c 27 01 	lea    rcx,[rip+0x1278c8c]        # 0x18128f5a8
   18001691c:	e8 4f 99 1f 00       	call   0x180210270
   180016921:	c7 05 81 8c 27 01 09 	mov    DWORD PTR [rip+0x1278c81],0x9        # 0x18128f5ac
   180016928:	00 00 00 
   18001692b:	48 8d 05 7e 8c 27 01 	lea    rax,[rip+0x1278c7e]        # 0x18128f5b0
   180016932:	48 89 05 67 8c 27 01 	mov    QWORD PTR [rip+0x1278c67],rax        # 0x18128f5a0
   180016939:	48 8b c8             	mov    rcx,rax
   18001693c:	ff 15 2e 93 5c 00    	call   QWORD PTR [rip+0x5c932e]        # 0x1805dfc70 ; ??0ShaderTexture@d3d@@QEAA@XZ
   180016942:	90                   	nop
   180016943:	48 8d 0d b6 25 5b 00 	lea    rcx,[rip+0x5b25b6]        # 0x1805c8f00
   18001694a:	e8 75 12 57 00       	call   0x180587bc4
   18001694f:	90                   	nop
   180016950:	48 8d 0d 41 8c 27 01 	lea    rcx,[rip+0x1278c41]        # 0x18128f598
   180016957:	e8 a8 14 57 00       	call   0x180587e04
   18001695c:	4d 85 f6             	test   r14,r14
   18001695f:	74 0e                	je     0x18001696f
   180016961:	49 8b ce             	mov    rcx,r14
   180016964:	ff 15 c6 93 5c 00    	call   QWORD PTR [rip+0x5c93c6]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   18001696a:	48 8b f8             	mov    rdi,rax
   18001696d:	eb 02                	jmp    0x180016971
   18001696f:	33 ff                	xor    edi,edi
   180016971:	8b 0d 31 8c 27 01    	mov    ecx,DWORD PTR [rip+0x1278c31]        # 0x18128f5a8
   180016977:	ff 15 0b 93 5c 00    	call   QWORD PTR [rip+0x5c930b]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   18001697d:	8b d0                	mov    edx,eax
   18001697f:	4c 8b c7             	mov    r8,rdi
   180016982:	48 8d 4d e0          	lea    rcx,[rbp-0x20]
   180016986:	ff 15 f4 92 5c 00    	call   QWORD PTR [rip+0x5c92f4]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   18001698c:	45 33 c0             	xor    r8d,r8d
   18001698f:	48 8d 55 e0          	lea    rdx,[rbp-0x20]
   180016993:	8b 0d 0f 8c 27 01    	mov    ecx,DWORD PTR [rip+0x1278c0f]        # 0x18128f5a8
   180016999:	ff 15 f1 95 5c 00    	call   QWORD PTR [rip+0x5c95f1]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   18001699f:	8b 0d 03 8c 27 01    	mov    ecx,DWORD PTR [rip+0x1278c03]        # 0x18128f5a8
   1800169a5:	ff 15 dd 92 5c 00    	call   QWORD PTR [rip+0x5c92dd]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   1800169ab:	8b d0                	mov    edx,eax
   1800169ad:	4c 8b c7             	mov    r8,rdi
   1800169b0:	48 8d 0d f9 8b 27 01 	lea    rcx,[rip+0x1278bf9]        # 0x18128f5b0
   1800169b7:	ff 15 d3 92 5c 00    	call   QWORD PTR [rip+0x5c92d3]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   1800169bd:	b9 40 00 00 00       	mov    ecx,0x40
   1800169c2:	41 8b 04 0f          	mov    eax,DWORD PTR [r15+rcx*1]
   1800169c6:	39 05 f4 8b 27 01    	cmp    DWORD PTR [rip+0x1278bf4],eax        # 0x18128f5c0
   1800169cc:	7e 64                	jle    0x180016a32
   1800169ce:	48 8d 0d eb 8b 27 01 	lea    rcx,[rip+0x1278beb]        # 0x18128f5c0
   1800169d5:	e8 8a 14 57 00       	call   0x180587e64
   1800169da:	83 3d df 8b 27 01 ff 	cmp    DWORD PTR [rip+0x1278bdf],0xffffffff        # 0x18128f5c0
   1800169e1:	75 4f                	jne    0x180016a32
   1800169e3:	48 c7 05 da 8b 27 01 	mov    QWORD PTR [rip+0x1278bda],0x0        # 0x18128f5c8
   1800169ea:	00 00 00 00 
   1800169ee:	48 8d 15 eb 8e 60 00 	lea    rdx,[rip+0x608eeb]        # 0x18061f8e0 ; 'g_DLF_iFilterPass'
   1800169f5:	48 8d 0d d4 8b 27 01 	lea    rcx,[rip+0x1278bd4]        # 0x18128f5d0
   1800169fc:	e8 6f 98 1f 00       	call   0x180210270
   180016a01:	c7 05 c9 8b 27 01 10 	mov    DWORD PTR [rip+0x1278bc9],0x10        # 0x18128f5d4
   180016a08:	00 00 00 
   180016a0b:	48 8d 05 c6 8b 27 01 	lea    rax,[rip+0x1278bc6]        # 0x18128f5d8
   180016a12:	48 89 05 af 8b 27 01 	mov    QWORD PTR [rip+0x1278baf],rax        # 0x18128f5c8
   180016a19:	48 8d 0d b0 24 5b 00 	lea    rcx,[rip+0x5b24b0]        # 0x1805c8ed0
   180016a20:	e8 9f 11 57 00       	call   0x180587bc4
   180016a25:	90                   	nop
   180016a26:	48 8d 0d 93 8b 27 01 	lea    rcx,[rip+0x1278b93]        # 0x18128f5c0
   180016a2d:	e8 d2 13 57 00       	call   0x180587e04
   180016a32:	89 b5 d8 01 00 00    	mov    DWORD PTR [rbp+0x1d8],esi
   180016a38:	45 33 c0             	xor    r8d,r8d
   180016a3b:	48 8d 95 d8 01 00 00 	lea    rdx,[rbp+0x1d8]
   180016a42:	8b 0d 88 8b 27 01    	mov    ecx,DWORD PTR [rip+0x1278b88]        # 0x18128f5d0
   180016a48:	ff 15 42 95 5c 00    	call   QWORD PTR [rip+0x5c9542]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   180016a4e:	8b 85 d8 01 00 00    	mov    eax,DWORD PTR [rbp+0x1d8]
   180016a54:	89 05 7e 8b 27 01    	mov    DWORD PTR [rip+0x1278b7e],eax        # 0x18128f5d8
   180016a5a:	8b fe                	mov    edi,esi
   180016a5c:	83 e7 01             	and    edi,0x1
   180016a5f:	48 8b 0d 1a 6a 27 01 	mov    rcx,QWORD PTR [rip+0x1276a1a]        # 0x18128d480
   180016a66:	48 85 c9             	test   rcx,rcx
   180016a69:	75 4f                	jne    0x180016aba
   180016a6b:	48 8d 15 26 8f 60 00 	lea    rdx,[rip+0x608f26]        # 0x18061f998 ; 'deferredlight_filtering_specular.rfx'
   180016a72:	48 8b 0d 57 c2 86 00 	mov    rcx,QWORD PTR [rip+0x86c257]        # 0x180882cd0
   180016a79:	e8 e2 2e 1c 00       	call   0x1801d9960
   180016a7e:	4c 8b e8             	mov    r13,rax
   180016a81:	48 8d 15 b0 8e 60 00 	lea    rdx,[rip+0x608eb0]        # 0x18061f938 ; 'filter_color'
   180016a88:	48 8b c8             	mov    rcx,rax
   180016a8b:	e8 d0 c1 1b 00       	call   0x1801d2c60
   180016a90:	48 85 c0             	test   rax,rax
   180016a93:	0f 84 7b 05 00 00    	je     0x180017014
   180016a99:	48 89 05 e0 69 27 01 	mov    QWORD PTR [rip+0x12769e0],rax        # 0x18128d480
   180016aa0:	48 8d 0d d9 69 27 01 	lea    rcx,[rip+0x12769d9]        # 0x18128d480
   180016aa7:	e8 54 1c 1c 00       	call   0x1801d8700
   180016aac:	48 8b 0d cd 69 27 01 	mov    rcx,QWORD PTR [rip+0x12769cd]        # 0x18128d480
   180016ab3:	44 8b ad e0 01 00 00 	mov    r13d,DWORD PTR [rbp+0x1e0]
   180016aba:	44 8b 51 04          	mov    r10d,DWORD PTR [rcx+0x4]
   180016abe:	44 0b d7             	or     r10d,edi
   180016ac1:	8b 41 08             	mov    eax,DWORD PTR [rcx+0x8]
   180016ac4:	f7 d0                	not    eax
   180016ac6:	44 23 d0             	and    r10d,eax
   180016ac9:	33 ff                	xor    edi,edi
   180016acb:	44 8b cf             	mov    r9d,edi
   180016ace:	44 8b 81 40 02 00 00 	mov    r8d,DWORD PTR [rcx+0x240]
   180016ad5:	41 83 e8 01          	sub    r8d,0x1
   180016ad9:	78 31                	js     0x180016b0c
   180016adb:	4c 8b 99 38 02 00 00 	mov    r11,QWORD PTR [rcx+0x238]
   180016ae2:	43 8d 0c 08          	lea    ecx,[r8+r9*1]
   180016ae6:	d1 f9                	sar    ecx,1
   180016ae8:	48 63 c1             	movsxd rax,ecx
   180016aeb:	48 69 d0 a8 00 00 00 	imul   rdx,rax,0xa8
   180016af2:	49 03 d3             	add    rdx,r11
   180016af5:	44 3b 52 04          	cmp    r10d,DWORD PTR [rdx+0x4]
   180016af9:	73 06                	jae    0x180016b01
   180016afb:	44 8d 41 ff          	lea    r8d,[rcx-0x1]
   180016aff:	eb 06                	jmp    0x180016b07
   180016b01:	76 0c                	jbe    0x180016b0f
   180016b03:	44 8d 49 01          	lea    r9d,[rcx+0x1]
   180016b07:	45 3b c8             	cmp    r9d,r8d
   180016b0a:	7e d6                	jle    0x180016ae2
   180016b0c:	48 8b d7             	mov    rdx,rdi
   180016b0f:	48 8b ca             	mov    rcx,rdx
   180016b12:	e8 39 4c 1c 00       	call   0x1801db750
   180016b17:	8b 45 a0             	mov    eax,DWORD PTR [rbp-0x60]
   180016b1a:	89 45 80             	mov    DWORD PTR [rbp-0x80],eax
   180016b1d:	8b 45 90             	mov    eax,DWORD PTR [rbp-0x70]
   180016b20:	89 45 84             	mov    DWORD PTR [rbp-0x7c],eax
   180016b23:	f2 0f 10 45 80       	movsd  xmm0,QWORD PTR [rbp-0x80]
   180016b28:	f2 0f 11 45 c0       	movsd  QWORD PTR [rbp-0x40],xmm0
   180016b2d:	c7 45 c8 01 00 00 00 	mov    DWORD PTR [rbp-0x38],0x1
   180016b34:	48 8d 55 c0          	lea    rdx,[rbp-0x40]
   180016b38:	48 8b 4d d0          	mov    rcx,QWORD PTR [rbp-0x30]
   180016b3c:	ff 15 7e 91 5c 00    	call   QWORD PTR [rip+0x5c917e]        # 0x1805dfcc0 ; ?dispatch@DeviceUtil@d3d@@SAXAEAVDeviceState@2@V?$Vector3Template@H@m@@@Z
   180016b42:	49 8b c4             	mov    rax,r12
   180016b45:	4d 8b e6             	mov    r12,r14
   180016b48:	4c 8b f0             	mov    r14,rax
   180016b4b:	ff c6                	inc    esi
   180016b4d:	41 3b f5             	cmp    esi,r13d
   180016b50:	0f 82 af fc ff ff    	jb     0x180016805
   180016b56:	48 c7 45 80 00 00 00 	mov    QWORD PTR [rbp-0x80],0x0
   180016b5d:	00 
   180016b5e:	89 7d 88             	mov    DWORD PTR [rbp-0x78],edi
   180016b61:	48 c7 45 c0 00 00 00 	mov    QWORD PTR [rbp-0x40],0x0
   180016b68:	00 
   180016b69:	89 7d c8             	mov    DWORD PTR [rbp-0x38],edi
   180016b6c:	48 8b 3d ad bf 8f 00 	mov    rdi,QWORD PTR [rip+0x8fbfad]        # 0x180912b20
   180016b73:	49 8b ce             	mov    rcx,r14
   180016b76:	ff 15 b4 91 5c 00    	call   QWORD PTR [rip+0x5c91b4]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   180016b7c:	48 8b f0             	mov    rsi,rax
   180016b7f:	48 8b cf             	mov    rcx,rdi
   180016b82:	ff 15 a8 91 5c 00    	call   QWORD PTR [rip+0x5c91a8]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   180016b88:	48 8b c8             	mov    rcx,rax
   180016b8b:	33 ff                	xor    edi,edi
   180016b8d:	48 89 7c 24 40       	mov    QWORD PTR [rsp+0x40],rdi
   180016b92:	48 8d 45 80          	lea    rax,[rbp-0x80]
   180016b96:	48 89 44 24 38       	mov    QWORD PTR [rsp+0x38],rax
   180016b9b:	89 7c 24 30          	mov    DWORD PTR [rsp+0x30],edi
   180016b9f:	89 7c 24 28          	mov    DWORD PTR [rsp+0x28],edi
   180016ba3:	48 89 74 24 20       	mov    QWORD PTR [rsp+0x20],rsi
   180016ba8:	4c 8d 4d c0          	lea    r9,[rbp-0x40]
   180016bac:	45 33 c0             	xor    r8d,r8d
   180016baf:	33 d2                	xor    edx,edx
   180016bb1:	ff 15 91 91 5c 00    	call   QWORD PTR [rip+0x5c9191]        # 0x1805dfd48 ; ?copyRegion@NativeTextureUtil@d3d@@SAXPEAVNativeTexture@2@HHAEBV?$Vector3Template@H@m@@0HH1PEBV45@@Z
   180016bb7:	40 38 bd e8 01 00 00 	cmp    BYTE PTR [rbp+0x1e8],dil
   180016bbe:	0f 84 27 03 00 00    	je     0x180016eeb
   180016bc4:	be 40 00 00 00       	mov    esi,0x40
   180016bc9:	41 8b 04 37          	mov    eax,DWORD PTR [r15+rsi*1]
   180016bcd:	39 05 0d 8a 27 01    	cmp    DWORD PTR [rip+0x1278a0d],eax        # 0x18128f5e0
   180016bd3:	7e 6a                	jle    0x180016c3f
   180016bd5:	48 8d 0d 04 8a 27 01 	lea    rcx,[rip+0x1278a04]        # 0x18128f5e0
   180016bdc:	e8 83 12 57 00       	call   0x180587e64
   180016be1:	83 3d f8 89 27 01 ff 	cmp    DWORD PTR [rip+0x12789f8],0xffffffff        # 0x18128f5e0
   180016be8:	75 55                	jne    0x180016c3f
   180016bea:	48 89 3d f7 89 27 01 	mov    QWORD PTR [rip+0x12789f7],rdi        # 0x18128f5e8
   180016bf1:	48 8d 15 b8 8c 60 00 	lea    rdx,[rip+0x608cb8]        # 0x18061f8b0 ; 'g_DLF_rwtColorTarget'
   180016bf8:	48 8d 0d f1 89 27 01 	lea    rcx,[rip+0x12789f1]        # 0x18128f5f0
   180016bff:	e8 6c 96 1f 00       	call   0x180210270
   180016c04:	c7 05 e6 89 27 01 09 	mov    DWORD PTR [rip+0x12789e6],0x9        # 0x18128f5f4
   180016c0b:	00 00 00 
   180016c0e:	48 8d 05 e3 89 27 01 	lea    rax,[rip+0x12789e3]        # 0x18128f5f8
   180016c15:	48 89 05 cc 89 27 01 	mov    QWORD PTR [rip+0x12789cc],rax        # 0x18128f5e8
   180016c1c:	48 8b c8             	mov    rcx,rax
   180016c1f:	ff 15 4b 90 5c 00    	call   QWORD PTR [rip+0x5c904b]        # 0x1805dfc70 ; ??0ShaderTexture@d3d@@QEAA@XZ
   180016c25:	90                   	nop
   180016c26:	48 8d 0d 73 22 5b 00 	lea    rcx,[rip+0x5b2273]        # 0x1805c8ea0
   180016c2d:	e8 92 0f 57 00       	call   0x180587bc4
   180016c32:	90                   	nop
   180016c33:	48 8d 0d a6 89 27 01 	lea    rcx,[rip+0x12789a6]        # 0x18128f5e0
   180016c3a:	e8 c5 11 57 00       	call   0x180587e04
   180016c3f:	4d 85 e4             	test   r12,r12
   180016c42:	74 0c                	je     0x180016c50
   180016c44:	49 8b cc             	mov    rcx,r12
   180016c47:	ff 15 e3 90 5c 00    	call   QWORD PTR [rip+0x5c90e3]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   180016c4d:	48 8b f8             	mov    rdi,rax
   180016c50:	8b 0d 9a 89 27 01    	mov    ecx,DWORD PTR [rip+0x127899a]        # 0x18128f5f0
   180016c56:	ff 15 2c 90 5c 00    	call   QWORD PTR [rip+0x5c902c]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   180016c5c:	8b d0                	mov    edx,eax
   180016c5e:	4c 8b c7             	mov    r8,rdi
   180016c61:	48 8d 4d e0          	lea    rcx,[rbp-0x20]
   180016c65:	ff 15 15 90 5c 00    	call   QWORD PTR [rip+0x5c9015]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   180016c6b:	45 33 c0             	xor    r8d,r8d
   180016c6e:	48 8d 55 e0          	lea    rdx,[rbp-0x20]
   180016c72:	8b 0d 78 89 27 01    	mov    ecx,DWORD PTR [rip+0x1278978]        # 0x18128f5f0
   180016c78:	ff 15 12 93 5c 00    	call   QWORD PTR [rip+0x5c9312]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   180016c7e:	8b 0d 6c 89 27 01    	mov    ecx,DWORD PTR [rip+0x127896c]        # 0x18128f5f0
   180016c84:	ff 15 fe 8f 5c 00    	call   QWORD PTR [rip+0x5c8ffe]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   180016c8a:	8b d0                	mov    edx,eax
   180016c8c:	4c 8b c7             	mov    r8,rdi
   180016c8f:	48 8d 0d 62 89 27 01 	lea    rcx,[rip+0x1278962]        # 0x18128f5f8
   180016c96:	ff 15 f4 8f 5c 00    	call   QWORD PTR [rip+0x5c8ff4]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   180016c9c:	48 8d 3d 7d 89 27 01 	lea    rdi,[rip+0x127897d]        # 0x18128f620
   180016ca3:	41 8b 04 37          	mov    eax,DWORD PTR [r15+rsi*1]
   180016ca7:	39 05 5b 89 27 01    	cmp    DWORD PTR [rip+0x127895b],eax        # 0x18128f608
   180016cad:	7e 67                	jle    0x180016d16
   180016caf:	48 8d 0d 52 89 27 01 	lea    rcx,[rip+0x1278952]        # 0x18128f608
   180016cb6:	e8 a9 11 57 00       	call   0x180587e64
   180016cbb:	83 3d 46 89 27 01 ff 	cmp    DWORD PTR [rip+0x1278946],0xffffffff        # 0x18128f608
   180016cc2:	75 52                	jne    0x180016d16
   180016cc4:	48 c7 05 41 89 27 01 	mov    QWORD PTR [rip+0x1278941],0x0        # 0x18128f610
   180016ccb:	00 00 00 00 
   180016ccf:	48 8d 15 f2 8b 60 00 	lea    rdx,[rip+0x608bf2]        # 0x18061f8c8 ; 'g_DLF_rwtColorSource'
   180016cd6:	48 8d 0d 3b 89 27 01 	lea    rcx,[rip+0x127893b]        # 0x18128f618
   180016cdd:	e8 8e 95 1f 00       	call   0x180210270
   180016ce2:	c7 05 30 89 27 01 09 	mov    DWORD PTR [rip+0x1278930],0x9        # 0x18128f61c
   180016ce9:	00 00 00 
   180016cec:	48 89 3d 1d 89 27 01 	mov    QWORD PTR [rip+0x127891d],rdi        # 0x18128f610
   180016cf3:	48 8b cf             	mov    rcx,rdi
   180016cf6:	ff 15 74 8f 5c 00    	call   QWORD PTR [rip+0x5c8f74]        # 0x1805dfc70 ; ??0ShaderTexture@d3d@@QEAA@XZ
   180016cfc:	90                   	nop
   180016cfd:	48 8d 0d 6c 21 5b 00 	lea    rcx,[rip+0x5b216c]        # 0x1805c8e70
   180016d04:	e8 bb 0e 57 00       	call   0x180587bc4
   180016d09:	90                   	nop
   180016d0a:	48 8d 0d f7 88 27 01 	lea    rcx,[rip+0x12788f7]        # 0x18128f608
   180016d11:	e8 ee 10 57 00       	call   0x180587e04
   180016d16:	4d 85 f6             	test   r14,r14
   180016d19:	74 0e                	je     0x180016d29
   180016d1b:	49 8b ce             	mov    rcx,r14
   180016d1e:	ff 15 0c 90 5c 00    	call   QWORD PTR [rip+0x5c900c]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   180016d24:	48 8b f8             	mov    rdi,rax
   180016d27:	eb 02                	jmp    0x180016d2b
   180016d29:	33 ff                	xor    edi,edi
   180016d2b:	8b 0d e7 88 27 01    	mov    ecx,DWORD PTR [rip+0x12788e7]        # 0x18128f618
   180016d31:	ff 15 51 8f 5c 00    	call   QWORD PTR [rip+0x5c8f51]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   180016d37:	8b d0                	mov    edx,eax
   180016d39:	4c 8b c7             	mov    r8,rdi
   180016d3c:	48 8d 4d e0          	lea    rcx,[rbp-0x20]
   180016d40:	ff 15 3a 8f 5c 00    	call   QWORD PTR [rip+0x5c8f3a]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   180016d46:	45 33 c0             	xor    r8d,r8d
   180016d49:	48 8d 55 e0          	lea    rdx,[rbp-0x20]
   180016d4d:	8b 0d c5 88 27 01    	mov    ecx,DWORD PTR [rip+0x12788c5]        # 0x18128f618
   180016d53:	ff 15 37 92 5c 00    	call   QWORD PTR [rip+0x5c9237]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   180016d59:	8b 0d b9 88 27 01    	mov    ecx,DWORD PTR [rip+0x12788b9]        # 0x18128f618
   180016d5f:	ff 15 23 8f 5c 00    	call   QWORD PTR [rip+0x5c8f23]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   180016d65:	8b d0                	mov    edx,eax
   180016d67:	4c 8b c7             	mov    r8,rdi
   180016d6a:	48 8d 0d af 88 27 01 	lea    rcx,[rip+0x12788af]        # 0x18128f620
   180016d71:	ff 15 19 8f 5c 00    	call   QWORD PTR [rip+0x5c8f19]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   180016d77:	41 8b 04 37          	mov    eax,DWORD PTR [r15+rsi*1]
   180016d7b:	39 05 af 88 27 01    	cmp    DWORD PTR [rip+0x12788af],eax        # 0x18128f630
   180016d81:	7e 64                	jle    0x180016de7
   180016d83:	48 8d 0d a6 88 27 01 	lea    rcx,[rip+0x12788a6]        # 0x18128f630
   180016d8a:	e8 d5 10 57 00       	call   0x180587e64
   180016d8f:	83 3d 9a 88 27 01 ff 	cmp    DWORD PTR [rip+0x127889a],0xffffffff        # 0x18128f630
   180016d96:	75 4f                	jne    0x180016de7
   180016d98:	48 c7 05 95 88 27 01 	mov    QWORD PTR [rip+0x1278895],0x0        # 0x18128f638
   180016d9f:	00 00 00 00 
   180016da3:	48 8d 15 36 8b 60 00 	lea    rdx,[rip+0x608b36]        # 0x18061f8e0 ; 'g_DLF_iFilterPass'
   180016daa:	48 8d 0d 8f 88 27 01 	lea    rcx,[rip+0x127888f]        # 0x18128f640
   180016db1:	e8 ba 94 1f 00       	call   0x180210270
   180016db6:	c7 05 84 88 27 01 10 	mov    DWORD PTR [rip+0x1278884],0x10        # 0x18128f644
   180016dbd:	00 00 00 
   180016dc0:	48 8d 05 81 88 27 01 	lea    rax,[rip+0x1278881]        # 0x18128f648
   180016dc7:	48 89 05 6a 88 27 01 	mov    QWORD PTR [rip+0x127886a],rax        # 0x18128f638
   180016dce:	48 8d 0d 6b 20 5b 00 	lea    rcx,[rip+0x5b206b]        # 0x1805c8e40
   180016dd5:	e8 ea 0d 57 00       	call   0x180587bc4
   180016dda:	90                   	nop
   180016ddb:	48 8d 0d 4e 88 27 01 	lea    rcx,[rip+0x127884e]        # 0x18128f630
   180016de2:	e8 1d 10 57 00       	call   0x180587e04
   180016de7:	44 89 ad e0 01 00 00 	mov    DWORD PTR [rbp+0x1e0],r13d
   180016dee:	45 33 c0             	xor    r8d,r8d
   180016df1:	48 8d 95 e0 01 00 00 	lea    rdx,[rbp+0x1e0]
   180016df8:	8b 0d 42 88 27 01    	mov    ecx,DWORD PTR [rip+0x1278842]        # 0x18128f640
   180016dfe:	ff 15 8c 91 5c 00    	call   QWORD PTR [rip+0x5c918c]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   180016e04:	8b 85 e0 01 00 00    	mov    eax,DWORD PTR [rbp+0x1e0]
   180016e0a:	89 05 38 88 27 01    	mov    DWORD PTR [rip+0x1278838],eax        # 0x18128f648
   180016e10:	48 8b 05 61 66 27 01 	mov    rax,QWORD PTR [rip+0x1276661]        # 0x18128d478
   180016e17:	48 85 c0             	test   rax,rax
   180016e1a:	75 48                	jne    0x180016e64
   180016e1c:	48 8d 15 75 8b 60 00 	lea    rdx,[rip+0x608b75]        # 0x18061f998 ; 'deferredlight_filtering_specular.rfx'
   180016e23:	48 8b 0d a6 be 86 00 	mov    rcx,QWORD PTR [rip+0x86bea6]        # 0x180882cd0
   180016e2a:	e8 31 2b 1c 00       	call   0x1801d9960
   180016e2f:	48 8b f8             	mov    rdi,rax
   180016e32:	48 8d 15 87 8b 60 00 	lea    rdx,[rip+0x608b87]        # 0x18061f9c0 ; 'evaluate_reflected_color'
   180016e39:	48 8b c8             	mov    rcx,rax
   180016e3c:	e8 1f be 1b 00       	call   0x1801d2c60
   180016e41:	48 85 c0             	test   rax,rax
   180016e44:	0f 84 6e 01 00 00    	je     0x180016fb8
   180016e4a:	48 89 05 27 66 27 01 	mov    QWORD PTR [rip+0x1276627],rax        # 0x18128d478
   180016e51:	48 8d 0d 20 66 27 01 	lea    rcx,[rip+0x1276620]        # 0x18128d478
   180016e58:	e8 a3 18 1c 00       	call   0x1801d8700
   180016e5d:	48 8b 05 14 66 27 01 	mov    rax,QWORD PTR [rip+0x1276614]        # 0x18128d478
   180016e64:	44 8b 50 08          	mov    r10d,DWORD PTR [rax+0x8]
   180016e68:	41 f7 d2             	not    r10d
   180016e6b:	44 23 50 04          	and    r10d,DWORD PTR [rax+0x4]
   180016e6f:	33 ff                	xor    edi,edi
   180016e71:	44 8b cf             	mov    r9d,edi
   180016e74:	44 8b 80 40 02 00 00 	mov    r8d,DWORD PTR [rax+0x240]
   180016e7b:	41 83 e8 01          	sub    r8d,0x1
   180016e7f:	78 31                	js     0x180016eb2
   180016e81:	4c 8b 98 38 02 00 00 	mov    r11,QWORD PTR [rax+0x238]
   180016e88:	43 8d 0c 08          	lea    ecx,[r8+r9*1]
   180016e8c:	d1 f9                	sar    ecx,1
   180016e8e:	48 63 c1             	movsxd rax,ecx
   180016e91:	48 69 d0 a8 00 00 00 	imul   rdx,rax,0xa8
   180016e98:	49 03 d3             	add    rdx,r11
   180016e9b:	44 3b 52 04          	cmp    r10d,DWORD PTR [rdx+0x4]
   180016e9f:	73 06                	jae    0x180016ea7
   180016ea1:	44 8d 41 ff          	lea    r8d,[rcx-0x1]
   180016ea5:	eb 06                	jmp    0x180016ead
   180016ea7:	76 0c                	jbe    0x180016eb5
   180016ea9:	44 8d 49 01          	lea    r9d,[rcx+0x1]
   180016ead:	45 3b c8             	cmp    r9d,r8d
   180016eb0:	7e d6                	jle    0x180016e88
   180016eb2:	48 8b d7             	mov    rdx,rdi
   180016eb5:	48 8b ca             	mov    rcx,rdx
   180016eb8:	e8 93 48 1c 00       	call   0x1801db750
   180016ebd:	8b 45 a0             	mov    eax,DWORD PTR [rbp-0x60]
   180016ec0:	89 45 a0             	mov    DWORD PTR [rbp-0x60],eax
   180016ec3:	8b 45 90             	mov    eax,DWORD PTR [rbp-0x70]
   180016ec6:	89 45 a4             	mov    DWORD PTR [rbp-0x5c],eax
   180016ec9:	f2 0f 10 45 a0       	movsd  xmm0,QWORD PTR [rbp-0x60]
   180016ece:	f2 0f 11 45 a0       	movsd  QWORD PTR [rbp-0x60],xmm0
   180016ed3:	c7 45 a8 01 00 00 00 	mov    DWORD PTR [rbp-0x58],0x1
   180016eda:	48 8d 55 a0          	lea    rdx,[rbp-0x60]
   180016ede:	48 8b 4d d0          	mov    rcx,QWORD PTR [rbp-0x30]
   180016ee2:	ff 15 d8 8d 5c 00    	call   QWORD PTR [rip+0x5c8dd8]        # 0x1805dfcc0 ; ?dispatch@DeviceUtil@d3d@@SAXAEAVDeviceState@2@V?$Vector3Template@H@m@@@Z
   180016ee8:	4d 8b f4             	mov    r14,r12
   180016eeb:	48 8b b5 d0 01 00 00 	mov    rsi,QWORD PTR [rbp+0x1d0]
   180016ef2:	4c 3b f6             	cmp    r14,rsi
   180016ef5:	74 59                	je     0x180016f50
   180016ef7:	48 c7 45 a0 00 00 00 	mov    QWORD PTR [rbp-0x60],0x0
   180016efe:	00 
   180016eff:	89 7d a8             	mov    DWORD PTR [rbp-0x58],edi
   180016f02:	48 c7 45 d0 00 00 00 	mov    QWORD PTR [rbp-0x30],0x0
   180016f09:	00 
   180016f0a:	89 7d d8             	mov    DWORD PTR [rbp-0x28],edi
   180016f0d:	49 8b ce             	mov    rcx,r14
   180016f10:	ff 15 1a 8e 5c 00    	call   QWORD PTR [rip+0x5c8e1a]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   180016f16:	48 8b f8             	mov    rdi,rax
   180016f19:	48 8b ce             	mov    rcx,rsi
   180016f1c:	ff 15 0e 8e 5c 00    	call   QWORD PTR [rip+0x5c8e0e]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   180016f22:	48 8b c8             	mov    rcx,rax
   180016f25:	33 d2                	xor    edx,edx
   180016f27:	48 89 54 24 40       	mov    QWORD PTR [rsp+0x40],rdx
   180016f2c:	48 8d 45 a0          	lea    rax,[rbp-0x60]
   180016f30:	48 89 44 24 38       	mov    QWORD PTR [rsp+0x38],rax
   180016f35:	89 54 24 30          	mov    DWORD PTR [rsp+0x30],edx
   180016f39:	89 54 24 28          	mov    DWORD PTR [rsp+0x28],edx
   180016f3d:	48 89 7c 24 20       	mov    QWORD PTR [rsp+0x20],rdi
   180016f42:	4c 8d 4d d0          	lea    r9,[rbp-0x30]
   180016f46:	45 33 c0             	xor    r8d,r8d
   180016f49:	ff 15 f9 8d 5c 00    	call   QWORD PTR [rip+0x5c8df9]        # 0x1805dfd48 ; ?copyRegion@NativeTextureUtil@d3d@@SAXPEAVNativeTexture@2@HHAEBV?$Vector3Template@H@m@@0HH1PEBV45@@Z
   180016f4f:	90                   	nop
   180016f50:	48 85 db             	test   rbx,rbx
   180016f53:	74 10                	je     0x180016f65
   180016f55:	48 8b d3             	mov    rdx,rbx
   180016f58:	48 8b 0d 79 b9 7e 00 	mov    rcx,QWORD PTR [rip+0x7eb979]        # 0x1808028d8
   180016f5f:	e8 9c 2e 09 00       	call   0x1800a9e00
   180016f64:	90                   	nop
   180016f65:	4c 8d 9c 24 88 02 00 	lea    r11,[rsp+0x288]
   180016f6c:	00 
   180016f6d:	41 0f 28 73 e8       	movaps xmm6,XMMWORD PTR [r11-0x18]
   180016f72:	41 0f 28 7b d8       	movaps xmm7,XMMWORD PTR [r11-0x28]
   180016f77:	45 0f 28 43 c8       	movaps xmm8,XMMWORD PTR [r11-0x38]
   180016f7c:	45 0f 28 4b b8       	movaps xmm9,XMMWORD PTR [r11-0x48]
   180016f81:	45 0f 28 53 a8       	movaps xmm10,XMMWORD PTR [r11-0x58]
   180016f86:	45 0f 28 5b 98       	movaps xmm11,XMMWORD PTR [r11-0x68]
   180016f8b:	45 0f 28 63 88       	movaps xmm12,XMMWORD PTR [r11-0x78]
   180016f90:	45 0f 28 ab 78 ff ff 	movaps xmm13,XMMWORD PTR [r11-0x88]
   180016f97:	ff 
   180016f98:	45 0f 28 b3 68 ff ff 	movaps xmm14,XMMWORD PTR [r11-0x98]
   180016f9f:	ff 
   180016fa0:	45 0f 28 bb 58 ff ff 	movaps xmm15,XMMWORD PTR [r11-0xa8]
   180016fa7:	ff 
   180016fa8:	49 8b e3             	mov    rsp,r11
   180016fab:	41 5f                	pop    r15
   180016fad:	41 5e                	pop    r14
   180016faf:	41 5d                	pop    r13
   180016fb1:	41 5c                	pop    r12
   180016fb3:	5f                   	pop    rdi
   180016fb4:	5e                   	pop    rsi
   180016fb5:	5b                   	pop    rbx
   180016fb6:	5d                   	pop    rbp
   180016fb7:	c3                   	ret
   180016fb8:	4c 8d 05 01 8a 60 00 	lea    r8,[rip+0x608a01]        # 0x18061f9c0 ; 'evaluate_reflected_color'
   180016fbf:	48 8b 57 08          	mov    rdx,QWORD PTR [rdi+0x8]
   180016fc3:	48 8d 0d 7e 18 62 00 	lea    rcx,[rip+0x62187e]        # 0x180638848 ; 'Effect "%s" could not find technique "%s"'
   180016fca:	ff 15 28 a3 5c 00    	call   QWORD PTR [rip+0x5ca328]        # 0x1805e12f8 ; ?str@r@@YAPEBDPEBDZZ
   180016fd0:	48 8b d0             	mov    rdx,rax
   180016fd3:	48 8d 4d e0          	lea    rcx,[rbp-0x20]
   180016fd7:	e8 94 36 0b 00       	call   0x1800ca670
   180016fdc:	90                   	nop
   180016fdd:	48 8b c8             	mov    rcx,rax
   180016fe0:	e8 db af 0b 00       	call   0x1800d1fc0
   180016fe5:	90                   	nop
   180016fe6:	4c 8d 05 33 89 60 00 	lea    r8,[rip+0x608933]        # 0x18061f920 ; 'temporal_feedback'
   180016fed:	48 8b 57 08          	mov    rdx,QWORD PTR [rdi+0x8]
   180016ff1:	48 8d 0d 50 18 62 00 	lea    rcx,[rip+0x621850]        # 0x180638848 ; 'Effect "%s" could not find technique "%s"'
   180016ff8:	ff 15 fa a2 5c 00    	call   QWORD PTR [rip+0x5ca2fa]        # 0x1805e12f8 ; ?str@r@@YAPEBDPEBDZZ
   180016ffe:	48 8b d0             	mov    rdx,rax
   180017001:	48 8d 4d e0          	lea    rcx,[rbp-0x20]
   180017005:	e8 66 36 0b 00       	call   0x1800ca670
   18001700a:	90                   	nop
   18001700b:	48 8b c8             	mov    rcx,rax
   18001700e:	e8 ad af 0b 00       	call   0x1800d1fc0
   180017013:	90                   	nop
   180017014:	4c 8d 05 1d 89 60 00 	lea    r8,[rip+0x60891d]        # 0x18061f938 ; 'filter_color'
   18001701b:	49 8b 55 08          	mov    rdx,QWORD PTR [r13+0x8]
   18001701f:	48 8d 0d 22 18 62 00 	lea    rcx,[rip+0x621822]        # 0x180638848 ; 'Effect "%s" could not find technique "%s"'
   180017026:	ff 15 cc a2 5c 00    	call   QWORD PTR [rip+0x5ca2cc]        # 0x1805e12f8 ; ?str@r@@YAPEBDPEBDZZ
   18001702c:	48 8b d0             	mov    rdx,rax
   18001702f:	48 8d 4d e0          	lea    rcx,[rbp-0x20]
   180017033:	e8 38 36 0b 00       	call   0x1800ca670
   180017038:	90                   	nop
   180017039:	48 8b c8             	mov    rcx,rax
   18001703c:	e8 7f af 0b 00       	call   0x1800d1fc0
   180017041:	cc                   	int3
