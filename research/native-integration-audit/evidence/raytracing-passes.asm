
upload/renderer_rmdwin10_f(4).dll:     file format pei-x86-64


Disassembly of section .text:

0000000180128d20 <.text+0x127d20>:
   180128d20:	48 8b c4             	mov    rax,rsp
   180128d23:	48 89 48 08          	mov    QWORD PTR [rax+0x8],rcx
   180128d27:	55                   	push   rbp
   180128d28:	56                   	push   rsi
   180128d29:	57                   	push   rdi
   180128d2a:	41 54                	push   r12
   180128d2c:	41 55                	push   r13
   180128d2e:	41 56                	push   r14
   180128d30:	41 57                	push   r15
   180128d32:	48 8d a8 18 fc ff ff 	lea    rbp,[rax-0x3e8]
   180128d39:	48 81 ec b0 04 00 00 	sub    rsp,0x4b0
   180128d40:	48 c7 85 f8 02 00 00 	mov    QWORD PTR [rbp+0x2f8],0xfffffffffffffffe
   180128d47:	fe ff ff ff 
   180128d4b:	48 89 58 10          	mov    QWORD PTR [rax+0x10],rbx
   180128d4f:	0f 29 70 b8          	movaps XMMWORD PTR [rax-0x48],xmm6
   180128d53:	0f 29 78 a8          	movaps XMMWORD PTR [rax-0x58],xmm7
   180128d57:	44 0f 29 40 98       	movaps XMMWORD PTR [rax-0x68],xmm8
   180128d5c:	44 0f 29 48 88       	movaps XMMWORD PTR [rax-0x78],xmm9
   180128d61:	44 0f 29 90 78 ff ff 	movaps XMMWORD PTR [rax-0x88],xmm10
   180128d68:	ff 
   180128d69:	44 0f 29 98 68 ff ff 	movaps XMMWORD PTR [rax-0x98],xmm11
   180128d70:	ff 
   180128d71:	44 0f 29 a0 58 ff ff 	movaps XMMWORD PTR [rax-0xa8],xmm12
   180128d78:	ff 
   180128d79:	44 0f 29 a8 48 ff ff 	movaps XMMWORD PTR [rax-0xb8],xmm13
   180128d80:	ff 
   180128d81:	44 0f 29 b0 38 ff ff 	movaps XMMWORD PTR [rax-0xc8],xmm14
   180128d88:	ff 
   180128d89:	44 0f 29 b8 28 ff ff 	movaps XMMWORD PTR [rax-0xd8],xmm15
   180128d90:	ff 
   180128d91:	48 8b da             	mov    rbx,rdx
   180128d94:	4c 8b f9             	mov    r15,rcx
   180128d97:	48 8d 0d 7a 6e 50 00 	lea    rcx,[rip+0x506e7a]        # 0x18062fc18 ; 'previouslineardepth'
   180128d9e:	e8 cd 31 01 00       	call   0x18013bf70
   180128da3:	48 8b d0             	mov    rdx,rax
   180128da6:	48 8b cb             	mov    rcx,rbx
   180128da9:	e8 d2 85 f7 ff       	call   0x1800a1380
   180128dae:	48 8b c8             	mov    rcx,rax
   180128db1:	e8 ba 21 ff ff       	call   0x18011af70
   180128db6:	48 8b 0d 03 9c 6d 00 	mov    rcx,QWORD PTR [rip+0x6d9c03]        # 0x1808029c0
   180128dbd:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   180128dc0:	ff 50 28             	call   QWORD PTR [rax+0x28]
   180128dc3:	48 8b c8             	mov    rcx,rax
   180128dc6:	ff 15 ac 71 4b 00    	call   QWORD PTR [rip+0x4b71ac]        # 0x1805dff78 ; ?prepareForConsuming@NativeTexture@d3d@@QEAAXXZ
   180128dcc:	80 3d 85 33 6c 00 00 	cmp    BYTE PTR [rip+0x6c3385],0x0        # 0x1807ec158
   180128dd3:	74 0f                	je     0x180128de4
   180128dd5:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   180128dd8:	48 8b 89 00 02 00 00 	mov    rcx,QWORD PTR [rcx+0x200]
   180128ddf:	e8 0c 84 f2 ff       	call   0x1800511f0
   180128de4:	8b 15 a2 56 7e 00    	mov    edx,DWORD PTR [rip+0x7e56a2]        # 0x18090e48c
   180128dea:	8b 0d 98 56 7e 00    	mov    ecx,DWORD PTR [rip+0x7e5698]        # 0x18090e488
   180128df0:	e8 2b 5e 0e 00       	call   0x18020ec20
   180128df5:	0f b6 05 51 33 6c 00 	movzx  eax,BYTE PTR [rip+0x6c3351]        # 0x1807ec14d
   180128dfc:	88 45 80             	mov    BYTE PTR [rbp-0x80],al
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
   180128e2d:	80 3d ef 9a 6d 00 00 	cmp    BYTE PTR [rip+0x6d9aef],0x0        # 0x180802923
   180128e34:	c6 45 81 01          	mov    BYTE PTR [rbp-0x7f],0x1
   180128e38:	74 04                	je     0x180128e3e
   180128e3a:	c6 45 81 00          	mov    BYTE PTR [rbp-0x7f],0x0
   180128e3e:	e8 4d 49 f8 ff       	call   0x1800ad790
   180128e43:	88 45 82             	mov    BYTE PTR [rbp-0x7e],al
   180128e46:	f3 0f 10 05 22 c8 7e 	movss  xmm0,DWORD PTR [rip+0x7ec822]        # 0x180915670
   180128e4d:	00 
   180128e4e:	f3 0f 10 35 62 60 55 	movss  xmm6,DWORD PTR [rip+0x556062]        # 0x18067eeb8
   180128e55:	00 
   180128e56:	0f 2f c6             	comiss xmm0,xmm6
   180128e59:	76 0d                	jbe    0x180128e68
   180128e5b:	f3 0f 10 05 0d cd 7e 	movss  xmm0,DWORD PTR [rip+0x7ecd0d]        # 0x180915b70
   180128e62:	00 
   180128e63:	0f 2f c6             	comiss xmm0,xmm6
   180128e66:	77 06                	ja     0x180128e6e
   180128e68:	66 c7 45 80 00 00    	mov    WORD PTR [rbp-0x80],0x0
   180128e6e:	80 3d 93 9b 6d 00 00 	cmp    BYTE PTR [rip+0x6d9b93],0x0        # 0x180802a08
   180128e75:	74 08                	je     0x180128e7f
   180128e77:	c6 45 80 00          	mov    BYTE PTR [rbp-0x80],0x0
   180128e7b:	c6 45 82 00          	mov    BYTE PTR [rbp-0x7e],0x0
   180128e7f:	45 33 e4             	xor    r12d,r12d
   180128e82:	41 8b fc             	mov    edi,r12d
   180128e85:	44 39 25 40 43 16 01 	cmp    DWORD PTR [rip+0x1164340],r12d        # 0x18128d1cc
   180128e8c:	7e 1a                	jle    0x180128ea8
   180128e8e:	48 8d 0d d3 6d 50 00 	lea    rcx,[rip+0x506dd3]        # 0x18062fc68 ; 'lineardepthhalfres'
   180128e95:	e8 c6 14 01 00       	call   0x18013a360
   180128e9a:	48 8b d0             	mov    rdx,rax
   180128e9d:	48 8b cb             	mov    rcx,rbx
   180128ea0:	e8 db 84 f7 ff       	call   0x1800a1380
   180128ea5:	48 8b f8             	mov    rdi,rax
   180128ea8:	48 8d 0d e1 6f 50 00 	lea    rcx,[rip+0x506fe1]        # 0x18062fe90 ; 'lastcolorbuffer'
   180128eaf:	e8 9c 32 01 00       	call   0x18013c150
   180128eb4:	48 8b d0             	mov    rdx,rax
   180128eb7:	48 8b cb             	mov    rcx,rbx
   180128eba:	e8 c1 84 f7 ff       	call   0x1800a1380
   180128ebf:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   180128ec2:	4c 8d 4d 80          	lea    r9,[rbp-0x80]
   180128ec6:	4c 8b c7             	mov    r8,rdi
   180128ec9:	48 8b d0             	mov    rdx,rax
   180128ecc:	48 8b 89 d8 00 00 00 	mov    rcx,QWORD PTR [rcx+0xd8]
   180128ed3:	e8 88 cf 11 00       	call   0x180245e60
   180128ed8:	44 38 25 29 9b 6d 00 	cmp    BYTE PTR [rip+0x6d9b29],r12b        # 0x180802a08
   180128edf:	75 28                	jne    0x180128f09
   180128ee1:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   180128ee4:	48 8b 91 d8 00 00 00 	mov    rdx,QWORD PTR [rcx+0xd8]
   180128eeb:	4c 8b 8a 80 00 00 00 	mov    r9,QWORD PTR [rdx+0x80]
   180128ef2:	4c 8b 42 78          	mov    r8,QWORD PTR [rdx+0x78]
   180128ef6:	48 8b 92 28 01 00 00 	mov    rdx,QWORD PTR [rdx+0x128]
   180128efd:	48 8b 89 e0 01 00 00 	mov    rcx,QWORD PTR [rcx+0x1e0]
   180128f04:	e8 f7 48 f8 ff       	call   0x1800ad800
   180128f09:	ff 15 a1 6d 4b 00    	call   QWORD PTR [rip+0x4b6da1]        # 0x1805dfcb0 ; ?retrieveForThread@DeviceState@d3d@@SAAEAV12@XZ
   180128f0f:	4c 8b e8             	mov    r13,rax
   180128f12:	48 89 85 48 01 00 00 	mov    QWORD PTR [rbp+0x148],rax
   180128f19:	44 38 25 eb 9a 6d 00 	cmp    BYTE PTR [rip+0x6d9aeb],r12b        # 0x180802a0b
   180128f20:	74 4d                	je     0x180128f6f
   180128f22:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   180128f25:	48 8b 89 48 02 00 00 	mov    rcx,QWORD PTR [rcx+0x248]
   180128f2c:	e8 df b0 f7 ff       	call   0x1800a4010
   180128f31:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   180128f34:	48 8b 89 20 02 00 00 	mov    rcx,QWORD PTR [rcx+0x220]
   180128f3b:	e8 b0 fc f7 ff       	call   0x1800a8bf0
   180128f40:	8b 15 46 55 7e 00    	mov    edx,DWORD PTR [rip+0x7e5546]        # 0x18090e48c
   180128f46:	8b 0d 3c 55 7e 00    	mov    ecx,DWORD PTR [rip+0x7e553c]        # 0x18090e488
   180128f4c:	e8 cf 5c 0e 00       	call   0x18020ec20
   180128f51:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   180128f54:	48 8b 89 20 02 00 00 	mov    rcx,QWORD PTR [rcx+0x220]
   180128f5b:	e8 20 f1 f7 ff       	call   0x1800a8080
   180128f60:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   180128f63:	48 8b 89 48 02 00 00 	mov    rcx,QWORD PTR [rcx+0x248]
   180128f6a:	e8 01 b5 f7 ff       	call   0x1800a4470
   180128f6f:	f3 0f 10 3d c9 a2 7e 	movss  xmm7,DWORD PTR [rip+0x7ea2c9]        # 0x180913240
   180128f76:	00 
   180128f77:	f3 0f 11 bd 40 01 00 	movss  DWORD PTR [rbp+0x140],xmm7
   180128f7e:	00 
   180128f7f:	f3 0f 10 0d 79 a3 7e 	movss  xmm1,DWORD PTR [rip+0x7ea379]        # 0x180913300
   180128f86:	00 
   180128f87:	45 0f 57 c0          	xorps  xmm8,xmm8
   180128f8b:	41 0f 2f c8          	comiss xmm1,xmm8
   180128f8f:	76 39                	jbe    0x180128fca
   180128f91:	f2 0f 10 05 2f 9a 6d 	movsd  xmm0,QWORD PTR [rip+0x6d9a2f]        # 0x1808029c8
   180128f98:	00 
   180128f99:	66 0f 5a c0          	cvtpd2ps xmm0,xmm0
   180128f9d:	f3 0f 59 c1          	mulss  xmm0,xmm1
   180128fa1:	f3 0f 59 05 37 62 55 	mulss  xmm0,DWORD PTR [rip+0x556237]        # 0x18067f1e0
   180128fa8:	00 
   180128fa9:	f3 0f 59 05 13 61 55 	mulss  xmm0,DWORD PTR [rip+0x556113]        # 0x18067f0c4
   180128fb0:	00 
   180128fb1:	e8 a0 fe 45 00       	call   0x180588e56
   180128fb6:	f3 0f 59 c6          	mulss  xmm0,xmm6
   180128fba:	f3 0f 58 c6          	addss  xmm0,xmm6
   180128fbe:	f3 0f 59 f8          	mulss  xmm7,xmm0
   180128fc2:	f3 0f 11 bd 40 01 00 	movss  DWORD PTR [rbp+0x140],xmm7
   180128fc9:	00 
   180128fca:	41 b8 02 00 00 00    	mov    r8d,0x2
   180128fd0:	bb 40 00 00 00       	mov    ebx,0x40
   180128fd5:	f3 44 0f 10 0d 36 5d 	movss  xmm9,DWORD PTR [rip+0x555d36]        # 0x18067ed14
   180128fdc:	55 00 
   180128fde:	f3 44 0f 10 15 c9 61 	movss  xmm10,DWORD PTR [rip+0x5561c9]        # 0x18067f1b0
   180128fe5:	55 00 
   180128fe7:	44 38 25 1a 9a 6d 00 	cmp    BYTE PTR [rip+0x6d9a1a],r12b        # 0x180802a08
   180128fee:	0f 84 fd 28 00 00    	je     0x18012b8f1
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
   180129034:	49 8b 07             	mov    rax,QWORD PTR [r15]
   180129037:	48 8b 80 d8 00 00 00 	mov    rax,QWORD PTR [rax+0xd8]
   18012903e:	4c 8b a0 28 01 00 00 	mov    r12,QWORD PTR [rax+0x128]
   180129045:	4c 89 a5 c8 01 00 00 	mov    QWORD PTR [rbp+0x1c8],r12
   18012904c:	48 8b ce             	mov    rcx,rsi
   18012904f:	65 48 8b 04 25 58 00 	mov    rax,QWORD PTR gs:0x58
   180129056:	00 00 
   180129058:	8b f3                	mov    esi,ebx
   18012905a:	4c 8b 34 c8          	mov    r14,QWORD PTR [rax+rcx*8]
   18012905e:	41 8b 04 1e          	mov    eax,DWORD PTR [r14+rbx*1]
   180129062:	39 05 ec d6 16 01    	cmp    DWORD PTR [rip+0x116d6ec],eax        # 0x181296754
   180129068:	7e 44                	jle    0x1801290ae
   18012906a:	48 8d 0d e3 d6 16 01 	lea    rcx,[rip+0x116d6e3]        # 0x181296754
   180129071:	e8 ee ed 45 00       	call   0x180587e64
   180129076:	83 3d d7 d6 16 01 ff 	cmp    DWORD PTR [rip+0x116d6d7],0xffffffff        # 0x181296754
   18012907d:	75 2f                	jne    0x1801290ae
   18012907f:	45 33 c0             	xor    r8d,r8d
   180129082:	48 8d 15 df 6d 50 00 	lea    rdx,[rip+0x506ddf]        # 0x18062fe68 ; 'g_fReflectionSunShadowKernelSize'
   180129089:	48 8d 0d c8 d6 16 01 	lea    rcx,[rip+0x116d6c8]        # 0x181296758
   180129090:	e8 6b a4 ee ff       	call   0x180013500
   180129095:	48 8d 0d 14 58 4a 00 	lea    rcx,[rip+0x4a5814]        # 0x1805ce8b0
   18012909c:	e8 23 eb 45 00       	call   0x180587bc4
   1801290a1:	90                   	nop
   1801290a2:	48 8d 0d ab d6 16 01 	lea    rcx,[rip+0x116d6ab]        # 0x181296754
   1801290a9:	e8 56 ed 45 00       	call   0x180587e04
   1801290ae:	0f 28 c7             	movaps xmm0,xmm7
   1801290b1:	f3 41 0f 58 c1       	addss  xmm0,xmm9
   1801290b6:	f3 0f 11 85 00 04 00 	movss  DWORD PTR [rbp+0x400],xmm0
   1801290bd:	00 
   1801290be:	45 33 c0             	xor    r8d,r8d
   1801290c1:	48 8d 95 00 04 00 00 	lea    rdx,[rbp+0x400]
   1801290c8:	8b 0d 92 d6 16 01    	mov    ecx,DWORD PTR [rip+0x116d692]        # 0x181296760
   1801290ce:	ff 15 bc 6e 4b 00    	call   QWORD PTR [rip+0x4b6ebc]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   1801290d4:	f3 0f 10 85 00 04 00 	movss  xmm0,DWORD PTR [rbp+0x400]
   1801290db:	00 
   1801290dc:	f3 0f 11 05 84 d6 16 	movss  DWORD PTR [rip+0x116d684],xmm0        # 0x181296768
   1801290e3:	01 
   1801290e4:	41 8b 04 1e          	mov    eax,DWORD PTR [r14+rbx*1]
   1801290e8:	39 05 82 d6 16 01    	cmp    DWORD PTR [rip+0x116d682],eax        # 0x181296770
   1801290ee:	7e 47                	jle    0x180129137
   1801290f0:	48 8d 0d 79 d6 16 01 	lea    rcx,[rip+0x116d679]        # 0x181296770
   1801290f7:	e8 68 ed 45 00       	call   0x180587e64
   1801290fc:	83 3d 6d d6 16 01 ff 	cmp    DWORD PTR [rip+0x116d66d],0xffffffff        # 0x181296770
   180129103:	75 32                	jne    0x180129137
   180129105:	41 b8 09 00 00 00    	mov    r8d,0x9
   18012910b:	48 8d 15 ae 6d 50 00 	lea    rdx,[rip+0x506dae]        # 0x18062fec0 ; 'g_rwtReflectionTarget'
   180129112:	48 8d 0d 5f d6 16 01 	lea    rcx,[rip+0x116d65f]        # 0x181296778
   180129119:	e8 52 7c ee ff       	call   0x180010d70
   18012911e:	48 8d 0d 5b 57 4a 00 	lea    rcx,[rip+0x4a575b]        # 0x1805ce880
   180129125:	e8 9a ea 45 00       	call   0x180587bc4
   18012912a:	90                   	nop
   18012912b:	48 8d 0d 3e d6 16 01 	lea    rcx,[rip+0x116d63e]        # 0x181296770
   180129132:	e8 cd ec 45 00       	call   0x180587e04
   180129137:	4d 85 e4             	test   r12,r12
   18012913a:	74 0e                	je     0x18012914a
   18012913c:	49 8b cc             	mov    rcx,r12
   18012913f:	ff 15 eb 6b 4b 00    	call   QWORD PTR [rip+0x4b6beb]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   180129145:	48 8b d8             	mov    rbx,rax
   180129148:	eb 02                	jmp    0x18012914c
   18012914a:	33 db                	xor    ebx,ebx
   18012914c:	8b 0d 2e d6 16 01    	mov    ecx,DWORD PTR [rip+0x116d62e]        # 0x181296780
   180129152:	ff 15 30 6b 4b 00    	call   QWORD PTR [rip+0x4b6b30]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   180129158:	8b d0                	mov    edx,eax
   18012915a:	4c 8b c3             	mov    r8,rbx
   18012915d:	48 8d 8d 50 01 00 00 	lea    rcx,[rbp+0x150]
   180129164:	ff 15 16 6b 4b 00    	call   QWORD PTR [rip+0x4b6b16]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   18012916a:	45 33 c0             	xor    r8d,r8d
   18012916d:	48 8d 95 50 01 00 00 	lea    rdx,[rbp+0x150]
   180129174:	8b 0d 06 d6 16 01    	mov    ecx,DWORD PTR [rip+0x116d606]        # 0x181296780
   18012917a:	ff 15 10 6e 4b 00    	call   QWORD PTR [rip+0x4b6e10]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   180129180:	8b 0d fa d5 16 01    	mov    ecx,DWORD PTR [rip+0x116d5fa]        # 0x181296780
   180129186:	ff 15 fc 6a 4b 00    	call   QWORD PTR [rip+0x4b6afc]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   18012918c:	8b d0                	mov    edx,eax
   18012918e:	4c 8b c3             	mov    r8,rbx
   180129191:	48 8d 0d f0 d5 16 01 	lea    rcx,[rip+0x116d5f0]        # 0x181296788
   180129198:	ff 15 f2 6a 4b 00    	call   QWORD PTR [rip+0x4b6af2]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   18012919e:	41 8b 04 36          	mov    eax,DWORD PTR [r14+rsi*1]
   1801291a2:	39 05 f0 d5 16 01    	cmp    DWORD PTR [rip+0x116d5f0],eax        # 0x181296798
   1801291a8:	7e 44                	jle    0x1801291ee
   1801291aa:	48 8d 0d e7 d5 16 01 	lea    rcx,[rip+0x116d5e7]        # 0x181296798
   1801291b1:	e8 ae ec 45 00       	call   0x180587e64
   1801291b6:	83 3d db d5 16 01 ff 	cmp    DWORD PTR [rip+0x116d5db],0xffffffff        # 0x181296798
   1801291bd:	75 2f                	jne    0x1801291ee
   1801291bf:	45 33 c0             	xor    r8d,r8d
   1801291c2:	48 8d 15 d7 6c 50 00 	lea    rdx,[rip+0x506cd7]        # 0x18062fea0 ; 'g_fClampReflectionIntensity'
   1801291c9:	48 8d 0d d0 d5 16 01 	lea    rcx,[rip+0x116d5d0]        # 0x1812967a0
   1801291d0:	e8 2b a3 ee ff       	call   0x180013500
   1801291d5:	48 8d 0d 74 56 4a 00 	lea    rcx,[rip+0x4a5674]        # 0x1805ce850
   1801291dc:	e8 e3 e9 45 00       	call   0x180587bc4
   1801291e1:	90                   	nop
   1801291e2:	48 8d 0d af d5 16 01 	lea    rcx,[rip+0x116d5af]        # 0x181296798
   1801291e9:	e8 16 ec 45 00       	call   0x180587e04
   1801291ee:	f3 0f 10 05 0a ad 7e 	movss  xmm0,DWORD PTR [rip+0x7ead0a]        # 0x180913f00
   1801291f5:	00 
   1801291f6:	f3 0f 11 85 00 04 00 	movss  DWORD PTR [rbp+0x400],xmm0
   1801291fd:	00 
   1801291fe:	45 33 c0             	xor    r8d,r8d
   180129201:	48 8d 95 00 04 00 00 	lea    rdx,[rbp+0x400]
   180129208:	8b 0d 9a d5 16 01    	mov    ecx,DWORD PTR [rip+0x116d59a]        # 0x1812967a8
   18012920e:	ff 15 7c 6d 4b 00    	call   QWORD PTR [rip+0x4b6d7c]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   180129214:	f3 0f 10 85 00 04 00 	movss  xmm0,DWORD PTR [rbp+0x400]
   18012921b:	00 
   18012921c:	f3 0f 11 05 8c d5 16 	movss  DWORD PTR [rip+0x116d58c],xmm0        # 0x1812967b0
   180129223:	01 
   180129224:	41 8b 04 36          	mov    eax,DWORD PTR [r14+rsi*1]
   180129228:	39 05 8a d5 16 01    	cmp    DWORD PTR [rip+0x116d58a],eax        # 0x1812967b8
   18012922e:	7e 44                	jle    0x180129274
   180129230:	48 8d 0d 81 d5 16 01 	lea    rcx,[rip+0x116d581]        # 0x1812967b8
   180129237:	e8 28 ec 45 00       	call   0x180587e64
   18012923c:	83 3d 75 d5 16 01 ff 	cmp    DWORD PTR [rip+0x116d575],0xffffffff        # 0x1812967b8
   180129243:	75 2f                	jne    0x180129274
   180129245:	45 33 c0             	xor    r8d,r8d
   180129248:	48 8d 15 a1 6c 50 00 	lea    rdx,[rip+0x506ca1]        # 0x18062fef0 ; 'g_fRTReflectionAddRays'
   18012924f:	48 8d 0d 6a d5 16 01 	lea    rcx,[rip+0x116d56a]        # 0x1812967c0
   180129256:	e8 a5 a2 ee ff       	call   0x180013500
   18012925b:	48 8d 0d be 55 4a 00 	lea    rcx,[rip+0x4a55be]        # 0x1805ce820
   180129262:	e8 5d e9 45 00       	call   0x180587bc4
   180129267:	90                   	nop
   180129268:	48 8d 0d 49 d5 16 01 	lea    rcx,[rip+0x116d549]        # 0x1812967b8
   18012926f:	e8 90 eb 45 00       	call   0x180587e04
   180129274:	f3 0f 10 05 44 ad 7e 	movss  xmm0,DWORD PTR [rip+0x7ead44]        # 0x180913fc0
   18012927b:	00 
   18012927c:	f3 0f 11 85 00 04 00 	movss  DWORD PTR [rbp+0x400],xmm0
   180129283:	00 
   180129284:	45 33 c0             	xor    r8d,r8d
   180129287:	48 8d 95 00 04 00 00 	lea    rdx,[rbp+0x400]
   18012928e:	8b 0d 34 d5 16 01    	mov    ecx,DWORD PTR [rip+0x116d534]        # 0x1812967c8
   180129294:	ff 15 f6 6c 4b 00    	call   QWORD PTR [rip+0x4b6cf6]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   18012929a:	f3 0f 10 85 00 04 00 	movss  xmm0,DWORD PTR [rbp+0x400]
   1801292a1:	00 
   1801292a2:	f3 0f 11 05 26 d5 16 	movss  DWORD PTR [rip+0x116d526],xmm0        # 0x1812967d0
   1801292a9:	01 
   1801292aa:	41 8b 04 36          	mov    eax,DWORD PTR [r14+rsi*1]
   1801292ae:	39 05 24 d5 16 01    	cmp    DWORD PTR [rip+0x116d524],eax        # 0x1812967d8
   1801292b4:	7e 47                	jle    0x1801292fd
   1801292b6:	48 8d 0d 1b d5 16 01 	lea    rcx,[rip+0x116d51b]        # 0x1812967d8
   1801292bd:	e8 a2 eb 45 00       	call   0x180587e64
   1801292c2:	83 3d 0f d5 16 01 ff 	cmp    DWORD PTR [rip+0x116d50f],0xffffffff        # 0x1812967d8
   1801292c9:	75 32                	jne    0x1801292fd
   1801292cb:	41 b8 14 00 00 00    	mov    r8d,0x14
   1801292d1:	48 8d 15 00 6c 50 00 	lea    rdx,[rip+0x506c00]        # 0x18062fed8 ; 'g_uRTReflectionRayCount'
   1801292d8:	48 8d 0d 01 d5 16 01 	lea    rcx,[rip+0x116d501]        # 0x1812967e0
   1801292df:	e8 1c a2 ee ff       	call   0x180013500
   1801292e4:	48 8d 0d 05 55 4a 00 	lea    rcx,[rip+0x4a5505]        # 0x1805ce7f0
   1801292eb:	e8 d4 e8 45 00       	call   0x180587bc4
   1801292f0:	90                   	nop
   1801292f1:	48 8d 0d e0 d4 16 01 	lea    rcx,[rip+0x116d4e0]        # 0x1812967d8
   1801292f8:	e8 07 eb 45 00       	call   0x180587e04
   1801292fd:	45 33 c0             	xor    r8d,r8d
   180129300:	48 8d 95 00 01 00 00 	lea    rdx,[rbp+0x100]
   180129307:	8b 0d db d4 16 01    	mov    ecx,DWORD PTR [rip+0x116d4db]        # 0x1812967e8
   18012930d:	ff 15 7d 6c 4b 00    	call   QWORD PTR [rip+0x4b6c7d]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   180129313:	8b 85 00 01 00 00    	mov    eax,DWORD PTR [rbp+0x100]
   180129319:	89 05 d1 d4 16 01    	mov    DWORD PTR [rip+0x116d4d1],eax        # 0x1812967f0
   18012931f:	41 8b 04 36          	mov    eax,DWORD PTR [r14+rsi*1]
   180129323:	39 05 cf d4 16 01    	cmp    DWORD PTR [rip+0x116d4cf],eax        # 0x1812967f8
   180129329:	7e 44                	jle    0x18012936f
   18012932b:	48 8d 0d c6 d4 16 01 	lea    rcx,[rip+0x116d4c6]        # 0x1812967f8
   180129332:	e8 2d eb 45 00       	call   0x180587e64
   180129337:	83 3d ba d4 16 01 ff 	cmp    DWORD PTR [rip+0x116d4ba],0xffffffff        # 0x1812967f8
   18012933e:	75 2f                	jne    0x18012936f
   180129340:	45 33 c0             	xor    r8d,r8d
   180129343:	48 8d 15 d6 6b 50 00 	lea    rdx,[rip+0x506bd6]        # 0x18062ff20 ; 'g_fRTZeroRayReflectance'
   18012934a:	48 8d 0d af d4 16 01 	lea    rcx,[rip+0x116d4af]        # 0x181296800
   180129351:	e8 aa a1 ee ff       	call   0x180013500
   180129356:	48 8d 0d 63 54 4a 00 	lea    rcx,[rip+0x4a5463]        # 0x1805ce7c0
   18012935d:	e8 62 e8 45 00       	call   0x180587bc4
   180129362:	90                   	nop
   180129363:	48 8d 0d 8e d4 16 01 	lea    rcx,[rip+0x116d48e]        # 0x1812967f8
   18012936a:	e8 95 ea 45 00       	call   0x180587e04
   18012936f:	f3 0f 10 05 19 a6 7e 	movss  xmm0,DWORD PTR [rip+0x7ea619]        # 0x180913990
   180129376:	00 
   180129377:	f3 0f 11 85 00 04 00 	movss  DWORD PTR [rbp+0x400],xmm0
   18012937e:	00 
   18012937f:	45 33 c0             	xor    r8d,r8d
   180129382:	48 8d 95 00 04 00 00 	lea    rdx,[rbp+0x400]
   180129389:	8b 0d 79 d4 16 01    	mov    ecx,DWORD PTR [rip+0x116d479]        # 0x181296808
   18012938f:	ff 15 fb 6b 4b 00    	call   QWORD PTR [rip+0x4b6bfb]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   180129395:	f3 0f 10 85 00 04 00 	movss  xmm0,DWORD PTR [rbp+0x400]
   18012939c:	00 
   18012939d:	f3 0f 11 05 6b d4 16 	movss  DWORD PTR [rip+0x116d46b],xmm0        # 0x181296810
   1801293a4:	01 
   1801293a5:	41 8b 04 36          	mov    eax,DWORD PTR [r14+rsi*1]
   1801293a9:	39 05 69 d4 16 01    	cmp    DWORD PTR [rip+0x116d469],eax        # 0x181296818
   1801293af:	7e 44                	jle    0x1801293f5
   1801293b1:	48 8d 0d 60 d4 16 01 	lea    rcx,[rip+0x116d460]        # 0x181296818
   1801293b8:	e8 a7 ea 45 00       	call   0x180587e64
   1801293bd:	83 3d 54 d4 16 01 ff 	cmp    DWORD PTR [rip+0x116d454],0xffffffff        # 0x181296818
   1801293c4:	75 2f                	jne    0x1801293f5
   1801293c6:	45 33 c0             	xor    r8d,r8d
   1801293c9:	48 8d 15 38 6b 50 00 	lea    rdx,[rip+0x506b38]        # 0x18062ff08 ; 'g_fRTMaxRayReflectance'
   1801293d0:	48 8d 0d 49 d4 16 01 	lea    rcx,[rip+0x116d449]        # 0x181296820
   1801293d7:	e8 24 a1 ee ff       	call   0x180013500
   1801293dc:	48 8d 0d ad 53 4a 00 	lea    rcx,[rip+0x4a53ad]        # 0x1805ce790
   1801293e3:	e8 dc e7 45 00       	call   0x180587bc4
   1801293e8:	90                   	nop
   1801293e9:	48 8d 0d 28 d4 16 01 	lea    rcx,[rip+0x116d428]        # 0x181296818
   1801293f0:	e8 0f ea 45 00       	call   0x180587e04
   1801293f5:	f3 0f 10 05 53 a6 7e 	movss  xmm0,DWORD PTR [rip+0x7ea653]        # 0x180913a50
   1801293fc:	00 
   1801293fd:	f3 0f 11 85 00 04 00 	movss  DWORD PTR [rbp+0x400],xmm0
   180129404:	00 
   180129405:	45 33 c0             	xor    r8d,r8d
   180129408:	48 8d 95 00 04 00 00 	lea    rdx,[rbp+0x400]
   18012940f:	8b 0d 13 d4 16 01    	mov    ecx,DWORD PTR [rip+0x116d413]        # 0x181296828
   180129415:	ff 15 75 6b 4b 00    	call   QWORD PTR [rip+0x4b6b75]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   18012941b:	f3 0f 10 85 00 04 00 	movss  xmm0,DWORD PTR [rbp+0x400]
   180129422:	00 
   180129423:	f3 0f 11 05 05 d4 16 	movss  DWORD PTR [rip+0x116d405],xmm0        # 0x181296830
   18012942a:	01 
   18012942b:	41 8b 04 36          	mov    eax,DWORD PTR [r14+rsi*1]
   18012942f:	39 05 03 d4 16 01    	cmp    DWORD PTR [rip+0x116d403],eax        # 0x181296838
   180129435:	7e 44                	jle    0x18012947b
   180129437:	48 8d 0d fa d3 16 01 	lea    rcx,[rip+0x116d3fa]        # 0x181296838
   18012943e:	e8 21 ea 45 00       	call   0x180587e64
   180129443:	83 3d ee d3 16 01 ff 	cmp    DWORD PTR [rip+0x116d3ee],0xffffffff        # 0x181296838
   18012944a:	75 2f                	jne    0x18012947b
   18012944c:	45 33 c0             	xor    r8d,r8d
   18012944f:	48 8d 15 fa 6a 50 00 	lea    rdx,[rip+0x506afa]        # 0x18062ff50 ; 'g_fRTMaxRayRoughness'
   180129456:	48 8d 0d e3 d3 16 01 	lea    rcx,[rip+0x116d3e3]        # 0x181296840
   18012945d:	e8 9e a0 ee ff       	call   0x180013500
   180129462:	48 8d 0d f7 52 4a 00 	lea    rcx,[rip+0x4a52f7]        # 0x1805ce760
   180129469:	e8 56 e7 45 00       	call   0x180587bc4
   18012946e:	90                   	nop
   18012946f:	48 8d 0d c2 d3 16 01 	lea    rcx,[rip+0x116d3c2]        # 0x181296838
   180129476:	e8 89 e9 45 00       	call   0x180587e04
   18012947b:	f3 0f 10 05 8d a6 7e 	movss  xmm0,DWORD PTR [rip+0x7ea68d]        # 0x180913b10
   180129482:	00 
   180129483:	f3 0f 11 85 00 04 00 	movss  DWORD PTR [rbp+0x400],xmm0
   18012948a:	00 
   18012948b:	45 33 c0             	xor    r8d,r8d
   18012948e:	48 8d 95 00 04 00 00 	lea    rdx,[rbp+0x400]
   180129495:	8b 0d ad d3 16 01    	mov    ecx,DWORD PTR [rip+0x116d3ad]        # 0x181296848
   18012949b:	ff 15 ef 6a 4b 00    	call   QWORD PTR [rip+0x4b6aef]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   1801294a1:	f3 0f 10 85 00 04 00 	movss  xmm0,DWORD PTR [rbp+0x400]
   1801294a8:	00 
   1801294a9:	f3 0f 11 05 9f d3 16 	movss  DWORD PTR [rip+0x116d39f],xmm0        # 0x181296850
   1801294b0:	01 
   1801294b1:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   1801294b4:	48 8b 89 38 02 00 00 	mov    rcx,QWORD PTR [rcx+0x238]
   1801294bb:	e8 b0 9d f7 ff       	call   0x1800a3270
   1801294c0:	49 8b 07             	mov    rax,QWORD PTR [r15]
   1801294c3:	48 8b 88 f8 00 00 00 	mov    rcx,QWORD PTR [rax+0xf8]
   1801294ca:	33 ff                	xor    edi,edi
   1801294cc:	39 79 28             	cmp    DWORD PTR [rcx+0x28],edi
   1801294cf:	41 b8 02 00 00 00    	mov    r8d,0x2
   1801294d5:	41 0f 4f f8          	cmovg  edi,r8d
   1801294d9:	0f b6 05 90 ac 7e 00 	movzx  eax,BYTE PTR [rip+0x7eac90]        # 0x180914170
   1801294e0:	f6 d8                	neg    al
   1801294e2:	1b c9                	sbb    ecx,ecx
   1801294e4:	83 e1 04             	and    ecx,0x4
   1801294e7:	0b f9                	or     edi,ecx
   1801294e9:	0f b6 05 b0 a3 7e 00 	movzx  eax,BYTE PTR [rip+0x7ea3b0]        # 0x1809138a0
   1801294f0:	f6 d8                	neg    al
   1801294f2:	1b db                	sbb    ebx,ebx
   1801294f4:	83 e3 40             	and    ebx,0x40
   1801294f7:	33 c0                	xor    eax,eax
   1801294f9:	38 05 11 97 7e 00    	cmp    BYTE PTR [rip+0x7e9711],al        # 0x180912c10
   1801294ff:	0f 95 c0             	setne  al
   180129502:	0b d8                	or     ebx,eax
   180129504:	0f b6 05 75 ab 7e 00 	movzx  eax,BYTE PTR [rip+0x7eab75]        # 0x180914080
   18012950b:	f6 d8                	neg    al
   18012950d:	1b c9                	sbb    ecx,ecx
   18012950f:	83 e1 08             	and    ecx,0x8
   180129512:	0b d9                	or     ebx,ecx
   180129514:	0b df                	or     ebx,edi
   180129516:	41 8b d0             	mov    edx,r8d
   180129519:	41 8b c8             	mov    ecx,r8d
   18012951c:	ff 15 7e 5f 4b 00    	call   QWORD PTR [rip+0x4b5f7e]        # 0x1805df4a0 ; ?beginPipelineSetup@DeviceUtilRaytracing@d3d@@SAXHH@Z
   180129522:	48 8b 0d d7 43 16 01 	mov    rcx,QWORD PTR [rip+0x11643d7]        # 0x18128d900
   180129529:	48 85 c9             	test   rcx,rcx
   18012952c:	75 3c                	jne    0x18012956a
   18012952e:	48 8d 15 03 6a 50 00 	lea    rdx,[rip+0x506a03]        # 0x18062ff38 ; 'rt_reflection.rfx'
   180129535:	48 8b 0d 94 97 75 00 	mov    rcx,QWORD PTR [rip+0x759794]        # 0x180882cd0
   18012953c:	e8 1f 04 0b 00       	call   0x1801d9960
   180129541:	48 8d 15 38 6a 50 00 	lea    rdx,[rip+0x506a38]        # 0x18062ff80 ; 'rt_reflectionDeferredShading'
   180129548:	48 8b c8             	mov    rcx,rax
   18012954b:	e8 90 97 0a 00       	call   0x1801d2ce0
   180129550:	48 89 05 a9 43 16 01 	mov    QWORD PTR [rip+0x11643a9],rax        # 0x18128d900
   180129557:	48 8d 0d a2 43 16 01 	lea    rcx,[rip+0x11643a2]        # 0x18128d900
   18012955e:	e8 9d f1 0a 00       	call   0x1801d8700
   180129563:	48 8b 0d 96 43 16 01 	mov    rcx,QWORD PTR [rip+0x1164396]        # 0x18128d900
   18012956a:	8b d3                	mov    edx,ebx
   18012956c:	e8 df 3e 0b 00       	call   0x1801dd450
   180129571:	48 8b c8             	mov    rcx,rax
   180129574:	49 8b d5             	mov    rdx,r13
   180129577:	e8 94 22 0b 00       	call   0x1801db810
   18012957c:	48 8d 0d e5 69 50 00 	lea    rcx,[rip+0x5069e5]        # 0x18062ff68 ; 'reflectionRayGeneration'
   180129583:	ff 15 1f 5f 4b 00    	call   QWORD PTR [rip+0x4b5f1f]        # 0x1805df4a8 ; ?setRayGeneration@DeviceUtilRaytracing@d3d@@SAXPEBD@Z
   180129589:	48 8d 15 80 e6 4f 00 	lea    rdx,[rip+0x4fe680]        # 0x180627c10 ; 'reflectionMiss'
   180129590:	33 c9                	xor    ecx,ecx
   180129592:	ff 15 18 5f 4b 00    	call   QWORD PTR [rip+0x4b5f18]        # 0x1805df4b0 ; ?setMiss@DeviceUtilRaytracing@d3d@@SAXHPEBD@Z
   180129598:	48 8d 1d 81 e6 4f 00 	lea    rbx,[rip+0x4fe681]        # 0x180627c20 ; 'reflectionClosestHit'
   18012959f:	48 89 5c 24 20       	mov    QWORD PTR [rsp+0x20],rbx
   1801295a4:	45 33 c9             	xor    r9d,r9d
   1801295a7:	45 33 c0             	xor    r8d,r8d
   1801295aa:	33 d2                	xor    edx,edx
   1801295ac:	33 c9                	xor    ecx,ecx
   1801295ae:	ff 15 04 5f 4b 00    	call   QWORD PTR [rip+0x4b5f04]        # 0x1805df4b8 ; ?setHitGroup@DeviceUtilRaytracing@d3d@@SAXHHPEBD00@Z
   1801295b4:	48 89 5c 24 20       	mov    QWORD PTR [rsp+0x20],rbx
   1801295b9:	4c 8d 0d 78 e6 4f 00 	lea    r9,[rip+0x4fe678]        # 0x180627c38 ; 'reflectionAlphaTestAnyHit'
   1801295c0:	45 33 c0             	xor    r8d,r8d
   1801295c3:	41 8d 50 01          	lea    edx,[r8+0x1]
   1801295c7:	33 c9                	xor    ecx,ecx
   1801295c9:	ff 15 e9 5e 4b 00    	call   QWORD PTR [rip+0x4b5ee9]        # 0x1805df4b8 ; ?setHitGroup@DeviceUtilRaytracing@d3d@@SAXHHPEBD00@Z
   1801295cf:	48 8d 15 e2 69 50 00 	lea    rdx,[rip+0x5069e2]        # 0x18062ffb8 ; 'shadowMiss'
   1801295d6:	b9 01 00 00 00       	mov    ecx,0x1
   1801295db:	ff 15 cf 5e 4b 00    	call   QWORD PTR [rip+0x4b5ecf]        # 0x1805df4b0 ; ?setMiss@DeviceUtilRaytracing@d3d@@SAXHPEBD@Z
   1801295e1:	48 8d 1d b8 69 50 00 	lea    rbx,[rip+0x5069b8]        # 0x18062ffa0 ; 'shadowClosestHit'
   1801295e8:	48 89 5c 24 20       	mov    QWORD PTR [rsp+0x20],rbx
   1801295ed:	45 33 c9             	xor    r9d,r9d
   1801295f0:	45 33 c0             	xor    r8d,r8d
   1801295f3:	33 d2                	xor    edx,edx
   1801295f5:	8d 4a 01             	lea    ecx,[rdx+0x1]
   1801295f8:	ff 15 ba 5e 4b 00    	call   QWORD PTR [rip+0x4b5eba]        # 0x1805df4b8 ; ?setHitGroup@DeviceUtilRaytracing@d3d@@SAXHHPEBD00@Z
   1801295fe:	48 89 5c 24 20       	mov    QWORD PTR [rsp+0x20],rbx
   180129603:	4c 8d 0d de 69 50 00 	lea    r9,[rip+0x5069de]        # 0x18062ffe8 ; 'shadowAlphaTestAnyHit'
   18012960a:	45 33 c0             	xor    r8d,r8d
   18012960d:	ba 01 00 00 00       	mov    edx,0x1
   180129612:	8b ca                	mov    ecx,edx
   180129614:	ff 15 9e 5e 4b 00    	call   QWORD PTR [rip+0x4b5e9e]        # 0x1805df4b8 ; ?setHitGroup@DeviceUtilRaytracing@d3d@@SAXHHPEBD00@Z
   18012961a:	49 8b 07             	mov    rax,QWORD PTR [r15]
   18012961d:	48 8b 80 20 02 00 00 	mov    rax,QWORD PTR [rax+0x220]
   180129624:	8b 88 88 00 00 00    	mov    ecx,DWORD PTR [rax+0x88]
   18012962a:	89 4c 24 20          	mov    DWORD PTR [rsp+0x20],ecx
   18012962e:	4c 8b 88 80 00 00 00 	mov    r9,QWORD PTR [rax+0x80]
   180129635:	ba 01 00 00 00       	mov    edx,0x1
   18012963a:	8b ca                	mov    ecx,edx
   18012963c:	44 8d 42 01          	lea    r8d,[rdx+0x1]
   180129640:	ff 15 7a 5e 4b 00    	call   QWORD PTR [rip+0x4b5e7a]        # 0x1805df4c0 ; ?endPipelineSetup@DeviceUtilRaytracing@d3d@@SAXHHHPEBURaytracingGeometryBinding@2@H@Z
   180129646:	8b 15 40 4e 7e 00    	mov    edx,DWORD PTR [rip+0x7e4e40]        # 0x18090e48c
   18012964c:	8b 0d 36 4e 7e 00    	mov    ecx,DWORD PTR [rip+0x7e4e36]        # 0x18090e488
   180129652:	ff 15 70 5e 4b 00    	call   QWORD PTR [rip+0x4b5e70]        # 0x1805df4c8 ; ?raytrace@DeviceUtilRaytracing@d3d@@SAXHH@Z
   180129658:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012965b:	48 8b 89 38 02 00 00 	mov    rcx,QWORD PTR [rcx+0x238]
   180129662:	e8 d9 a2 f7 ff       	call   0x1800a3940
   180129667:	44 0f b6 2d 81 a0 7e 	movzx  r13d,BYTE PTR [rip+0x7ea081]        # 0x1809136f0
   18012966e:	00 
   18012966f:	44 88 ad 08 04 00 00 	mov    BYTE PTR [rbp+0x408],r13b
   180129676:	33 db                	xor    ebx,ebx
   180129678:	45 84 ed             	test   r13b,r13b
   18012967b:	0f 94 c3             	sete   bl
   18012967e:	0b df                	or     ebx,edi
   180129680:	8b 0d 06 4e 7e 00    	mov    ecx,DWORD PTR [rip+0x7e4e06]        # 0x18090e48c
   180129686:	83 c1 0f             	add    ecx,0xf
   180129689:	c1 e9 04             	shr    ecx,0x4
   18012968c:	8b 05 f6 4d 7e 00    	mov    eax,DWORD PTR [rip+0x7e4df6]        # 0x18090e488
   180129692:	83 c0 07             	add    eax,0x7
   180129695:	c1 e8 03             	shr    eax,0x3
   180129698:	89 45 90             	mov    DWORD PTR [rbp-0x70],eax
   18012969b:	89 4d 94             	mov    DWORD PTR [rbp-0x6c],ecx
   18012969e:	48 8b 0d 63 42 16 01 	mov    rcx,QWORD PTR [rip+0x1164263]        # 0x18128d908
   1801296a5:	48 85 c9             	test   rcx,rcx
   1801296a8:	75 3c                	jne    0x1801296e6
   1801296aa:	48 8d 15 17 69 50 00 	lea    rdx,[rip+0x506917]        # 0x18062ffc8 ; 'rt_deferred_reflection.rfx'
   1801296b1:	48 8b 0d 18 96 75 00 	mov    rcx,QWORD PTR [rip+0x759618]        # 0x180882cd0
   1801296b8:	e8 a3 02 0b 00       	call   0x1801d9960
   1801296bd:	48 8d 15 3c 69 50 00 	lea    rdx,[rip+0x50693c]        # 0x180630000 ; 'rt_deferredReflection'
   1801296c4:	48 8b c8             	mov    rcx,rax
   1801296c7:	e8 14 96 0a 00       	call   0x1801d2ce0
   1801296cc:	48 89 05 35 42 16 01 	mov    QWORD PTR [rip+0x1164235],rax        # 0x18128d908
   1801296d3:	48 8d 0d 2e 42 16 01 	lea    rcx,[rip+0x116422e]        # 0x18128d908
   1801296da:	e8 21 f0 0a 00       	call   0x1801d8700
   1801296df:	48 8b 0d 22 42 16 01 	mov    rcx,QWORD PTR [rip+0x1164222]        # 0x18128d908
   1801296e6:	8b d3                	mov    edx,ebx
   1801296e8:	e8 63 3d 0b 00       	call   0x1801dd450
   1801296ed:	48 8b c8             	mov    rcx,rax
   1801296f0:	e8 5b 20 0b 00       	call   0x1801db750
   1801296f5:	f2 0f 10 45 90       	movsd  xmm0,QWORD PTR [rbp-0x70]
   1801296fa:	f2 0f 11 45 90       	movsd  QWORD PTR [rbp-0x70],xmm0
   1801296ff:	c7 45 98 01 00 00 00 	mov    DWORD PTR [rbp-0x68],0x1
   180129706:	48 8d 55 90          	lea    rdx,[rbp-0x70]
   18012970a:	48 8b bd 48 01 00 00 	mov    rdi,QWORD PTR [rbp+0x148]
   180129711:	48 8b cf             	mov    rcx,rdi
   180129714:	ff 15 a6 65 4b 00    	call   QWORD PTR [rip+0x4b65a6]        # 0x1805dfcc0 ; ?dispatch@DeviceUtil@d3d@@SAXAEAVDeviceState@2@V?$Vector3Template@H@m@@@Z
   18012971a:	90                   	nop
   18012971b:	41 8b 04 36          	mov    eax,DWORD PTR [r14+rsi*1]
   18012971f:	39 05 33 d1 16 01    	cmp    DWORD PTR [rip+0x116d133],eax        # 0x181296858
   180129725:	7e 3b                	jle    0x180129762
   180129727:	48 8d 0d 2a d1 16 01 	lea    rcx,[rip+0x116d12a]        # 0x181296858
   18012972e:	e8 31 e7 45 00       	call   0x180587e64
   180129733:	83 3d 1e d1 16 01 ff 	cmp    DWORD PTR [rip+0x116d11e],0xffffffff        # 0x181296858
   18012973a:	75 26                	jne    0x180129762
   18012973c:	48 8d 0d 1d d1 16 01 	lea    rcx,[rip+0x116d11d]        # 0x181296860
   180129743:	ff 15 07 71 4b 00    	call   QWORD PTR [rip+0x4b7107]        # 0x1805e0850 ; ??0View@m@@QEAA@XZ
   180129749:	48 8d 0d 00 50 4a 00 	lea    rcx,[rip+0x4a5000]        # 0x1805ce750
   180129750:	e8 6f e4 45 00       	call   0x180587bc4
   180129755:	90                   	nop
   180129756:	48 8d 0d fb d0 16 01 	lea    rcx,[rip+0x116d0fb]        # 0x181296858
   18012975d:	e8 a2 e6 45 00       	call   0x180587e04
   180129762:	41 8b 04 36          	mov    eax,DWORD PTR [r14+rsi*1]
   180129766:	39 05 d4 d3 16 01    	cmp    DWORD PTR [rip+0x116d3d4],eax        # 0x181296b40
   18012976c:	7e 3a                	jle    0x1801297a8
   18012976e:	48 8d 0d cb d3 16 01 	lea    rcx,[rip+0x116d3cb]        # 0x181296b40
   180129775:	e8 ea e6 45 00       	call   0x180587e64
   18012977a:	83 3d bf d3 16 01 ff 	cmp    DWORD PTR [rip+0x116d3bf],0xffffffff        # 0x181296b40
   180129781:	75 25                	jne    0x1801297a8
   180129783:	48 8d 0d be d3 16 01 	lea    rcx,[rip+0x116d3be]        # 0x181296b48
   18012978a:	e8 a1 f4 00 00       	call   0x180138c30
   18012978f:	48 8d 0d 6a 4f 4a 00 	lea    rcx,[rip+0x4a4f6a]        # 0x1805ce700
   180129796:	e8 29 e4 45 00       	call   0x180587bc4
   18012979b:	90                   	nop
   18012979c:	48 8d 0d 9d d3 16 01 	lea    rcx,[rip+0x116d39d]        # 0x181296b40
   1801297a3:	e8 5c e6 45 00       	call   0x180587e04
   1801297a8:	48 8d 1d 61 60 4f 00 	lea    rbx,[rip+0x4f6061]        # 0x18061f810
   1801297af:	41 8b 04 36          	mov    eax,DWORD PTR [r14+rsi*1]
   1801297b3:	39 05 9f d3 16 01    	cmp    DWORD PTR [rip+0x116d39f],eax        # 0x181296b58
   1801297b9:	7e 55                	jle    0x180129810
   1801297bb:	48 8d 0d 96 d3 16 01 	lea    rcx,[rip+0x116d396]        # 0x181296b58
   1801297c2:	e8 9d e6 45 00       	call   0x180587e64
   1801297c7:	83 3d 8a d3 16 01 ff 	cmp    DWORD PTR [rip+0x116d38a],0xffffffff        # 0x181296b58
   1801297ce:	75 40                	jne    0x180129810
   1801297d0:	45 33 c0             	xor    r8d,r8d
   1801297d3:	48 8d 15 56 68 50 00 	lea    rdx,[rip+0x506856]        # 0x180630030 ; 'rtd:Snap Reflection State'
   1801297da:	48 8d 0d 7f d3 16 01 	lea    rcx,[rip+0x116d37f]        # 0x181296b60
   1801297e1:	ff 15 41 7b 4b 00    	call   QWORD PTR [rip+0x4b7b41]        # 0x1805e1328 ; ??0BaseTweakable@d@@QEAA@PEBDW4Type@01@@Z
   1801297e7:	48 89 1d 72 d3 16 01 	mov    QWORD PTR [rip+0x116d372],rbx        # 0x181296b60
   1801297ee:	66 c7 05 19 d4 16 01 	mov    WORD PTR [rip+0x116d419],0x0        # 0x181296c10
   1801297f5:	00 00 
   1801297f7:	48 8d 0d f2 4e 4a 00 	lea    rcx,[rip+0x4a4ef2]        # 0x1805ce6f0
   1801297fe:	e8 c1 e3 45 00       	call   0x180587bc4
   180129803:	90                   	nop
   180129804:	48 8d 0d 4d d3 16 01 	lea    rcx,[rip+0x116d34d]        # 0x181296b58
   18012980b:	e8 f4 e5 45 00       	call   0x180587e04
   180129810:	80 3d f9 d3 16 01 00 	cmp    BYTE PTR [rip+0x116d3f9],0x0        # 0x181296c10
   180129817:	0f 84 98 04 00 00    	je     0x180129cb5
   18012981d:	8b 05 65 4c 7e 00    	mov    eax,DWORD PTR [rip+0x7e4c65]        # 0x18090e488
   180129823:	0f af 05 62 4c 7e 00 	imul   eax,DWORD PTR [rip+0x7e4c62]        # 0x18090e48c
   18012982a:	48 63 c8             	movsxd rcx,eax
   18012982d:	48 8d 1c 89          	lea    rbx,[rcx+rcx*4]
   180129831:	48 c1 e3 03          	shl    rbx,0x3
   180129835:	41 8b 04 36          	mov    eax,DWORD PTR [r14+rsi*1]
   180129839:	39 05 d9 d3 16 01    	cmp    DWORD PTR [rip+0x116d3d9],eax        # 0x181296c18
   18012983f:	7e 38                	jle    0x180129879
   180129841:	48 8d 0d d0 d3 16 01 	lea    rcx,[rip+0x116d3d0]        # 0x181296c18
   180129848:	e8 17 e6 45 00       	call   0x180587e64
   18012984d:	83 3d c4 d3 16 01 ff 	cmp    DWORD PTR [rip+0x116d3c4],0xffffffff        # 0x181296c18
   180129854:	75 23                	jne    0x180129879
   180129856:	48 c7 05 bf d3 16 01 	mov    QWORD PTR [rip+0x116d3bf],0x0        # 0x181296c20
   18012985d:	00 00 00 00 
   180129861:	48 8d 0d 48 4e 4a 00 	lea    rcx,[rip+0x4a4e48]        # 0x1805ce6b0
   180129868:	e8 57 e3 45 00       	call   0x180587bc4
   18012986d:	48 8d 0d a4 d3 16 01 	lea    rcx,[rip+0x116d3a4]        # 0x181296c18
   180129874:	e8 8b e5 45 00       	call   0x180587e04
   180129879:	41 8b 04 36          	mov    eax,DWORD PTR [r14+rsi*1]
   18012987d:	39 05 a5 d3 16 01    	cmp    DWORD PTR [rip+0x116d3a5],eax        # 0x181296c28
   180129883:	7e 38                	jle    0x1801298bd
   180129885:	48 8d 0d 9c d3 16 01 	lea    rcx,[rip+0x116d39c]        # 0x181296c28
   18012988c:	e8 d3 e5 45 00       	call   0x180587e64
   180129891:	83 3d 90 d3 16 01 ff 	cmp    DWORD PTR [rip+0x116d390],0xffffffff        # 0x181296c28
   180129898:	75 23                	jne    0x1801298bd
   18012989a:	48 c7 05 8b d3 16 01 	mov    QWORD PTR [rip+0x116d38b],0x0        # 0x181296c30
   1801298a1:	00 00 00 00 
   1801298a5:	48 8d 0d c4 4d 4a 00 	lea    rcx,[rip+0x4a4dc4]        # 0x1805ce670
   1801298ac:	e8 13 e3 45 00       	call   0x180587bc4
   1801298b1:	48 8d 0d 70 d3 16 01 	lea    rcx,[rip+0x116d370]        # 0x181296c28
   1801298b8:	e8 47 e5 45 00       	call   0x180587e04
   1801298bd:	48 8b 0d 5c d3 16 01 	mov    rcx,QWORD PTR [rip+0x116d35c]        # 0x181296c20
   1801298c4:	48 85 c9             	test   rcx,rcx
   1801298c7:	74 1e                	je     0x1801298e7
   1801298c9:	ff 15 a9 65 4b 00    	call   QWORD PTR [rip+0x4b65a9]        # 0x1805dfe78 ; ?getSize@NativeBufferUtil@d3d@@SA_KPEBVNativeBuffer@2@@Z
   1801298cf:	48 3b c3             	cmp    rax,rbx
   1801298d2:	72 13                	jb     0x1801298e7
   1801298d4:	48 8b 0d 45 d3 16 01 	mov    rcx,QWORD PTR [rip+0x116d345]        # 0x181296c20
   1801298db:	ff 15 3f 66 4b 00    	call   QWORD PTR [rip+0x4b663f]        # 0x1805dff20 ; ?getStride@NativeBufferUtil@d3d@@SA_KPEBVNativeBuffer@2@@Z
   1801298e1:	48 83 f8 28          	cmp    rax,0x28
   1801298e5:	74 3a                	je     0x180129921
   1801298e7:	c6 85 00 04 00 00 00 	mov    BYTE PTR [rbp+0x400],0x0
   1801298ee:	33 d2                	xor    edx,edx
   1801298f0:	48 8d 8d 00 04 00 00 	lea    rcx,[rbp+0x400]
   1801298f7:	e8 44 95 ee ff       	call   0x180012e40
   1801298fc:	48 8b d0             	mov    rdx,rax
   1801298ff:	48 8d 0d 1a d3 16 01 	lea    rcx,[rip+0x116d31a]        # 0x181296c20
   180129906:	e8 55 9a ee ff       	call   0x180013360
   18012990b:	41 b8 28 00 00 00    	mov    r8d,0x28
   180129911:	48 8b d3             	mov    rdx,rbx
   180129914:	48 8b 0d 05 d3 16 01 	mov    rcx,QWORD PTR [rip+0x116d305]        # 0x181296c20
   18012991b:	ff 15 ff 63 4b 00    	call   QWORD PTR [rip+0x4b63ff]        # 0x1805dfd20 ; ?allocUAVShaderResource@NativeBufferUtil@d3d@@SAXPEAVNativeBuffer@2@_K1@Z
   180129921:	48 83 3d 07 d3 16 01 	cmp    QWORD PTR [rip+0x116d307],0x0        # 0x181296c30
   180129928:	00 
   180129929:	75 39                	jne    0x180129964
   18012992b:	c6 85 00 04 00 00 00 	mov    BYTE PTR [rbp+0x400],0x0
   180129932:	33 d2                	xor    edx,edx
   180129934:	48 8d 8d 00 04 00 00 	lea    rcx,[rbp+0x400]
   18012993b:	e8 00 95 ee ff       	call   0x180012e40
   180129940:	48 8b d0             	mov    rdx,rax
   180129943:	48 8d 0d e6 d2 16 01 	lea    rcx,[rip+0x116d2e6]        # 0x181296c30
   18012994a:	e8 11 9a ee ff       	call   0x180013360
   18012994f:	ba 04 00 00 00       	mov    edx,0x4
   180129954:	44 8b c2             	mov    r8d,edx
   180129957:	48 8b 0d d2 d2 16 01 	mov    rcx,QWORD PTR [rip+0x116d2d2]        # 0x181296c30
   18012995e:	ff 15 bc 63 4b 00    	call   QWORD PTR [rip+0x4b63bc]        # 0x1805dfd20 ; ?allocUAVShaderResource@NativeBufferUtil@d3d@@SAXPEAVNativeBuffer@2@_K1@Z
   180129964:	41 8b 04 36          	mov    eax,DWORD PTR [r14+rsi*1]
   180129968:	39 05 ca d2 16 01    	cmp    DWORD PTR [rip+0x116d2ca],eax        # 0x181296c38
   18012996e:	7e 47                	jle    0x1801299b7
   180129970:	48 8d 0d c1 d2 16 01 	lea    rcx,[rip+0x116d2c1]        # 0x181296c38
   180129977:	e8 e8 e4 45 00       	call   0x180587e64
   18012997c:	83 3d b5 d2 16 01 ff 	cmp    DWORD PTR [rip+0x116d2b5],0xffffffff        # 0x181296c38
   180129983:	75 32                	jne    0x1801299b7
   180129985:	41 b8 0a 00 00 00    	mov    r8d,0xa
   18012998b:	48 8d 15 86 66 50 00 	lea    rdx,[rip+0x506686]        # 0x180630018 ; 'g_rwbDebugSnapPayload'
   180129992:	48 8d 0d a7 d2 16 01 	lea    rcx,[rip+0x116d2a7]        # 0x181296c40
   180129999:	e8 22 74 ee ff       	call   0x180010dc0
   18012999e:	48 8d 0d 9b 4c 4a 00 	lea    rcx,[rip+0x4a4c9b]        # 0x1805ce640
   1801299a5:	e8 1a e2 45 00       	call   0x180587bc4
   1801299aa:	90                   	nop
   1801299ab:	48 8d 0d 86 d2 16 01 	lea    rcx,[rip+0x116d286]        # 0x181296c38
   1801299b2:	e8 4d e4 45 00       	call   0x180587e04
   1801299b7:	48 8b 1d 62 d2 16 01 	mov    rbx,QWORD PTR [rip+0x116d262]        # 0x181296c20
   1801299be:	8b 0d 84 d2 16 01    	mov    ecx,DWORD PTR [rip+0x116d284]        # 0x181296c48
   1801299c4:	ff 15 be 62 4b 00    	call   QWORD PTR [rip+0x4b62be]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   1801299ca:	8b d0                	mov    edx,eax
   1801299cc:	4c 8b c3             	mov    r8,rbx
   1801299cf:	48 8d 8d 00 04 00 00 	lea    rcx,[rbp+0x400]
   1801299d6:	ff 15 bc 62 4b 00    	call   QWORD PTR [rip+0x4b62bc]        # 0x1805dfc98 ; ??0ShaderBuffer@d3d@@QEAA@W4ResourceType@1@PEAVNativeBuffer@1@@Z
   1801299dc:	45 33 c0             	xor    r8d,r8d
   1801299df:	48 8d 95 00 04 00 00 	lea    rdx,[rbp+0x400]
   1801299e6:	8b 0d 5c d2 16 01    	mov    ecx,DWORD PTR [rip+0x116d25c]        # 0x181296c48
   1801299ec:	ff 15 9e 65 4b 00    	call   QWORD PTR [rip+0x4b659e]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   1801299f2:	8b 0d 50 d2 16 01    	mov    ecx,DWORD PTR [rip+0x116d250]        # 0x181296c48
   1801299f8:	ff 15 8a 62 4b 00    	call   QWORD PTR [rip+0x4b628a]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   1801299fe:	8b d0                	mov    edx,eax
   180129a00:	4c 8b c3             	mov    r8,rbx
   180129a03:	48 8d 0d 46 d2 16 01 	lea    rcx,[rip+0x116d246]        # 0x181296c50
   180129a0a:	ff 15 90 62 4b 00    	call   QWORD PTR [rip+0x4b6290]        # 0x1805dfca0 ; ?setBuffer@ShaderBuffer@d3d@@QEAAXW4ResourceType@2@PEAVNativeBuffer@2@@Z
   180129a10:	41 8b 04 36          	mov    eax,DWORD PTR [r14+rsi*1]
   180129a14:	39 05 3e d2 16 01    	cmp    DWORD PTR [rip+0x116d23e],eax        # 0x181296c58
   180129a1a:	7e 47                	jle    0x180129a63
   180129a1c:	48 8d 0d 35 d2 16 01 	lea    rcx,[rip+0x116d235]        # 0x181296c58
   180129a23:	e8 3c e4 45 00       	call   0x180587e64
   180129a28:	83 3d 29 d2 16 01 ff 	cmp    DWORD PTR [rip+0x116d229],0xffffffff        # 0x181296c58
   180129a2f:	75 32                	jne    0x180129a63
   180129a31:	41 b8 0a 00 00 00    	mov    r8d,0xa
   180129a37:	48 8d 15 3a 66 50 00 	lea    rdx,[rip+0x50663a]        # 0x180630078 ; 'g_rwbDebugSnapPayloadCount'
   180129a3e:	48 8d 0d 1b d2 16 01 	lea    rcx,[rip+0x116d21b]        # 0x181296c60
   180129a45:	e8 76 73 ee ff       	call   0x180010dc0
   180129a4a:	48 8d 0d bf 4b 4a 00 	lea    rcx,[rip+0x4a4bbf]        # 0x1805ce610
   180129a51:	e8 6e e1 45 00       	call   0x180587bc4
   180129a56:	90                   	nop
   180129a57:	48 8d 0d fa d1 16 01 	lea    rcx,[rip+0x116d1fa]        # 0x181296c58
   180129a5e:	e8 a1 e3 45 00       	call   0x180587e04
   180129a63:	48 8b 1d c6 d1 16 01 	mov    rbx,QWORD PTR [rip+0x116d1c6]        # 0x181296c30
   180129a6a:	8b 0d f8 d1 16 01    	mov    ecx,DWORD PTR [rip+0x116d1f8]        # 0x181296c68
   180129a70:	ff 15 12 62 4b 00    	call   QWORD PTR [rip+0x4b6212]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   180129a76:	8b d0                	mov    edx,eax
   180129a78:	4c 8b c3             	mov    r8,rbx
   180129a7b:	48 8d 8d 00 04 00 00 	lea    rcx,[rbp+0x400]
   180129a82:	ff 15 10 62 4b 00    	call   QWORD PTR [rip+0x4b6210]        # 0x1805dfc98 ; ??0ShaderBuffer@d3d@@QEAA@W4ResourceType@1@PEAVNativeBuffer@1@@Z
   180129a88:	45 33 c0             	xor    r8d,r8d
   180129a8b:	48 8d 95 00 04 00 00 	lea    rdx,[rbp+0x400]
   180129a92:	8b 0d d0 d1 16 01    	mov    ecx,DWORD PTR [rip+0x116d1d0]        # 0x181296c68
   180129a98:	ff 15 f2 64 4b 00    	call   QWORD PTR [rip+0x4b64f2]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   180129a9e:	8b 0d c4 d1 16 01    	mov    ecx,DWORD PTR [rip+0x116d1c4]        # 0x181296c68
   180129aa4:	ff 15 de 61 4b 00    	call   QWORD PTR [rip+0x4b61de]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   180129aaa:	8b d0                	mov    edx,eax
   180129aac:	4c 8b c3             	mov    r8,rbx
   180129aaf:	48 8d 0d ba d1 16 01 	lea    rcx,[rip+0x116d1ba]        # 0x181296c70
   180129ab6:	ff 15 e4 61 4b 00    	call   QWORD PTR [rip+0x4b61e4]        # 0x1805dfca0 ; ?setBuffer@ShaderBuffer@d3d@@QEAAXW4ResourceType@2@PEAVNativeBuffer@2@@Z
   180129abc:	48 8b 0d 35 3e 16 01 	mov    rcx,QWORD PTR [rip+0x1163e35]        # 0x18128d8f8
   180129ac3:	48 85 c9             	test   rcx,rcx
   180129ac6:	75 3c                	jne    0x180129b04
   180129ac8:	48 8d 15 f9 64 50 00 	lea    rdx,[rip+0x5064f9]        # 0x18062ffc8 ; 'rt_deferred_reflection.rfx'
   180129acf:	48 8b 0d fa 91 75 00 	mov    rcx,QWORD PTR [rip+0x7591fa]        # 0x180882cd0
   180129ad6:	e8 85 fe 0a 00       	call   0x1801d9960
   180129adb:	48 8d 15 6e 65 50 00 	lea    rdx,[rip+0x50656e]        # 0x180630050 ; 'rt_deferredReflectionSnapDebugClear'
   180129ae2:	48 8b c8             	mov    rcx,rax
   180129ae5:	e8 f6 91 0a 00       	call   0x1801d2ce0
   180129aea:	48 89 05 07 3e 16 01 	mov    QWORD PTR [rip+0x1163e07],rax        # 0x18128d8f8
   180129af1:	48 8d 0d 00 3e 16 01 	lea    rcx,[rip+0x1163e00]        # 0x18128d8f8
   180129af8:	e8 03 ec 0a 00       	call   0x1801d8700
   180129afd:	48 8b 0d f4 3d 16 01 	mov    rcx,QWORD PTR [rip+0x1163df4]        # 0x18128d8f8
   180129b04:	33 d2                	xor    edx,edx
   180129b06:	e8 45 39 0b 00       	call   0x1801dd450
   180129b0b:	48 8b c8             	mov    rcx,rax
   180129b0e:	e8 3d 1c 0b 00       	call   0x1801db750
   180129b13:	c7 45 90 01 00 00 00 	mov    DWORD PTR [rbp-0x70],0x1
   180129b1a:	c7 45 94 01 00 00 00 	mov    DWORD PTR [rbp-0x6c],0x1
   180129b21:	f2 0f 10 45 90       	movsd  xmm0,QWORD PTR [rbp-0x70]
   180129b26:	f2 0f 11 45 90       	movsd  QWORD PTR [rbp-0x70],xmm0
   180129b2b:	c7 45 98 01 00 00 00 	mov    DWORD PTR [rbp-0x68],0x1
   180129b32:	48 8d 55 90          	lea    rdx,[rbp-0x70]
   180129b36:	48 8b cf             	mov    rcx,rdi
   180129b39:	ff 15 81 61 4b 00    	call   QWORD PTR [rip+0x4b6181]        # 0x1805dfcc0 ; ?dispatch@DeviceUtil@d3d@@SAXAEAVDeviceState@2@V?$Vector3Template@H@m@@@Z
   180129b3f:	48 8b 0d aa 3d 16 01 	mov    rcx,QWORD PTR [rip+0x1163daa]        # 0x18128d8f0
   180129b46:	48 85 c9             	test   rcx,rcx
   180129b49:	75 3c                	jne    0x180129b87
   180129b4b:	48 8d 15 76 64 50 00 	lea    rdx,[rip+0x506476]        # 0x18062ffc8 ; 'rt_deferred_reflection.rfx'
   180129b52:	48 8b 0d 77 91 75 00 	mov    rcx,QWORD PTR [rip+0x759177]        # 0x180882cd0
   180129b59:	e8 02 fe 0a 00       	call   0x1801d9960
   180129b5e:	48 8d 15 4b 65 50 00 	lea    rdx,[rip+0x50654b]        # 0x1806300b0 ; 'rt_deferredReflectionSnapDebug'
   180129b65:	48 8b c8             	mov    rcx,rax
   180129b68:	e8 73 91 0a 00       	call   0x1801d2ce0
   180129b6d:	48 89 05 7c 3d 16 01 	mov    QWORD PTR [rip+0x1163d7c],rax        # 0x18128d8f0
   180129b74:	48 8d 0d 75 3d 16 01 	lea    rcx,[rip+0x1163d75]        # 0x18128d8f0
   180129b7b:	e8 80 eb 0a 00       	call   0x1801d8700
   180129b80:	48 8b 0d 69 3d 16 01 	mov    rcx,QWORD PTR [rip+0x1163d69]        # 0x18128d8f0
   180129b87:	33 d2                	xor    edx,edx
   180129b89:	e8 c2 38 0b 00       	call   0x1801dd450
   180129b8e:	48 8b c8             	mov    rcx,rax
   180129b91:	e8 ba 1b 0b 00       	call   0x1801db750
   180129b96:	8b 05 f0 48 7e 00    	mov    eax,DWORD PTR [rip+0x7e48f0]        # 0x18090e48c
   180129b9c:	83 c0 07             	add    eax,0x7
   180129b9f:	99                   	cdq
   180129ba0:	83 e2 07             	and    edx,0x7
   180129ba3:	8d 0c 02             	lea    ecx,[rdx+rax*1]
   180129ba6:	c1 f9 03             	sar    ecx,0x3
   180129ba9:	8b 05 d9 48 7e 00    	mov    eax,DWORD PTR [rip+0x7e48d9]        # 0x18090e488
   180129baf:	83 c0 07             	add    eax,0x7
   180129bb2:	99                   	cdq
   180129bb3:	83 e2 07             	and    edx,0x7
   180129bb6:	03 c2                	add    eax,edx
   180129bb8:	c1 f8 03             	sar    eax,0x3
   180129bbb:	89 45 90             	mov    DWORD PTR [rbp-0x70],eax
   180129bbe:	89 4d 94             	mov    DWORD PTR [rbp-0x6c],ecx
   180129bc1:	f2 0f 10 45 90       	movsd  xmm0,QWORD PTR [rbp-0x70]
   180129bc6:	f2 0f 11 45 90       	movsd  QWORD PTR [rbp-0x70],xmm0
   180129bcb:	c7 45 98 01 00 00 00 	mov    DWORD PTR [rbp-0x68],0x1
   180129bd2:	48 8d 55 90          	lea    rdx,[rbp-0x70]
   180129bd6:	48 8b cf             	mov    rcx,rdi
   180129bd9:	ff 15 e1 60 4b 00    	call   QWORD PTR [rip+0x4b60e1]        # 0x1805dfcc0 ; ?dispatch@DeviceUtil@d3d@@SAXAEAVDeviceState@2@V?$Vector3Template@H@m@@@Z
   180129bdf:	48 c7 85 00 04 00 00 	mov    QWORD PTR [rbp+0x400],0x0
   180129be6:	00 00 00 00 
   180129bea:	48 8b 1d 3f d0 16 01 	mov    rbx,QWORD PTR [rip+0x116d03f]        # 0x181296c30
   180129bf1:	48 8b cb             	mov    rcx,rbx
   180129bf4:	ff 15 7e 62 4b 00    	call   QWORD PTR [rip+0x4b627e]        # 0x1805dfe78 ; ?getSize@NativeBufferUtil@d3d@@SA_KPEBVNativeBuffer@2@@Z
   180129bfa:	4c 8b c0             	mov    r8,rax
   180129bfd:	48 8d 85 00 04 00 00 	lea    rax,[rbp+0x400]
   180129c04:	48 89 44 24 20       	mov    QWORD PTR [rsp+0x20],rax
   180129c09:	33 d2                	xor    edx,edx
   180129c0b:	44 8d 4a 01          	lea    r9d,[rdx+0x1]
   180129c0f:	48 8b cb             	mov    rcx,rbx
   180129c12:	ff 15 58 62 4b 00    	call   QWORD PTR [rip+0x4b6258]        # 0x1805dfe70 ; ?lock@NativeBufferUtil@d3d@@SAXPEAVNativeBuffer@2@_K1W4MapType@2@PEAPEAX@Z
   180129c18:	48 8b 85 00 04 00 00 	mov    rax,QWORD PTR [rbp+0x400]
   180129c1f:	8b 18                	mov    ebx,DWORD PTR [rax]
   180129c21:	33 d2                	xor    edx,edx
   180129c23:	48 8b 0d 06 d0 16 01 	mov    rcx,QWORD PTR [rip+0x116d006]        # 0x181296c30
   180129c2a:	ff 15 50 62 4b 00    	call   QWORD PTR [rip+0x4b6250]        # 0x1805dfe80 ; ?unlock@NativeBufferUtil@d3d@@SAXPEAVNativeBuffer@2@PEAX@Z
   180129c30:	8b fb                	mov    edi,ebx
   180129c32:	8b d3                	mov    edx,ebx
   180129c34:	48 8d 0d 0d cf 16 01 	lea    rcx,[rip+0x116cf0d]        # 0x181296b48
   180129c3b:	e8 d0 e3 00 00       	call   0x180138010
   180129c40:	48 c7 45 c0 00 00 00 	mov    QWORD PTR [rbp-0x40],0x0
   180129c47:	00 
   180129c48:	48 8b 1d d1 cf 16 01 	mov    rbx,QWORD PTR [rip+0x116cfd1]        # 0x181296c20
   180129c4f:	48 8b cb             	mov    rcx,rbx
   180129c52:	ff 15 20 62 4b 00    	call   QWORD PTR [rip+0x4b6220]        # 0x1805dfe78 ; ?getSize@NativeBufferUtil@d3d@@SA_KPEBVNativeBuffer@2@@Z
   180129c58:	4c 8b c0             	mov    r8,rax
   180129c5b:	48 8d 45 c0          	lea    rax,[rbp-0x40]
   180129c5f:	48 89 44 24 20       	mov    QWORD PTR [rsp+0x20],rax
   180129c64:	33 d2                	xor    edx,edx
   180129c66:	44 8d 4a 01          	lea    r9d,[rdx+0x1]
   180129c6a:	48 8b cb             	mov    rcx,rbx
   180129c6d:	ff 15 fd 61 4b 00    	call   QWORD PTR [rip+0x4b61fd]        # 0x1805dfe70 ; ?lock@NativeBufferUtil@d3d@@SAXPEAVNativeBuffer@2@_K1W4MapType@2@PEAPEAX@Z
   180129c73:	4c 8d 04 bf          	lea    r8,[rdi+rdi*4]
   180129c77:	49 c1 e0 03          	shl    r8,0x3
   180129c7b:	48 8b 55 c0          	mov    rdx,QWORD PTR [rbp-0x40]
   180129c7f:	48 8b 0d c2 ce 16 01 	mov    rcx,QWORD PTR [rip+0x116cec2]        # 0x181296b48
   180129c86:	e8 83 f1 45 00       	call   0x180588e0e
   180129c8b:	33 d2                	xor    edx,edx
   180129c8d:	48 8b 0d 8c cf 16 01 	mov    rcx,QWORD PTR [rip+0x116cf8c]        # 0x181296c20
   180129c94:	ff 15 e6 61 4b 00    	call   QWORD PTR [rip+0x4b61e6]        # 0x1805dfe80 ; ?unlock@NativeBufferUtil@d3d@@SAXPEAVNativeBuffer@2@PEAX@Z
   180129c9a:	48 8b 15 37 8d 6d 00 	mov    rdx,QWORD PTR [rip+0x6d8d37]        # 0x1808029d8
   180129ca1:	48 8d 0d b8 cb 16 01 	lea    rcx,[rip+0x116cbb8]        # 0x181296860
   180129ca8:	ff 15 ca 6a 4b 00    	call   QWORD PTR [rip+0x4b6aca]        # 0x1805e0778 ; ??4View@m@@QEAAAEAV01@AEBV01@@Z
   180129cae:	48 8d 1d 5b 5b 4f 00 	lea    rbx,[rip+0x4f5b5b]        # 0x18061f810
   180129cb5:	83 3d 94 ce 16 01 00 	cmp    DWORD PTR [rip+0x116ce94],0x0        # 0x181296b50
   180129cbc:	0f 84 cd 1b 00 00    	je     0x18012b88f
   180129cc2:	41 8b 04 36          	mov    eax,DWORD PTR [r14+rsi*1]
   180129cc6:	39 05 ac cf 16 01    	cmp    DWORD PTR [rip+0x116cfac],eax        # 0x181296c78
   180129ccc:	7e 55                	jle    0x180129d23
   180129cce:	48 8d 0d a3 cf 16 01 	lea    rcx,[rip+0x116cfa3]        # 0x181296c78
   180129cd5:	e8 8a e1 45 00       	call   0x180587e64
   180129cda:	83 3d 97 cf 16 01 ff 	cmp    DWORD PTR [rip+0x116cf97],0xffffffff        # 0x181296c78
   180129ce1:	75 40                	jne    0x180129d23
   180129ce3:	45 33 c0             	xor    r8d,r8d
   180129ce6:	48 8d 15 ab 63 50 00 	lea    rdx,[rip+0x5063ab]        # 0x180630098 ; 'rtd:Origin Spheres'
   180129ced:	48 8d 0d 8c cf 16 01 	lea    rcx,[rip+0x116cf8c]        # 0x181296c80
   180129cf4:	ff 15 2e 76 4b 00    	call   QWORD PTR [rip+0x4b762e]        # 0x1805e1328 ; ??0BaseTweakable@d@@QEAA@PEBDW4Type@01@@Z
   180129cfa:	48 89 1d 7f cf 16 01 	mov    QWORD PTR [rip+0x116cf7f],rbx        # 0x181296c80
   180129d01:	66 c7 05 26 d0 16 01 	mov    WORD PTR [rip+0x116d026],0x0        # 0x181296d30
   180129d08:	00 00 
   180129d0a:	48 8d 0d ef 48 4a 00 	lea    rcx,[rip+0x4a48ef]        # 0x1805ce600
   180129d11:	e8 ae de 45 00       	call   0x180587bc4
   180129d16:	90                   	nop
   180129d17:	48 8d 0d 5a cf 16 01 	lea    rcx,[rip+0x116cf5a]        # 0x181296c78
   180129d1e:	e8 e1 e0 45 00       	call   0x180587e04
   180129d23:	41 8b 04 36          	mov    eax,DWORD PTR [r14+rsi*1]
   180129d27:	39 05 0b d0 16 01    	cmp    DWORD PTR [rip+0x116d00b],eax        # 0x181296d38
   180129d2d:	7e 55                	jle    0x180129d84
   180129d2f:	48 8d 0d 02 d0 16 01 	lea    rcx,[rip+0x116d002]        # 0x181296d38
   180129d36:	e8 29 e1 45 00       	call   0x180587e64
   180129d3b:	83 3d f6 cf 16 01 ff 	cmp    DWORD PTR [rip+0x116cff6],0xffffffff        # 0x181296d38
   180129d42:	75 40                	jne    0x180129d84
   180129d44:	45 33 c0             	xor    r8d,r8d
   180129d47:	48 8d 15 9a 63 50 00 	lea    rdx,[rip+0x50639a]        # 0x1806300e8 ; 'rtd:Reflected'
   180129d4e:	48 8d 0d eb cf 16 01 	lea    rcx,[rip+0x116cfeb]        # 0x181296d40
   180129d55:	ff 15 cd 75 4b 00    	call   QWORD PTR [rip+0x4b75cd]        # 0x1805e1328 ; ??0BaseTweakable@d@@QEAA@PEBDW4Type@01@@Z
   180129d5b:	48 89 1d de cf 16 01 	mov    QWORD PTR [rip+0x116cfde],rbx        # 0x181296d40
   180129d62:	66 c7 05 85 d0 16 01 	mov    WORD PTR [rip+0x116d085],0x0        # 0x181296df0
   180129d69:	00 00 
   180129d6b:	48 8d 0d 7e 48 4a 00 	lea    rcx,[rip+0x4a487e]        # 0x1805ce5f0
   180129d72:	e8 4d de 45 00       	call   0x180587bc4
   180129d77:	90                   	nop
   180129d78:	48 8d 0d b9 cf 16 01 	lea    rcx,[rip+0x116cfb9]        # 0x181296d38
   180129d7f:	e8 80 e0 45 00       	call   0x180587e04
   180129d84:	41 8b 04 36          	mov    eax,DWORD PTR [r14+rsi*1]
   180129d88:	39 05 6a d0 16 01    	cmp    DWORD PTR [rip+0x116d06a],eax        # 0x181296df8
   180129d8e:	7e 55                	jle    0x180129de5
   180129d90:	48 8d 0d 61 d0 16 01 	lea    rcx,[rip+0x116d061]        # 0x181296df8
   180129d97:	e8 c8 e0 45 00       	call   0x180587e64
   180129d9c:	83 3d 55 d0 16 01 ff 	cmp    DWORD PTR [rip+0x116d055],0xffffffff        # 0x181296df8
   180129da3:	75 40                	jne    0x180129de5
   180129da5:	45 33 c0             	xor    r8d,r8d
   180129da8:	48 8d 15 21 63 50 00 	lea    rdx,[rip+0x506321]        # 0x1806300d0 ; 'rtd:Missed Lines'
   180129daf:	48 8d 0d 4a d0 16 01 	lea    rcx,[rip+0x116d04a]        # 0x181296e00
   180129db6:	ff 15 6c 75 4b 00    	call   QWORD PTR [rip+0x4b756c]        # 0x1805e1328 ; ??0BaseTweakable@d@@QEAA@PEBDW4Type@01@@Z
   180129dbc:	48 89 1d 3d d0 16 01 	mov    QWORD PTR [rip+0x116d03d],rbx        # 0x181296e00
   180129dc3:	66 c7 05 e4 d0 16 01 	mov    WORD PTR [rip+0x116d0e4],0x0        # 0x181296eb0
   180129dca:	00 00 
   180129dcc:	48 8d 0d 0d 48 4a 00 	lea    rcx,[rip+0x4a480d]        # 0x1805ce5e0
   180129dd3:	e8 ec dd 45 00       	call   0x180587bc4
   180129dd8:	90                   	nop
   180129dd9:	48 8d 0d 18 d0 16 01 	lea    rcx,[rip+0x116d018]        # 0x181296df8
   180129de0:	e8 1f e0 45 00       	call   0x180587e04
   180129de5:	41 8b 04 36          	mov    eax,DWORD PTR [r14+rsi*1]
   180129de9:	39 05 c9 d0 16 01    	cmp    DWORD PTR [rip+0x116d0c9],eax        # 0x181296eb8
   180129def:	7e 55                	jle    0x180129e46
   180129df1:	48 8d 0d c0 d0 16 01 	lea    rcx,[rip+0x116d0c0]        # 0x181296eb8
   180129df8:	e8 67 e0 45 00       	call   0x180587e64
   180129dfd:	83 3d b4 d0 16 01 ff 	cmp    DWORD PTR [rip+0x116d0b4],0xffffffff        # 0x181296eb8
   180129e04:	75 40                	jne    0x180129e46
   180129e06:	45 33 c0             	xor    r8d,r8d
   180129e09:	48 8d 15 08 63 50 00 	lea    rdx,[rip+0x506308]        # 0x180630118 ; 'rtd:Hit Lines'
   180129e10:	48 8d 0d a9 d0 16 01 	lea    rcx,[rip+0x116d0a9]        # 0x181296ec0
   180129e17:	ff 15 0b 75 4b 00    	call   QWORD PTR [rip+0x4b750b]        # 0x1805e1328 ; ??0BaseTweakable@d@@QEAA@PEBDW4Type@01@@Z
   180129e1d:	48 89 1d 9c d0 16 01 	mov    QWORD PTR [rip+0x116d09c],rbx        # 0x181296ec0
   180129e24:	66 c7 05 43 d1 16 01 	mov    WORD PTR [rip+0x116d143],0x0        # 0x181296f70
   180129e2b:	00 00 
   180129e2d:	48 8d 0d 9c 47 4a 00 	lea    rcx,[rip+0x4a479c]        # 0x1805ce5d0
   180129e34:	e8 8b dd 45 00       	call   0x180587bc4
   180129e39:	90                   	nop
   180129e3a:	48 8d 0d 77 d0 16 01 	lea    rcx,[rip+0x116d077]        # 0x181296eb8
   180129e41:	e8 be df 45 00       	call   0x180587e04
   180129e46:	41 8b 04 36          	mov    eax,DWORD PTR [r14+rsi*1]
   180129e4a:	39 05 28 d1 16 01    	cmp    DWORD PTR [rip+0x116d128],eax        # 0x181296f78
   180129e50:	7e 55                	jle    0x180129ea7
   180129e52:	48 8d 0d 1f d1 16 01 	lea    rcx,[rip+0x116d11f]        # 0x181296f78
   180129e59:	e8 06 e0 45 00       	call   0x180587e64
   180129e5e:	83 3d 13 d1 16 01 ff 	cmp    DWORD PTR [rip+0x116d113],0xffffffff        # 0x181296f78
   180129e65:	75 40                	jne    0x180129ea7
   180129e67:	45 33 c0             	xor    r8d,r8d
   180129e6a:	48 8d 15 87 62 50 00 	lea    rdx,[rip+0x506287]        # 0x1806300f8 ; 'rtd:Use Radiance On Lines'
   180129e71:	48 8d 0d 08 d1 16 01 	lea    rcx,[rip+0x116d108]        # 0x181296f80
   180129e78:	ff 15 aa 74 4b 00    	call   QWORD PTR [rip+0x4b74aa]        # 0x1805e1328 ; ??0BaseTweakable@d@@QEAA@PEBDW4Type@01@@Z
   180129e7e:	48 89 1d fb d0 16 01 	mov    QWORD PTR [rip+0x116d0fb],rbx        # 0x181296f80
   180129e85:	66 c7 05 a2 d1 16 01 	mov    WORD PTR [rip+0x116d1a2],0x0        # 0x181297030
   180129e8c:	00 00 
   180129e8e:	48 8d 0d 2b 47 4a 00 	lea    rcx,[rip+0x4a472b]        # 0x1805ce5c0
   180129e95:	e8 2a dd 45 00       	call   0x180587bc4
   180129e9a:	90                   	nop
   180129e9b:	48 8d 0d d6 d0 16 01 	lea    rcx,[rip+0x116d0d6]        # 0x181296f78
   180129ea2:	e8 5d df 45 00       	call   0x180587e04
   180129ea7:	41 8b 04 36          	mov    eax,DWORD PTR [r14+rsi*1]
   180129eab:	39 05 87 d1 16 01    	cmp    DWORD PTR [rip+0x116d187],eax        # 0x181297038
   180129eb1:	7e 55                	jle    0x180129f08
   180129eb3:	48 8d 0d 7e d1 16 01 	lea    rcx,[rip+0x116d17e]        # 0x181297038
   180129eba:	e8 a5 df 45 00       	call   0x180587e64
   180129ebf:	83 3d 72 d1 16 01 ff 	cmp    DWORD PTR [rip+0x116d172],0xffffffff        # 0x181297038
   180129ec6:	75 40                	jne    0x180129f08
   180129ec8:	45 33 c0             	xor    r8d,r8d
   180129ecb:	48 8d 15 66 62 50 00 	lea    rdx,[rip+0x506266]        # 0x180630138 ; 'rtd:Camera Frustum'
   180129ed2:	48 8d 0d 67 d1 16 01 	lea    rcx,[rip+0x116d167]        # 0x181297040
   180129ed9:	ff 15 49 74 4b 00    	call   QWORD PTR [rip+0x4b7449]        # 0x1805e1328 ; ??0BaseTweakable@d@@QEAA@PEBDW4Type@01@@Z
   180129edf:	48 89 1d 5a d1 16 01 	mov    QWORD PTR [rip+0x116d15a],rbx        # 0x181297040
   180129ee6:	66 c7 05 01 d2 16 01 	mov    WORD PTR [rip+0x116d201],0x0        # 0x1812970f0
   180129eed:	00 00 
   180129eef:	48 8d 0d ba 46 4a 00 	lea    rcx,[rip+0x4a46ba]        # 0x1805ce5b0
   180129ef6:	e8 c9 dc 45 00       	call   0x180587bc4
   180129efb:	90                   	nop
   180129efc:	48 8d 0d 35 d1 16 01 	lea    rcx,[rip+0x116d135]        # 0x181297038
   180129f03:	e8 fc de 45 00       	call   0x180587e04
   180129f08:	41 8b 04 36          	mov    eax,DWORD PTR [r14+rsi*1]
   180129f0c:	39 05 e6 d1 16 01    	cmp    DWORD PTR [rip+0x116d1e6],eax        # 0x1812970f8
   180129f12:	7e 55                	jle    0x180129f69
   180129f14:	48 8d 0d dd d1 16 01 	lea    rcx,[rip+0x116d1dd]        # 0x1812970f8
   180129f1b:	e8 44 df 45 00       	call   0x180587e64
   180129f20:	83 3d d1 d1 16 01 ff 	cmp    DWORD PTR [rip+0x116d1d1],0xffffffff        # 0x1812970f8
   180129f27:	75 40                	jne    0x180129f69
   180129f29:	45 33 c0             	xor    r8d,r8d
   180129f2c:	48 8d 15 f5 61 50 00 	lea    rdx,[rip+0x5061f5]        # 0x180630128 ; 'rtd:Eye Rays'
   180129f33:	48 8d 0d c6 d1 16 01 	lea    rcx,[rip+0x116d1c6]        # 0x181297100
   180129f3a:	ff 15 e8 73 4b 00    	call   QWORD PTR [rip+0x4b73e8]        # 0x1805e1328 ; ??0BaseTweakable@d@@QEAA@PEBDW4Type@01@@Z
   180129f40:	48 89 1d b9 d1 16 01 	mov    QWORD PTR [rip+0x116d1b9],rbx        # 0x181297100
   180129f47:	66 c7 05 60 d2 16 01 	mov    WORD PTR [rip+0x116d260],0x0        # 0x1812971b0
   180129f4e:	00 00 
   180129f50:	48 8d 0d 49 46 4a 00 	lea    rcx,[rip+0x4a4649]        # 0x1805ce5a0
   180129f57:	e8 68 dc 45 00       	call   0x180587bc4
   180129f5c:	90                   	nop
   180129f5d:	48 8d 0d 94 d1 16 01 	lea    rcx,[rip+0x116d194]        # 0x1812970f8
   180129f64:	e8 9b de 45 00       	call   0x180587e04
   180129f69:	41 8b 04 36          	mov    eax,DWORD PTR [r14+rsi*1]
   180129f6d:	39 05 45 d2 16 01    	cmp    DWORD PTR [rip+0x116d245],eax        # 0x1812971b8
   180129f73:	7e 64                	jle    0x180129fd9
   180129f75:	48 8d 0d 3c d2 16 01 	lea    rcx,[rip+0x116d23c]        # 0x1812971b8
   180129f7c:	e8 e3 de 45 00       	call   0x180587e64
   180129f81:	83 3d 30 d2 16 01 ff 	cmp    DWORD PTR [rip+0x116d230],0xffffffff        # 0x1812971b8
   180129f88:	75 4f                	jne    0x180129fd9
   180129f8a:	41 b8 02 00 00 00    	mov    r8d,0x2
   180129f90:	48 8d 15 e9 61 50 00 	lea    rdx,[rip+0x5061e9]        # 0x180630180 ; 'rtd:Reflection Line Length'
   180129f97:	48 8d 0d 22 d2 16 01 	lea    rcx,[rip+0x116d222]        # 0x1812971c0
   180129f9e:	ff 15 84 73 4b 00    	call   QWORD PTR [rip+0x4b7384]        # 0x1805e1328 ; ??0BaseTweakable@d@@QEAA@PEBDW4Type@01@@Z
   180129fa4:	48 8d 05 ed 57 4f 00 	lea    rax,[rip+0x4f57ed]        # 0x18061f798
   180129fab:	48 89 05 0e d2 16 01 	mov    QWORD PTR [rip+0x116d20e],rax        # 0x1812971c0
   180129fb2:	0f 28 05 37 57 55 00 	movaps xmm0,XMMWORD PTR [rip+0x555737]        # 0x18067f6f0
   180129fb9:	0f 11 05 b0 d2 16 01 	movups XMMWORD PTR [rip+0x116d2b0],xmm0        # 0x181297270
   180129fc0:	48 8d 0d c9 45 4a 00 	lea    rcx,[rip+0x4a45c9]        # 0x1805ce590
   180129fc7:	e8 f8 db 45 00       	call   0x180587bc4
   180129fcc:	90                   	nop
   180129fcd:	48 8d 0d e4 d1 16 01 	lea    rcx,[rip+0x116d1e4]        # 0x1812971b8
   180129fd4:	e8 2b de 45 00       	call   0x180587e04
   180129fd9:	ba 10 00 00 00       	mov    edx,0x10
   180129fde:	44 8d 42 1a          	lea    r8d,[rdx+0x1a]
   180129fe2:	48 8d 8d 50 01 00 00 	lea    rcx,[rbp+0x150]
   180129fe9:	e8 02 29 0e 00       	call   0x18020c8f0
   180129fee:	90                   	nop
   180129fef:	8b 05 5b cb 16 01    	mov    eax,DWORD PTR [rip+0x116cb5b]        # 0x181296b50
   180129ff5:	48 8d 0c 80          	lea    rcx,[rax+rax*4]
   180129ff9:	48 8b 35 48 cb 16 01 	mov    rsi,QWORD PTR [rip+0x116cb48]        # 0x181296b48
   18012a000:	48 8d 1c ce          	lea    rbx,[rsi+rcx*8]
   18012a004:	48 3b f3             	cmp    rsi,rbx
   18012a007:	0f 84 9e 17 00 00    	je     0x18012b7ab
   18012a00d:	48 83 c6 20          	add    rsi,0x20
   18012a011:	41 bf ff 00 00 00    	mov    r15d,0xff
   18012a017:	f3 44 0f 10 1d 54 4d 	movss  xmm11,DWORD PTR [rip+0x554d54]        # 0x18067ed74
   18012a01e:	55 00 
   18012a020:	f3 44 0f 10 25 bf 53 	movss  xmm12,DWORD PTR [rip+0x5553bf]        # 0x18067f3e8
   18012a027:	55 00 
   18012a029:	80 3d 00 cd 16 01 00 	cmp    BYTE PTR [rip+0x116cd00],0x0        # 0x181296d30
   18012a030:	0f 84 47 01 00 00    	je     0x18012a17d
   18012a036:	0f 28 05 23 55 55 00 	movaps xmm0,XMMWORD PTR [rip+0x555523]        # 0x18067f560
   18012a03d:	0f 29 45 e0          	movaps XMMWORD PTR [rbp-0x20],xmm0
   18012a041:	0f 28 c8             	movaps xmm1,xmm0
   18012a044:	0f 29 45 f0          	movaps XMMWORD PTR [rbp-0x10],xmm0
   18012a048:	0f 29 45 00          	movaps XMMWORD PTR [rbp+0x0],xmm0
   18012a04c:	48 8d 15 7d c8 16 01 	lea    rdx,[rip+0x116c87d]        # 0x1812968d0
   18012a053:	48 8d 8d a0 00 00 00 	lea    rcx,[rbp+0xa0]
   18012a05a:	e8 41 e9 ee ff       	call   0x1800189a0
   18012a05f:	f2 0f 10 76 e0       	movsd  xmm6,QWORD PTR [rsi-0x20]
   18012a064:	8b 46 e8             	mov    eax,DWORD PTR [rsi-0x18]
   18012a067:	89 85 0c 01 00 00    	mov    DWORD PTR [rbp+0x10c],eax
   18012a06d:	f3 0f 10 8d a0 00 00 	movss  xmm1,DWORD PTR [rbp+0xa0]
   18012a074:	00 
   18012a075:	f2 0f 11 b5 04 01 00 	movsd  QWORD PTR [rbp+0x104],xmm6
   18012a07c:	00 
   18012a07d:	f3 0f 59 ce          	mulss  xmm1,xmm6
   18012a081:	f3 0f 10 bd ac 00 00 	movss  xmm7,DWORD PTR [rbp+0xac]
   18012a088:	00 
   18012a089:	f3 0f 10 ad 08 01 00 	movss  xmm5,DWORD PTR [rbp+0x108]
   18012a090:	00 
   18012a091:	f3 0f 59 fd          	mulss  xmm7,xmm5
   18012a095:	f3 0f 10 85 b8 00 00 	movss  xmm0,DWORD PTR [rbp+0xb8]
   18012a09c:	00 
   18012a09d:	f3 0f 10 9d 0c 01 00 	movss  xmm3,DWORD PTR [rbp+0x10c]
   18012a0a4:	00 
   18012a0a5:	f3 0f 59 c3          	mulss  xmm0,xmm3
   18012a0a9:	f3 0f 58 f9          	addss  xmm7,xmm1
   18012a0ad:	f3 0f 58 f8          	addss  xmm7,xmm0
   18012a0b1:	f3 0f 58 bd c4 00 00 	addss  xmm7,DWORD PTR [rbp+0xc4]
   18012a0b8:	00 
   18012a0b9:	f3 0f 10 a5 a4 00 00 	movss  xmm4,DWORD PTR [rbp+0xa4]
   18012a0c0:	00 
   18012a0c1:	f3 0f 59 e6          	mulss  xmm4,xmm6
   18012a0c5:	f3 0f 10 8d b0 00 00 	movss  xmm1,DWORD PTR [rbp+0xb0]
   18012a0cc:	00 
   18012a0cd:	f3 0f 59 cd          	mulss  xmm1,xmm5
   18012a0d1:	f3 0f 10 85 bc 00 00 	movss  xmm0,DWORD PTR [rbp+0xbc]
   18012a0d8:	00 
   18012a0d9:	f3 0f 59 c3          	mulss  xmm0,xmm3
   18012a0dd:	f3 0f 58 e1          	addss  xmm4,xmm1
   18012a0e1:	f3 0f 58 e0          	addss  xmm4,xmm0
   18012a0e5:	f3 0f 58 a5 c8 00 00 	addss  xmm4,DWORD PTR [rbp+0xc8]
   18012a0ec:	00 
   18012a0ed:	f3 0f 10 95 a8 00 00 	movss  xmm2,DWORD PTR [rbp+0xa8]
   18012a0f4:	00 
   18012a0f5:	f3 0f 59 d6          	mulss  xmm2,xmm6
   18012a0f9:	f3 0f 10 8d b4 00 00 	movss  xmm1,DWORD PTR [rbp+0xb4]
   18012a100:	00 
   18012a101:	f3 0f 59 cd          	mulss  xmm1,xmm5
   18012a105:	f3 0f 10 85 c0 00 00 	movss  xmm0,DWORD PTR [rbp+0xc0]
   18012a10c:	00 
   18012a10d:	f3 0f 59 c3          	mulss  xmm0,xmm3
   18012a111:	f3 0f 58 d1          	addss  xmm2,xmm1
   18012a115:	f3 0f 58 d0          	addss  xmm2,xmm0
   18012a119:	f3 0f 58 95 cc 00 00 	addss  xmm2,DWORD PTR [rbp+0xcc]
   18012a120:	00 
   18012a121:	f3 0f 11 bd 04 01 00 	movss  DWORD PTR [rbp+0x104],xmm7
   18012a128:	00 
   18012a129:	f3 0f 11 a5 08 01 00 	movss  DWORD PTR [rbp+0x108],xmm4
   18012a130:	00 
   18012a131:	f3 0f 11 95 0c 01 00 	movss  DWORD PTR [rbp+0x10c],xmm2
   18012a138:	00 
   18012a139:	c7 44 24 38 08 00 00 	mov    DWORD PTR [rsp+0x38],0x8
   18012a140:	00 
   18012a141:	c7 44 24 30 0c 00 00 	mov    DWORD PTR [rsp+0x30],0xc
   18012a148:	00 
   18012a149:	c7 44 24 28 01 00 00 	mov    DWORD PTR [rsp+0x28],0x1
   18012a150:	00 
   18012a151:	c7 44 24 20 00 00 00 	mov    DWORD PTR [rsp+0x20],0x0
   18012a158:	00 
   18012a159:	4c 8b 0d a0 71 4b 00 	mov    r9,QWORD PTR [rip+0x4b71a0]        # 0x1805e1300 ; ?White@Color@g@@2V12@B
   18012a160:	4c 8d 45 e0          	lea    r8,[rbp-0x20]
   18012a164:	41 0f 28 cb          	movaps xmm1,xmm11
   18012a168:	48 8d 8d 04 01 00 00 	lea    rcx,[rbp+0x104]
   18012a16f:	ff 15 e3 66 4b 00    	call   QWORD PTR [rip+0x4b66e3]        # 0x1805e0858 ; ?fillSphere@d@@YAXAEBV?$Vector3Template@M@m@@MAEBV?$Matrix4x3Template@M@3@AEBVColor@g@@W4BlendMode@1@W4DepthMode@1@HH@Z
   18012a175:	f3 0f 10 35 3b 4d 55 	movss  xmm6,DWORD PTR [rip+0x554d3b]        # 0x18067eeb8
   18012a17c:	00 
   18012a17d:	48 8d 15 4c c7 16 01 	lea    rdx,[rip+0x116c74c]        # 0x1812968d0
   18012a184:	44 0f 2f 05 e4 d0 16 	comiss xmm8,DWORD PTR [rip+0x116d0e4]        # 0x181297270
   18012a18b:	01 
   18012a18c:	0f 82 f2 00 00 00    	jb     0x18012a284
   18012a192:	48 8d 8d a0 00 00 00 	lea    rcx,[rbp+0xa0]
   18012a199:	e8 02 e8 ee ff       	call   0x1800189a0
   18012a19e:	f2 0f 10 76 ec       	movsd  xmm6,QWORD PTR [rsi-0x14]
   18012a1a3:	8b 46 f4             	mov    eax,DWORD PTR [rsi-0xc]
   18012a1a6:	89 85 18 01 00 00    	mov    DWORD PTR [rbp+0x118],eax
   18012a1ac:	f3 0f 10 8d a0 00 00 	movss  xmm1,DWORD PTR [rbp+0xa0]
   18012a1b3:	00 
   18012a1b4:	f2 0f 11 b5 10 01 00 	movsd  QWORD PTR [rbp+0x110],xmm6
   18012a1bb:	00 
   18012a1bc:	f3 0f 59 ce          	mulss  xmm1,xmm6
   18012a1c0:	f3 0f 10 bd ac 00 00 	movss  xmm7,DWORD PTR [rbp+0xac]
   18012a1c7:	00 
   18012a1c8:	f3 0f 10 ad 14 01 00 	movss  xmm5,DWORD PTR [rbp+0x114]
   18012a1cf:	00 
   18012a1d0:	f3 0f 59 fd          	mulss  xmm7,xmm5
   18012a1d4:	f3 0f 10 85 b8 00 00 	movss  xmm0,DWORD PTR [rbp+0xb8]
   18012a1db:	00 
   18012a1dc:	f3 0f 10 a5 18 01 00 	movss  xmm4,DWORD PTR [rbp+0x118]
   18012a1e3:	00 
   18012a1e4:	f3 0f 59 c4          	mulss  xmm0,xmm4
   18012a1e8:	f3 0f 58 f9          	addss  xmm7,xmm1
   18012a1ec:	f3 0f 58 f8          	addss  xmm7,xmm0
   18012a1f0:	f3 0f 58 bd c4 00 00 	addss  xmm7,DWORD PTR [rbp+0xc4]
   18012a1f7:	00 
   18012a1f8:	f3 0f 10 8d a4 00 00 	movss  xmm1,DWORD PTR [rbp+0xa4]
   18012a1ff:	00 
   18012a200:	f3 0f 59 ce          	mulss  xmm1,xmm6
   18012a204:	f3 0f 10 9d b0 00 00 	movss  xmm3,DWORD PTR [rbp+0xb0]
   18012a20b:	00 
   18012a20c:	f3 0f 59 dd          	mulss  xmm3,xmm5
   18012a210:	f3 0f 10 85 bc 00 00 	movss  xmm0,DWORD PTR [rbp+0xbc]
   18012a217:	00 
   18012a218:	f3 0f 59 c4          	mulss  xmm0,xmm4
   18012a21c:	f3 0f 58 d9          	addss  xmm3,xmm1
   18012a220:	f3 0f 58 d8          	addss  xmm3,xmm0
   18012a224:	f3 0f 58 9d c8 00 00 	addss  xmm3,DWORD PTR [rbp+0xc8]
   18012a22b:	00 
   18012a22c:	f3 0f 10 95 a8 00 00 	movss  xmm2,DWORD PTR [rbp+0xa8]
   18012a233:	00 
   18012a234:	f3 0f 59 d6          	mulss  xmm2,xmm6
   18012a238:	f3 0f 10 8d b4 00 00 	movss  xmm1,DWORD PTR [rbp+0xb4]
   18012a23f:	00 
   18012a240:	f3 0f 59 cd          	mulss  xmm1,xmm5
   18012a244:	f3 0f 10 85 c0 00 00 	movss  xmm0,DWORD PTR [rbp+0xc0]
   18012a24b:	00 
   18012a24c:	f3 0f 59 c4          	mulss  xmm0,xmm4
   18012a250:	f3 0f 58 d1          	addss  xmm2,xmm1
   18012a254:	f3 0f 58 d0          	addss  xmm2,xmm0
   18012a258:	f3 0f 58 95 cc 00 00 	addss  xmm2,DWORD PTR [rbp+0xcc]
   18012a25f:	00 
   18012a260:	f3 0f 11 bd 10 01 00 	movss  DWORD PTR [rbp+0x110],xmm7
   18012a267:	00 
   18012a268:	f3 0f 11 9d 14 01 00 	movss  DWORD PTR [rbp+0x114],xmm3
   18012a26f:	00 
   18012a270:	f3 0f 11 95 18 01 00 	movss  DWORD PTR [rbp+0x118],xmm2
   18012a277:	00 
   18012a278:	48 8d 85 10 01 00 00 	lea    rax,[rbp+0x110]
   18012a27f:	e9 8c 01 00 00       	jmp    0x18012a410
   18012a284:	48 8d 4d e0          	lea    rcx,[rbp-0x20]
   18012a288:	e8 13 e7 ee ff       	call   0x1800189a0
   18012a28d:	f2 0f 10 46 ec       	movsd  xmm0,QWORD PTR [rsi-0x14]
   18012a292:	8b 46 f4             	mov    eax,DWORD PTR [rsi-0xc]
   18012a295:	89 85 d8 01 00 00    	mov    DWORD PTR [rbp+0x1d8],eax
   18012a29b:	44 0f 28 c8          	movaps xmm9,xmm0
   18012a29f:	f3 44 0f 5c 4e e0    	subss  xmm9,DWORD PTR [rsi-0x20]
   18012a2a5:	44 0f 28 c0          	movaps xmm8,xmm0
   18012a2a9:	45 0f c6 c0 55       	shufps xmm8,xmm8,0x55
   18012a2ae:	f2 0f 11 85 d0 01 00 	movsd  QWORD PTR [rbp+0x1d0],xmm0
   18012a2b5:	00 
   18012a2b6:	f3 44 0f 5c 46 e4    	subss  xmm8,DWORD PTR [rsi-0x1c]
   18012a2bc:	f3 0f 10 bd d8 01 00 	movss  xmm7,DWORD PTR [rbp+0x1d8]
   18012a2c3:	00 
   18012a2c4:	f3 0f 5c 7e e8       	subss  xmm7,DWORD PTR [rsi-0x18]
   18012a2c9:	41 0f 28 d0          	movaps xmm2,xmm8
   18012a2cd:	f3 41 0f 59 d0       	mulss  xmm2,xmm8
   18012a2d2:	41 0f 28 c1          	movaps xmm0,xmm9
   18012a2d6:	f3 41 0f 59 c1       	mulss  xmm0,xmm9
   18012a2db:	f3 0f 58 d0          	addss  xmm2,xmm0
   18012a2df:	0f 28 c7             	movaps xmm0,xmm7
   18012a2e2:	f3 0f 59 c7          	mulss  xmm0,xmm7
   18012a2e6:	f3 0f 58 c2          	addss  xmm0,xmm2
   18012a2ea:	0f 57 ed             	xorps  xmm5,xmm5
   18012a2ed:	f3 0f 10 e8          	movss  xmm5,xmm0
   18012a2f1:	0f 57 e4             	xorps  xmm4,xmm4
   18012a2f4:	0f 28 c6             	movaps xmm0,xmm6
   18012a2f7:	0f 28 f0             	movaps xmm6,xmm0
   18012a2fa:	41 0f 28 c2          	movaps xmm0,xmm10
   18012a2fe:	0f 28 d8             	movaps xmm3,xmm0
   18012a301:	f3 0f 52 d5          	rsqrtss xmm2,xmm5
   18012a305:	0f 28 cd             	movaps xmm1,xmm5
   18012a308:	f3 0f 59 ca          	mulss  xmm1,xmm2
   18012a30c:	f3 0f 59 ca          	mulss  xmm1,xmm2
   18012a310:	f3 0f 5c d9          	subss  xmm3,xmm1
   18012a314:	f3 0f 59 f2          	mulss  xmm6,xmm2
   18012a318:	f3 0f 59 f3          	mulss  xmm6,xmm3
   18012a31c:	f3 0f c2 ec 00       	cmpeqss xmm5,xmm4
   18012a321:	0f 28 c5             	movaps xmm0,xmm5
   18012a324:	66 0f 38 14 f4       	blendvps xmm6,xmm4,xmm0
   18012a329:	f3 44 0f 59 ce       	mulss  xmm9,xmm6
   18012a32e:	f3 0f 10 05 3a cf 16 	movss  xmm0,DWORD PTR [rip+0x116cf3a]        # 0x181297270
   18012a335:	01 
   18012a336:	f3 44 0f 59 c8       	mulss  xmm9,xmm0
   18012a33b:	f3 44 0f 59 c6       	mulss  xmm8,xmm6
   18012a340:	f3 44 0f 59 c0       	mulss  xmm8,xmm0
   18012a345:	f3 0f 59 fe          	mulss  xmm7,xmm6
   18012a349:	f3 0f 59 f8          	mulss  xmm7,xmm0
   18012a34d:	f2 0f 10 4e e0       	movsd  xmm1,QWORD PTR [rsi-0x20]
   18012a352:	8b 46 e8             	mov    eax,DWORD PTR [rsi-0x18]
   18012a355:	89 45 d8             	mov    DWORD PTR [rbp-0x28],eax
   18012a358:	f3 44 0f 58 c9       	addss  xmm9,xmm1
   18012a35d:	0f 28 c1             	movaps xmm0,xmm1
   18012a360:	0f c6 c0 55          	shufps xmm0,xmm0,0x55
   18012a364:	f2 0f 11 4d d0       	movsd  QWORD PTR [rbp-0x30],xmm1
   18012a369:	f3 44 0f 58 c0       	addss  xmm8,xmm0
   18012a36e:	f3 0f 58 7d d8       	addss  xmm7,DWORD PTR [rbp-0x28]
   18012a373:	f3 0f 10 65 e0       	movss  xmm4,DWORD PTR [rbp-0x20]
   18012a378:	f3 41 0f 59 e1       	mulss  xmm4,xmm9
   18012a37d:	f3 0f 10 4d ec       	movss  xmm1,DWORD PTR [rbp-0x14]
   18012a382:	f3 41 0f 59 c8       	mulss  xmm1,xmm8
   18012a387:	f3 0f 10 45 f8       	movss  xmm0,DWORD PTR [rbp-0x8]
   18012a38c:	f3 0f 59 c7          	mulss  xmm0,xmm7
   18012a390:	f3 0f 58 e1          	addss  xmm4,xmm1
   18012a394:	f3 0f 58 e0          	addss  xmm4,xmm0
   18012a398:	f3 0f 58 65 04       	addss  xmm4,DWORD PTR [rbp+0x4]
   18012a39d:	f3 0f 10 5d e4       	movss  xmm3,DWORD PTR [rbp-0x1c]
   18012a3a2:	f3 41 0f 59 d9       	mulss  xmm3,xmm9
   18012a3a7:	f3 0f 10 4d f0       	movss  xmm1,DWORD PTR [rbp-0x10]
   18012a3ac:	f3 41 0f 59 c8       	mulss  xmm1,xmm8
   18012a3b1:	f3 0f 10 45 fc       	movss  xmm0,DWORD PTR [rbp-0x4]
   18012a3b6:	f3 0f 59 c7          	mulss  xmm0,xmm7
   18012a3ba:	f3 0f 58 d9          	addss  xmm3,xmm1
   18012a3be:	f3 0f 58 d8          	addss  xmm3,xmm0
   18012a3c2:	f3 0f 58 5d 08       	addss  xmm3,DWORD PTR [rbp+0x8]
   18012a3c7:	f3 0f 10 4d e8       	movss  xmm1,DWORD PTR [rbp-0x18]
   18012a3cc:	f3 41 0f 59 c9       	mulss  xmm1,xmm9
   18012a3d1:	f3 0f 10 55 f4       	movss  xmm2,DWORD PTR [rbp-0xc]
   18012a3d6:	f3 41 0f 59 d0       	mulss  xmm2,xmm8
   18012a3db:	f3 0f 10 45 00       	movss  xmm0,DWORD PTR [rbp+0x0]
   18012a3e0:	f3 0f 59 c7          	mulss  xmm0,xmm7
   18012a3e4:	f3 0f 58 d1          	addss  xmm2,xmm1
   18012a3e8:	f3 0f 58 d0          	addss  xmm2,xmm0
   18012a3ec:	f3 0f 58 55 0c       	addss  xmm2,DWORD PTR [rbp+0xc]
   18012a3f1:	f3 0f 11 a5 80 02 00 	movss  DWORD PTR [rbp+0x280],xmm4
   18012a3f8:	00 
   18012a3f9:	f3 0f 11 9d 84 02 00 	movss  DWORD PTR [rbp+0x284],xmm3
   18012a400:	00 
   18012a401:	f3 0f 11 95 88 02 00 	movss  DWORD PTR [rbp+0x288],xmm2
   18012a408:	00 
   18012a409:	48 8d 85 80 02 00 00 	lea    rax,[rbp+0x280]
   18012a410:	f2 0f 10 00          	movsd  xmm0,QWORD PTR [rax]
   18012a414:	8b 40 08             	mov    eax,DWORD PTR [rax+0x8]
   18012a417:	89 85 78 02 00 00    	mov    DWORD PTR [rbp+0x278],eax
   18012a41d:	f3 44 0f 10 b5 78 02 	movss  xmm14,DWORD PTR [rbp+0x278]
   18012a424:	00 00 
   18012a426:	44 0f 28 e8          	movaps xmm13,xmm0
   18012a42a:	45 0f c6 ed 55       	shufps xmm13,xmm13,0x55
   18012a42f:	f2 0f 11 85 70 02 00 	movsd  QWORD PTR [rbp+0x270],xmm0
   18012a436:	00 
   18012a437:	f3 44 0f 10 bd 70 02 	movss  xmm15,DWORD PTR [rbp+0x270]
   18012a43e:	00 00 
   18012a440:	80 3d a9 c9 16 01 00 	cmp    BYTE PTR [rip+0x116c9a9],0x0        # 0x181296df0
   18012a447:	0f 84 3e 05 00 00    	je     0x18012a98b
   18012a44d:	45 33 f6             	xor    r14d,r14d
   18012a450:	f3 0f 10 05 60 4a 55 	movss  xmm0,DWORD PTR [rip+0x554a60]        # 0x18067eeb8
   18012a457:	00 
   18012a458:	0f 29 45 b0          	movaps XMMWORD PTR [rbp-0x50],xmm0
   18012a45c:	41 0f 28 c2          	movaps xmm0,xmm10
   18012a460:	0f 29 85 e0 01 00 00 	movaps XMMWORD PTR [rbp+0x1e0],xmm0
   18012a467:	45 33 e4             	xor    r12d,r12d
   18012a46a:	48 8d 15 5f c4 16 01 	lea    rdx,[rip+0x116c45f]        # 0x1812968d0
   18012a471:	48 8d 4d e0          	lea    rcx,[rbp-0x20]
   18012a475:	e8 26 e5 ee ff       	call   0x1800189a0
   18012a47a:	f2 0f 10 66 ec       	movsd  xmm4,QWORD PTR [rsi-0x14]
   18012a47f:	8b 46 f4             	mov    eax,DWORD PTR [rsi-0xc]
   18012a482:	89 85 58 02 00 00    	mov    DWORD PTR [rbp+0x258],eax
   18012a488:	f3 44 0f 10 5d e0    	movss  xmm11,DWORD PTR [rbp-0x20]
   18012a48e:	f2 0f 11 a5 50 02 00 	movsd  QWORD PTR [rbp+0x250],xmm4
   18012a495:	00 
   18012a496:	f3 44 0f 59 dc       	mulss  xmm11,xmm4
   18012a49b:	f3 0f 10 4d ec       	movss  xmm1,DWORD PTR [rbp-0x14]
   18012a4a0:	f3 0f 10 9d 54 02 00 	movss  xmm3,DWORD PTR [rbp+0x254]
   18012a4a7:	00 
   18012a4a8:	f3 0f 59 cb          	mulss  xmm1,xmm3
   18012a4ac:	f3 0f 10 45 f8       	movss  xmm0,DWORD PTR [rbp-0x8]
   18012a4b1:	f3 0f 10 95 58 02 00 	movss  xmm2,DWORD PTR [rbp+0x258]
   18012a4b8:	00 
   18012a4b9:	f3 0f 59 c2          	mulss  xmm0,xmm2
   18012a4bd:	f3 44 0f 58 d9       	addss  xmm11,xmm1
   18012a4c2:	f3 44 0f 58 d8       	addss  xmm11,xmm0
   18012a4c7:	f3 44 0f 58 5d 04    	addss  xmm11,DWORD PTR [rbp+0x4]
   18012a4cd:	f3 44 0f 10 4d e4    	movss  xmm9,DWORD PTR [rbp-0x1c]
   18012a4d3:	f3 44 0f 59 cc       	mulss  xmm9,xmm4
   18012a4d8:	f3 0f 10 4d f0       	movss  xmm1,DWORD PTR [rbp-0x10]
   18012a4dd:	f3 0f 59 cb          	mulss  xmm1,xmm3
   18012a4e1:	f3 0f 10 45 fc       	movss  xmm0,DWORD PTR [rbp-0x4]
   18012a4e6:	f3 0f 59 c2          	mulss  xmm0,xmm2
   18012a4ea:	f3 44 0f 58 c9       	addss  xmm9,xmm1
   18012a4ef:	f3 44 0f 58 c8       	addss  xmm9,xmm0
   18012a4f4:	f3 44 0f 58 4d 08    	addss  xmm9,DWORD PTR [rbp+0x8]
   18012a4fa:	f3 0f 10 75 e8       	movss  xmm6,DWORD PTR [rbp-0x18]
   18012a4ff:	f3 0f 59 f4          	mulss  xmm6,xmm4
   18012a503:	f3 0f 10 4d f4       	movss  xmm1,DWORD PTR [rbp-0xc]
   18012a508:	f3 0f 59 cb          	mulss  xmm1,xmm3
   18012a50c:	f3 0f 10 45 00       	movss  xmm0,DWORD PTR [rbp+0x0]
   18012a511:	f3 0f 59 c2          	mulss  xmm0,xmm2
   18012a515:	f3 0f 58 f1          	addss  xmm6,xmm1
   18012a519:	f3 0f 58 f0          	addss  xmm6,xmm0
   18012a51d:	f3 0f 58 75 0c       	addss  xmm6,DWORD PTR [rbp+0xc]
   18012a522:	48 8d 15 a7 c3 16 01 	lea    rdx,[rip+0x116c3a7]        # 0x1812968d0
   18012a529:	48 8d 8d a0 00 00 00 	lea    rcx,[rbp+0xa0]
   18012a530:	e8 6b e4 ee ff       	call   0x1800189a0
   18012a535:	f2 0f 10 66 e0       	movsd  xmm4,QWORD PTR [rsi-0x20]
   18012a53a:	8b 46 e8             	mov    eax,DWORD PTR [rsi-0x18]
   18012a53d:	89 85 a8 02 00 00    	mov    DWORD PTR [rbp+0x2a8],eax
   18012a543:	f3 0f 10 8d a0 00 00 	movss  xmm1,DWORD PTR [rbp+0xa0]
   18012a54a:	00 
   18012a54b:	f2 0f 11 a5 a0 02 00 	movsd  QWORD PTR [rbp+0x2a0],xmm4
   18012a552:	00 
   18012a553:	f3 0f 59 cc          	mulss  xmm1,xmm4
   18012a557:	f3 44 0f 10 95 ac 00 	movss  xmm10,DWORD PTR [rbp+0xac]
   18012a55e:	00 00 
   18012a560:	f3 0f 10 9d a4 02 00 	movss  xmm3,DWORD PTR [rbp+0x2a4]
   18012a567:	00 
   18012a568:	f3 44 0f 59 d3       	mulss  xmm10,xmm3
   18012a56d:	f3 0f 10 85 b8 00 00 	movss  xmm0,DWORD PTR [rbp+0xb8]
   18012a574:	00 
   18012a575:	f3 0f 10 95 a8 02 00 	movss  xmm2,DWORD PTR [rbp+0x2a8]
   18012a57c:	00 
   18012a57d:	f3 0f 59 c2          	mulss  xmm0,xmm2
   18012a581:	f3 44 0f 58 d1       	addss  xmm10,xmm1
   18012a586:	f3 44 0f 58 d0       	addss  xmm10,xmm0
   18012a58b:	f3 44 0f 58 95 c4 00 	addss  xmm10,DWORD PTR [rbp+0xc4]
   18012a592:	00 00 
   18012a594:	f3 0f 10 8d a4 00 00 	movss  xmm1,DWORD PTR [rbp+0xa4]
   18012a59b:	00 
   18012a59c:	f3 0f 59 cc          	mulss  xmm1,xmm4
   18012a5a0:	f3 44 0f 10 85 b0 00 	movss  xmm8,DWORD PTR [rbp+0xb0]
   18012a5a7:	00 00 
   18012a5a9:	f3 44 0f 59 c3       	mulss  xmm8,xmm3
   18012a5ae:	f3 0f 10 85 bc 00 00 	movss  xmm0,DWORD PTR [rbp+0xbc]
   18012a5b5:	00 
   18012a5b6:	f3 0f 59 c2          	mulss  xmm0,xmm2
   18012a5ba:	f3 44 0f 58 c1       	addss  xmm8,xmm1
   18012a5bf:	f3 44 0f 58 c0       	addss  xmm8,xmm0
   18012a5c4:	f3 44 0f 58 85 c8 00 	addss  xmm8,DWORD PTR [rbp+0xc8]
   18012a5cb:	00 00 
   18012a5cd:	f3 0f 10 8d a8 00 00 	movss  xmm1,DWORD PTR [rbp+0xa8]
   18012a5d4:	00 
   18012a5d5:	f3 0f 59 cc          	mulss  xmm1,xmm4
   18012a5d9:	f3 0f 10 bd b4 00 00 	movss  xmm7,DWORD PTR [rbp+0xb4]
   18012a5e0:	00 
   18012a5e1:	f3 0f 59 fb          	mulss  xmm7,xmm3
   18012a5e5:	f3 0f 10 85 c0 00 00 	movss  xmm0,DWORD PTR [rbp+0xc0]
   18012a5ec:	00 
   18012a5ed:	f3 0f 59 c2          	mulss  xmm0,xmm2
   18012a5f1:	f3 0f 58 f9          	addss  xmm7,xmm1
   18012a5f5:	f3 0f 58 f8          	addss  xmm7,xmm0
   18012a5f9:	f3 0f 58 bd cc 00 00 	addss  xmm7,DWORD PTR [rbp+0xcc]
   18012a600:	00 
   18012a601:	f3 45 0f 5c d3       	subss  xmm10,xmm11
   18012a606:	f3 45 0f 5c c1       	subss  xmm8,xmm9
   18012a60b:	f3 0f 5c fe          	subss  xmm7,xmm6
   18012a60f:	41 0f 28 d0          	movaps xmm2,xmm8
   18012a613:	f3 41 0f 59 d0       	mulss  xmm2,xmm8
   18012a618:	41 0f 28 c2          	movaps xmm0,xmm10
   18012a61c:	f3 41 0f 59 c2       	mulss  xmm0,xmm10
   18012a621:	f3 0f 58 d0          	addss  xmm2,xmm0
   18012a625:	0f 28 c7             	movaps xmm0,xmm7
   18012a628:	f3 0f 59 c7          	mulss  xmm0,xmm7
   18012a62c:	f3 0f 58 c2          	addss  xmm0,xmm2
   18012a630:	0f 28 f0             	movaps xmm6,xmm0
   18012a633:	0f 57 ed             	xorps  xmm5,xmm5
   18012a636:	f3 0f 52 de          	rsqrtss xmm3,xmm6
   18012a63a:	0f 28 c8             	movaps xmm1,xmm0
   18012a63d:	f3 0f 59 cb          	mulss  xmm1,xmm3
   18012a641:	f3 0f 59 cb          	mulss  xmm1,xmm3
   18012a645:	0f 28 95 e0 01 00 00 	movaps xmm2,XMMWORD PTR [rbp+0x1e0]
   18012a64c:	f3 0f 5c d1          	subss  xmm2,xmm1
   18012a650:	0f 28 65 b0          	movaps xmm4,XMMWORD PTR [rbp-0x50]
   18012a654:	f3 0f 59 e3          	mulss  xmm4,xmm3
   18012a658:	f3 0f 59 e2          	mulss  xmm4,xmm2
   18012a65c:	f3 0f c2 f5 00       	cmpeqss xmm6,xmm5
   18012a661:	0f 28 c6             	movaps xmm0,xmm6
   18012a664:	66 0f 38 14 e5       	blendvps xmm4,xmm5,xmm0
   18012a669:	f3 44 0f 59 d4       	mulss  xmm10,xmm4
   18012a66e:	f3 44 0f 11 95 90 02 	movss  DWORD PTR [rbp+0x290],xmm10
   18012a675:	00 00 
   18012a677:	f3 44 0f 59 c4       	mulss  xmm8,xmm4
   18012a67c:	f3 44 0f 11 85 94 02 	movss  DWORD PTR [rbp+0x294],xmm8
   18012a683:	00 00 
   18012a685:	f3 0f 59 fc          	mulss  xmm7,xmm4
   18012a689:	f3 0f 11 bd 98 02 00 	movss  DWORD PTR [rbp+0x298],xmm7
   18012a690:	00 
   18012a691:	48 c7 85 60 01 00 00 	mov    QWORD PTR [rbp+0x160],0x0
   18012a698:	00 00 00 00 
   18012a69c:	c7 85 68 01 00 00 00 	mov    DWORD PTR [rbp+0x168],0x0
   18012a6a3:	00 00 00 
   18012a6a6:	48 c7 85 70 01 00 00 	mov    QWORD PTR [rbp+0x170],0x0
   18012a6ad:	00 00 00 00 
   18012a6b1:	c7 85 78 01 00 00 00 	mov    DWORD PTR [rbp+0x178],0x0
   18012a6b8:	00 00 00 
   18012a6bb:	4c 8d 85 70 01 00 00 	lea    r8,[rbp+0x170]
   18012a6c2:	48 8d 95 60 01 00 00 	lea    rdx,[rbp+0x160]
   18012a6c9:	48 8d 8d 90 02 00 00 	lea    rcx,[rbp+0x290]
   18012a6d0:	e8 0b 00 01 00       	call   0x18013a6e0
   18012a6d5:	48 8b 85 50 01 00 00 	mov    rax,QWORD PTR [rbp+0x150]
   18012a6dc:	f3 42 0f 10 54 20 04 	movss  xmm2,DWORD PTR [rax+r12*1+0x4]
   18012a6e3:	f3 42 0f 10 0c 20    	movss  xmm1,DWORD PTR [rax+r12*1]
   18012a6e9:	f3 44 0f 10 85 60 01 	movss  xmm8,DWORD PTR [rbp+0x160]
   18012a6f0:	00 00 
   18012a6f2:	f3 44 0f 59 c1       	mulss  xmm8,xmm1
   18012a6f7:	0f 28 c2             	movaps xmm0,xmm2
   18012a6fa:	f3 0f 59 85 70 01 00 	mulss  xmm0,DWORD PTR [rbp+0x170]
   18012a701:	00 
   18012a702:	f3 44 0f 58 c0       	addss  xmm8,xmm0
   18012a707:	f3 44 0f 10 8d 64 01 	movss  xmm9,DWORD PTR [rbp+0x164]
   18012a70e:	00 00 
   18012a710:	f3 44 0f 59 c9       	mulss  xmm9,xmm1
   18012a715:	0f 28 c2             	movaps xmm0,xmm2
   18012a718:	f3 0f 59 85 74 01 00 	mulss  xmm0,DWORD PTR [rbp+0x174]
   18012a71f:	00 
   18012a720:	f3 44 0f 58 c8       	addss  xmm9,xmm0
   18012a725:	f3 44 0f 10 95 68 01 	movss  xmm10,DWORD PTR [rbp+0x168]
   18012a72c:	00 00 
   18012a72e:	f3 44 0f 59 d1       	mulss  xmm10,xmm1
   18012a733:	f3 0f 59 95 78 01 00 	mulss  xmm2,DWORD PTR [rbp+0x178]
   18012a73a:	00 
   18012a73b:	f3 44 0f 58 d2       	addss  xmm10,xmm2
   18012a740:	66 41 0f 6e c6       	movd   xmm0,r14d
   18012a745:	0f 5b c0             	cvtdq2ps xmm0,xmm0
   18012a748:	f3 0f 59 05 ec 45 55 	mulss  xmm0,DWORD PTR [rip+0x5545ec]        # 0x18067ed3c
   18012a74f:	00 
   18012a750:	f3 44 0f 59 c0       	mulss  xmm8,xmm0
   18012a755:	f3 44 0f 59 c8       	mulss  xmm9,xmm0
   18012a75a:	f3 44 0f 59 d0       	mulss  xmm10,xmm0
   18012a75f:	80 3d ca c8 16 01 00 	cmp    BYTE PTR [rip+0x116c8ca],0x0        # 0x181297030
   18012a766:	0f 84 b9 00 00 00    	je     0x18012a825
   18012a76c:	f3 0f 10 4e f8       	movss  xmm1,DWORD PTR [rsi-0x8]
   18012a771:	f3 41 0f 59 cc       	mulss  xmm1,xmm12
   18012a776:	0f 28 d1             	movaps xmm2,xmm1
   18012a779:	66 0f 3a 0a d2 0c    	roundss xmm2,xmm2,0xc
   18012a77f:	f3 0f 2d ca          	cvtss2si ecx,xmm2
   18012a783:	85 c9                	test   ecx,ecx
   18012a785:	79 09                	jns    0x18012a790
   18012a787:	c6 85 02 04 00 00 00 	mov    BYTE PTR [rbp+0x402],0x0
   18012a78e:	eb 10                	jmp    0x18012a7a0
   18012a790:	0f b6 c1             	movzx  eax,cl
   18012a793:	41 3b cf             	cmp    ecx,r15d
   18012a796:	41 0f 4f c7          	cmovg  eax,r15d
   18012a79a:	88 85 02 04 00 00    	mov    BYTE PTR [rbp+0x402],al
   18012a7a0:	f3 0f 10 4e fc       	movss  xmm1,DWORD PTR [rsi-0x4]
   18012a7a5:	f3 41 0f 59 cc       	mulss  xmm1,xmm12
   18012a7aa:	0f 28 d1             	movaps xmm2,xmm1
   18012a7ad:	66 0f 3a 0a d2 0c    	roundss xmm2,xmm2,0xc
   18012a7b3:	f3 0f 2d ca          	cvtss2si ecx,xmm2
   18012a7b7:	85 c9                	test   ecx,ecx
   18012a7b9:	79 09                	jns    0x18012a7c4
   18012a7bb:	c6 85 01 04 00 00 00 	mov    BYTE PTR [rbp+0x401],0x0
   18012a7c2:	eb 10                	jmp    0x18012a7d4
   18012a7c4:	0f b6 c1             	movzx  eax,cl
   18012a7c7:	41 3b cf             	cmp    ecx,r15d
   18012a7ca:	41 0f 4f c7          	cmovg  eax,r15d
   18012a7ce:	88 85 01 04 00 00    	mov    BYTE PTR [rbp+0x401],al
   18012a7d4:	f3 0f 10 0e          	movss  xmm1,DWORD PTR [rsi]
   18012a7d8:	f3 41 0f 59 cc       	mulss  xmm1,xmm12
   18012a7dd:	0f 28 d1             	movaps xmm2,xmm1
   18012a7e0:	66 0f 3a 0a d2 0c    	roundss xmm2,xmm2,0xc
   18012a7e6:	f3 0f 2d ca          	cvtss2si ecx,xmm2
   18012a7ea:	85 c9                	test   ecx,ecx
   18012a7ec:	79 17                	jns    0x18012a805
   18012a7ee:	c6 85 00 04 00 00 00 	mov    BYTE PTR [rbp+0x400],0x0
   18012a7f5:	44 88 bd 03 04 00 00 	mov    BYTE PTR [rbp+0x403],r15b
   18012a7fc:	48 8d bd 00 04 00 00 	lea    rdi,[rbp+0x400]
   18012a803:	eb 36                	jmp    0x18012a83b
   18012a805:	0f b6 c1             	movzx  eax,cl
   18012a808:	41 3b cf             	cmp    ecx,r15d
   18012a80b:	41 0f 4f c7          	cmovg  eax,r15d
   18012a80f:	88 85 00 04 00 00    	mov    BYTE PTR [rbp+0x400],al
   18012a815:	44 88 bd 03 04 00 00 	mov    BYTE PTR [rbp+0x403],r15b
   18012a81c:	48 8d bd 00 04 00 00 	lea    rdi,[rbp+0x400]
   18012a823:	eb 16                	jmp    0x18012a83b
   18012a825:	48 8b 05 d4 6a 4b 00 	mov    rax,QWORD PTR [rip+0x4b6ad4]        # 0x1805e1300 ; ?White@Color@g@@2V12@B
   18012a82c:	8b 08                	mov    ecx,DWORD PTR [rax]
   18012a82e:	89 8d c0 01 00 00    	mov    DWORD PTR [rbp+0x1c0],ecx
   18012a834:	48 8d bd c0 01 00 00 	lea    rdi,[rbp+0x1c0]
   18012a83b:	41 0f 28 c0          	movaps xmm0,xmm8
   18012a83f:	f3 41 0f 58 c7       	addss  xmm0,xmm15
   18012a844:	f3 0f 11 85 b8 02 00 	movss  DWORD PTR [rbp+0x2b8],xmm0
   18012a84b:	00 
   18012a84c:	41 0f 28 c9          	movaps xmm1,xmm9
   18012a850:	f3 41 0f 58 cd       	addss  xmm1,xmm13
   18012a855:	f3 0f 11 8d bc 02 00 	movss  DWORD PTR [rbp+0x2bc],xmm1
   18012a85c:	00 
   18012a85d:	41 0f 28 c2          	movaps xmm0,xmm10
   18012a861:	f3 41 0f 58 c6       	addss  xmm0,xmm14
   18012a866:	f3 0f 11 85 c0 02 00 	movss  DWORD PTR [rbp+0x2c0],xmm0
   18012a86d:	00 
   18012a86e:	48 8d 15 5b c0 16 01 	lea    rdx,[rip+0x116c05b]        # 0x1812968d0
   18012a875:	48 8d 4d e0          	lea    rcx,[rbp-0x20]
   18012a879:	e8 22 e1 ee ff       	call   0x1800189a0
   18012a87e:	f2 0f 10 76 e0       	movsd  xmm6,QWORD PTR [rsi-0x20]
   18012a883:	8b 46 e8             	mov    eax,DWORD PTR [rsi-0x18]
   18012a886:	89 85 b4 02 00 00    	mov    DWORD PTR [rbp+0x2b4],eax
   18012a88c:	f3 0f 10 7d e0       	movss  xmm7,DWORD PTR [rbp-0x20]
   18012a891:	f2 0f 11 b5 ac 02 00 	movsd  QWORD PTR [rbp+0x2ac],xmm6
   18012a898:	00 
   18012a899:	f3 0f 59 fe          	mulss  xmm7,xmm6
   18012a89d:	f3 0f 10 4d ec       	movss  xmm1,DWORD PTR [rbp-0x14]
   18012a8a2:	f3 0f 10 a5 b0 02 00 	movss  xmm4,DWORD PTR [rbp+0x2b0]
   18012a8a9:	00 
   18012a8aa:	f3 0f 59 cc          	mulss  xmm1,xmm4
   18012a8ae:	f3 0f 10 45 f8       	movss  xmm0,DWORD PTR [rbp-0x8]
   18012a8b3:	f3 0f 10 9d b4 02 00 	movss  xmm3,DWORD PTR [rbp+0x2b4]
   18012a8ba:	00 
   18012a8bb:	f3 0f 59 c3          	mulss  xmm0,xmm3
   18012a8bf:	f3 0f 58 f9          	addss  xmm7,xmm1
   18012a8c3:	f3 0f 58 f8          	addss  xmm7,xmm0
   18012a8c7:	f3 0f 58 7d 04       	addss  xmm7,DWORD PTR [rbp+0x4]
   18012a8cc:	f3 0f 10 6d e4       	movss  xmm5,DWORD PTR [rbp-0x1c]
   18012a8d1:	f3 0f 59 ee          	mulss  xmm5,xmm6
   18012a8d5:	f3 0f 10 4d f0       	movss  xmm1,DWORD PTR [rbp-0x10]
   18012a8da:	f3 0f 59 cc          	mulss  xmm1,xmm4
   18012a8de:	f3 0f 10 45 fc       	movss  xmm0,DWORD PTR [rbp-0x4]
   18012a8e3:	f3 0f 59 c3          	mulss  xmm0,xmm3
   18012a8e7:	f3 0f 58 e9          	addss  xmm5,xmm1
   18012a8eb:	f3 0f 58 e8          	addss  xmm5,xmm0
   18012a8ef:	f3 0f 58 6d 08       	addss  xmm5,DWORD PTR [rbp+0x8]
   18012a8f4:	f3 0f 10 55 e8       	movss  xmm2,DWORD PTR [rbp-0x18]
   18012a8f9:	f3 0f 59 d6          	mulss  xmm2,xmm6
   18012a8fd:	f3 0f 10 4d f4       	movss  xmm1,DWORD PTR [rbp-0xc]
   18012a902:	f3 0f 59 cc          	mulss  xmm1,xmm4
   18012a906:	f3 0f 10 45 00       	movss  xmm0,DWORD PTR [rbp+0x0]
   18012a90b:	f3 0f 59 c3          	mulss  xmm0,xmm3
   18012a90f:	f3 0f 58 d1          	addss  xmm2,xmm1
   18012a913:	f3 0f 58 d0          	addss  xmm2,xmm0
   18012a917:	f3 0f 58 55 0c       	addss  xmm2,DWORD PTR [rbp+0xc]
   18012a91c:	f3 44 0f 58 c7       	addss  xmm8,xmm7
   18012a921:	f3 44 0f 11 85 c4 02 	movss  DWORD PTR [rbp+0x2c4],xmm8
   18012a928:	00 00 
   18012a92a:	f3 44 0f 58 cd       	addss  xmm9,xmm5
   18012a92f:	f3 44 0f 11 8d c8 02 	movss  DWORD PTR [rbp+0x2c8],xmm9
   18012a936:	00 00 
   18012a938:	f3 44 0f 58 d2       	addss  xmm10,xmm2
   18012a93d:	f3 44 0f 11 95 cc 02 	movss  DWORD PTR [rbp+0x2cc],xmm10
   18012a944:	00 00 
   18012a946:	c7 44 24 20 01 00 00 	mov    DWORD PTR [rsp+0x20],0x1
   18012a94d:	00 
   18012a94e:	45 33 c9             	xor    r9d,r9d
   18012a951:	4c 8b c7             	mov    r8,rdi
   18012a954:	48 8d 95 b8 02 00 00 	lea    rdx,[rbp+0x2b8]
   18012a95b:	48 8d 8d c4 02 00 00 	lea    rcx,[rbp+0x2c4]
   18012a962:	ff 15 b0 5d 4b 00    	call   QWORD PTR [rip+0x4b5db0]        # 0x1805e0718 ; ?drawLine@d@@YAXAEBV?$Vector3Template@M@m@@0AEBVColor@g@@W4BlendMode@1@W4DepthMode@1@@Z
   18012a968:	41 ff c6             	inc    r14d
   18012a96b:	49 83 c4 08          	add    r12,0x8
   18012a96f:	41 83 fe 10          	cmp    r14d,0x10
   18012a973:	0f 82 f1 fa ff ff    	jb     0x18012a46a
   18012a979:	f3 44 0f 10 15 2e 48 	movss  xmm10,DWORD PTR [rip+0x55482e]        # 0x18067f1b0
   18012a980:	55 00 
   18012a982:	f3 44 0f 10 1d e9 43 	movss  xmm11,DWORD PTR [rip+0x5543e9]        # 0x18067ed74
   18012a989:	55 00 
   18012a98b:	44 8b 6e 04          	mov    r13d,DWORD PTR [rsi+0x4]
   18012a98f:	41 81 fd ff ff 00 00 	cmp    r13d,0xffff
   18012a996:	0f 85 66 06 00 00    	jne    0x18012b002
   18012a99c:	80 3d 0d c5 16 01 00 	cmp    BYTE PTR [rip+0x116c50d],0x0        # 0x181296eb0
   18012a9a3:	0f 84 18 05 00 00    	je     0x18012aec1
   18012a9a9:	45 33 f6             	xor    r14d,r14d
   18012a9ac:	f3 0f 10 05 04 45 55 	movss  xmm0,DWORD PTR [rip+0x554504]        # 0x18067eeb8
   18012a9b3:	00 
   18012a9b4:	0f 29 85 e0 01 00 00 	movaps XMMWORD PTR [rbp+0x1e0],xmm0
   18012a9bb:	41 0f 28 c2          	movaps xmm0,xmm10
   18012a9bf:	0f 29 45 b0          	movaps XMMWORD PTR [rbp-0x50],xmm0
   18012a9c3:	45 33 e4             	xor    r12d,r12d
   18012a9c6:	48 8d 15 03 bf 16 01 	lea    rdx,[rip+0x116bf03]        # 0x1812968d0
   18012a9cd:	48 8d 4d e0          	lea    rcx,[rbp-0x20]
   18012a9d1:	e8 ca df ee ff       	call   0x1800189a0
   18012a9d6:	f2 0f 10 66 ec       	movsd  xmm4,QWORD PTR [rsi-0x14]
   18012a9db:	8b 46 f4             	mov    eax,DWORD PTR [rsi-0xc]
   18012a9de:	89 85 d8 02 00 00    	mov    DWORD PTR [rbp+0x2d8],eax
   18012a9e4:	f3 44 0f 10 5d e0    	movss  xmm11,DWORD PTR [rbp-0x20]
   18012a9ea:	f2 0f 11 a5 d0 02 00 	movsd  QWORD PTR [rbp+0x2d0],xmm4
   18012a9f1:	00 
   18012a9f2:	f3 44 0f 59 dc       	mulss  xmm11,xmm4
   18012a9f7:	f3 0f 10 4d ec       	movss  xmm1,DWORD PTR [rbp-0x14]
   18012a9fc:	f3 0f 10 9d d4 02 00 	movss  xmm3,DWORD PTR [rbp+0x2d4]
   18012aa03:	00 
   18012aa04:	f3 0f 59 cb          	mulss  xmm1,xmm3
   18012aa08:	f3 0f 10 45 f8       	movss  xmm0,DWORD PTR [rbp-0x8]
   18012aa0d:	f3 0f 10 95 d8 02 00 	movss  xmm2,DWORD PTR [rbp+0x2d8]
   18012aa14:	00 
   18012aa15:	f3 0f 59 c2          	mulss  xmm0,xmm2
   18012aa19:	f3 44 0f 58 d9       	addss  xmm11,xmm1
   18012aa1e:	f3 44 0f 58 d8       	addss  xmm11,xmm0
   18012aa23:	f3 44 0f 58 5d 04    	addss  xmm11,DWORD PTR [rbp+0x4]
   18012aa29:	f3 44 0f 10 4d e4    	movss  xmm9,DWORD PTR [rbp-0x1c]
   18012aa2f:	f3 44 0f 59 cc       	mulss  xmm9,xmm4
   18012aa34:	f3 0f 10 4d f0       	movss  xmm1,DWORD PTR [rbp-0x10]
   18012aa39:	f3 0f 59 cb          	mulss  xmm1,xmm3
   18012aa3d:	f3 0f 10 45 fc       	movss  xmm0,DWORD PTR [rbp-0x4]
   18012aa42:	f3 0f 59 c2          	mulss  xmm0,xmm2
   18012aa46:	f3 44 0f 58 c9       	addss  xmm9,xmm1
   18012aa4b:	f3 44 0f 58 c8       	addss  xmm9,xmm0
   18012aa50:	f3 44 0f 58 4d 08    	addss  xmm9,DWORD PTR [rbp+0x8]
   18012aa56:	f3 0f 10 75 e8       	movss  xmm6,DWORD PTR [rbp-0x18]
   18012aa5b:	f3 0f 59 f4          	mulss  xmm6,xmm4
   18012aa5f:	f3 0f 10 4d f4       	movss  xmm1,DWORD PTR [rbp-0xc]
   18012aa64:	f3 0f 59 cb          	mulss  xmm1,xmm3
   18012aa68:	f3 0f 10 45 00       	movss  xmm0,DWORD PTR [rbp+0x0]
   18012aa6d:	f3 0f 59 c2          	mulss  xmm0,xmm2
   18012aa71:	f3 0f 58 f1          	addss  xmm6,xmm1
   18012aa75:	f3 0f 58 f0          	addss  xmm6,xmm0
   18012aa79:	f3 0f 58 75 0c       	addss  xmm6,DWORD PTR [rbp+0xc]
   18012aa7e:	48 8d 15 4b be 16 01 	lea    rdx,[rip+0x116be4b]        # 0x1812968d0
   18012aa85:	48 8d 8d a0 00 00 00 	lea    rcx,[rbp+0xa0]
   18012aa8c:	e8 0f df ee ff       	call   0x1800189a0
   18012aa91:	f2 0f 10 66 e0       	movsd  xmm4,QWORD PTR [rsi-0x20]
   18012aa96:	8b 46 e8             	mov    eax,DWORD PTR [rsi-0x18]
   18012aa99:	89 85 08 02 00 00    	mov    DWORD PTR [rbp+0x208],eax
   18012aa9f:	f3 44 0f 10 95 a0 00 	movss  xmm10,DWORD PTR [rbp+0xa0]
   18012aaa6:	00 00 
   18012aaa8:	f2 0f 11 a5 00 02 00 	movsd  QWORD PTR [rbp+0x200],xmm4
   18012aaaf:	00 
   18012aab0:	f3 44 0f 59 d4       	mulss  xmm10,xmm4
   18012aab5:	f3 0f 10 8d ac 00 00 	movss  xmm1,DWORD PTR [rbp+0xac]
   18012aabc:	00 
   18012aabd:	f3 0f 10 9d 04 02 00 	movss  xmm3,DWORD PTR [rbp+0x204]
   18012aac4:	00 
   18012aac5:	f3 0f 59 cb          	mulss  xmm1,xmm3
   18012aac9:	f3 0f 10 85 b8 00 00 	movss  xmm0,DWORD PTR [rbp+0xb8]
   18012aad0:	00 
   18012aad1:	f3 0f 10 95 08 02 00 	movss  xmm2,DWORD PTR [rbp+0x208]
   18012aad8:	00 
   18012aad9:	f3 0f 59 c2          	mulss  xmm0,xmm2
   18012aadd:	f3 44 0f 58 d1       	addss  xmm10,xmm1
   18012aae2:	f3 44 0f 58 d0       	addss  xmm10,xmm0
   18012aae7:	f3 44 0f 58 95 c4 00 	addss  xmm10,DWORD PTR [rbp+0xc4]
   18012aaee:	00 00 
   18012aaf0:	f3 44 0f 10 85 a4 00 	movss  xmm8,DWORD PTR [rbp+0xa4]
   18012aaf7:	00 00 
   18012aaf9:	f3 44 0f 59 c4       	mulss  xmm8,xmm4
   18012aafe:	f3 0f 10 8d b0 00 00 	movss  xmm1,DWORD PTR [rbp+0xb0]
   18012ab05:	00 
   18012ab06:	f3 0f 59 cb          	mulss  xmm1,xmm3
   18012ab0a:	f3 0f 10 85 bc 00 00 	movss  xmm0,DWORD PTR [rbp+0xbc]
   18012ab11:	00 
   18012ab12:	f3 0f 59 c2          	mulss  xmm0,xmm2
   18012ab16:	f3 44 0f 58 c1       	addss  xmm8,xmm1
   18012ab1b:	f3 44 0f 58 c0       	addss  xmm8,xmm0
   18012ab20:	f3 44 0f 58 85 c8 00 	addss  xmm8,DWORD PTR [rbp+0xc8]
   18012ab27:	00 00 
   18012ab29:	f3 0f 10 bd a8 00 00 	movss  xmm7,DWORD PTR [rbp+0xa8]
   18012ab30:	00 
   18012ab31:	f3 0f 59 fc          	mulss  xmm7,xmm4
   18012ab35:	f3 0f 10 8d b4 00 00 	movss  xmm1,DWORD PTR [rbp+0xb4]
   18012ab3c:	00 
   18012ab3d:	f3 0f 59 cb          	mulss  xmm1,xmm3
   18012ab41:	f3 0f 10 85 c0 00 00 	movss  xmm0,DWORD PTR [rbp+0xc0]
   18012ab48:	00 
   18012ab49:	f3 0f 59 c2          	mulss  xmm0,xmm2
   18012ab4d:	f3 0f 58 f9          	addss  xmm7,xmm1
   18012ab51:	f3 0f 58 f8          	addss  xmm7,xmm0
   18012ab55:	f3 0f 58 bd cc 00 00 	addss  xmm7,DWORD PTR [rbp+0xcc]
   18012ab5c:	00 
   18012ab5d:	f3 45 0f 5c d3       	subss  xmm10,xmm11
   18012ab62:	f3 45 0f 5c c1       	subss  xmm8,xmm9
   18012ab67:	f3 0f 5c fe          	subss  xmm7,xmm6
   18012ab6b:	41 0f 28 d0          	movaps xmm2,xmm8
   18012ab6f:	f3 41 0f 59 d0       	mulss  xmm2,xmm8
   18012ab74:	41 0f 28 c2          	movaps xmm0,xmm10
   18012ab78:	f3 41 0f 59 c2       	mulss  xmm0,xmm10
   18012ab7d:	f3 0f 58 d0          	addss  xmm2,xmm0
   18012ab81:	0f 28 c7             	movaps xmm0,xmm7
   18012ab84:	f3 0f 59 c7          	mulss  xmm0,xmm7
   18012ab88:	f3 0f 58 c2          	addss  xmm0,xmm2
   18012ab8c:	0f 28 f0             	movaps xmm6,xmm0
   18012ab8f:	0f 57 ed             	xorps  xmm5,xmm5
   18012ab92:	f3 0f 52 de          	rsqrtss xmm3,xmm6
   18012ab96:	0f 28 c8             	movaps xmm1,xmm0
   18012ab99:	f3 0f 59 cb          	mulss  xmm1,xmm3
   18012ab9d:	f3 0f 59 cb          	mulss  xmm1,xmm3
   18012aba1:	0f 28 55 b0          	movaps xmm2,XMMWORD PTR [rbp-0x50]
   18012aba5:	f3 0f 5c d1          	subss  xmm2,xmm1
   18012aba9:	0f 28 a5 e0 01 00 00 	movaps xmm4,XMMWORD PTR [rbp+0x1e0]
   18012abb0:	f3 0f 59 e3          	mulss  xmm4,xmm3
   18012abb4:	f3 0f 59 e2          	mulss  xmm4,xmm2
   18012abb8:	f3 0f c2 f5 00       	cmpeqss xmm6,xmm5
   18012abbd:	0f 28 c6             	movaps xmm0,xmm6
   18012abc0:	66 0f 38 14 e5       	blendvps xmm4,xmm5,xmm0
   18012abc5:	0f 28 c4             	movaps xmm0,xmm4
   18012abc8:	f3 41 0f 59 c2       	mulss  xmm0,xmm10
   18012abcd:	f3 0f 11 85 10 02 00 	movss  DWORD PTR [rbp+0x210],xmm0
   18012abd4:	00 
   18012abd5:	0f 28 cc             	movaps xmm1,xmm4
   18012abd8:	f3 41 0f 59 c8       	mulss  xmm1,xmm8
   18012abdd:	f3 0f 11 8d 14 02 00 	movss  DWORD PTR [rbp+0x214],xmm1
   18012abe4:	00 
   18012abe5:	f3 0f 59 e7          	mulss  xmm4,xmm7
   18012abe9:	f3 0f 11 a5 18 02 00 	movss  DWORD PTR [rbp+0x218],xmm4
   18012abf0:	00 
   18012abf1:	48 c7 85 90 01 00 00 	mov    QWORD PTR [rbp+0x190],0x0
   18012abf8:	00 00 00 00 
   18012abfc:	c7 85 98 01 00 00 00 	mov    DWORD PTR [rbp+0x198],0x0
   18012ac03:	00 00 00 
   18012ac06:	48 c7 85 80 01 00 00 	mov    QWORD PTR [rbp+0x180],0x0
   18012ac0d:	00 00 00 00 
   18012ac11:	c7 85 88 01 00 00 00 	mov    DWORD PTR [rbp+0x188],0x0
   18012ac18:	00 00 00 
   18012ac1b:	4c 8d 85 80 01 00 00 	lea    r8,[rbp+0x180]
   18012ac22:	48 8d 95 90 01 00 00 	lea    rdx,[rbp+0x190]
   18012ac29:	48 8d 8d 10 02 00 00 	lea    rcx,[rbp+0x210]
   18012ac30:	e8 ab fa 00 00       	call   0x18013a6e0
   18012ac35:	48 8b 85 50 01 00 00 	mov    rax,QWORD PTR [rbp+0x150]
   18012ac3c:	f3 42 0f 10 4c 20 04 	movss  xmm1,DWORD PTR [rax+r12*1+0x4]
   18012ac43:	f3 42 0f 10 14 20    	movss  xmm2,DWORD PTR [rax+r12*1]
   18012ac49:	f3 44 0f 10 85 80 01 	movss  xmm8,DWORD PTR [rbp+0x180]
   18012ac50:	00 00 
   18012ac52:	f3 44 0f 59 c1       	mulss  xmm8,xmm1
   18012ac57:	f3 0f 10 85 90 01 00 	movss  xmm0,DWORD PTR [rbp+0x190]
   18012ac5e:	00 
   18012ac5f:	f3 0f 59 c2          	mulss  xmm0,xmm2
   18012ac63:	f3 44 0f 58 c0       	addss  xmm8,xmm0
   18012ac68:	f3 44 0f 10 8d 84 01 	movss  xmm9,DWORD PTR [rbp+0x184]
   18012ac6f:	00 00 
   18012ac71:	f3 44 0f 59 c9       	mulss  xmm9,xmm1
   18012ac76:	f3 0f 10 85 94 01 00 	movss  xmm0,DWORD PTR [rbp+0x194]
   18012ac7d:	00 
   18012ac7e:	f3 0f 59 c2          	mulss  xmm0,xmm2
   18012ac82:	f3 44 0f 58 c8       	addss  xmm9,xmm0
   18012ac87:	f3 44 0f 10 95 88 01 	movss  xmm10,DWORD PTR [rbp+0x188]
   18012ac8e:	00 00 
   18012ac90:	f3 44 0f 59 d1       	mulss  xmm10,xmm1
   18012ac95:	f3 0f 10 85 98 01 00 	movss  xmm0,DWORD PTR [rbp+0x198]
   18012ac9c:	00 
   18012ac9d:	f3 0f 59 c2          	mulss  xmm0,xmm2
   18012aca1:	f3 44 0f 58 d0       	addss  xmm10,xmm0
   18012aca6:	66 41 0f 6e ce       	movd   xmm1,r14d
   18012acab:	0f 5b c9             	cvtdq2ps xmm1,xmm1
   18012acae:	f3 0f 59 0d 86 40 55 	mulss  xmm1,DWORD PTR [rip+0x554086]        # 0x18067ed3c
   18012acb5:	00 
   18012acb6:	f3 44 0f 59 c1       	mulss  xmm8,xmm1
   18012acbb:	f3 44 0f 59 c9       	mulss  xmm9,xmm1
   18012acc0:	f3 44 0f 59 d1       	mulss  xmm10,xmm1
   18012acc5:	80 3d 64 c3 16 01 00 	cmp    BYTE PTR [rip+0x116c364],0x0        # 0x181297030
   18012accc:	0f 84 9b 00 00 00    	je     0x18012ad6d
   18012acd2:	f3 0f 10 4e f8       	movss  xmm1,DWORD PTR [rsi-0x8]
   18012acd7:	f3 41 0f 59 cc       	mulss  xmm1,xmm12
   18012acdc:	0f 28 d1             	movaps xmm2,xmm1
   18012acdf:	66 0f 3a 0a d2 0c    	roundss xmm2,xmm2,0xc
   18012ace5:	f3 0f 2d ca          	cvtss2si ecx,xmm2
   18012ace9:	85 c9                	test   ecx,ecx
   18012aceb:	79 06                	jns    0x18012acf3
   18012aced:	c6 45 a2 00          	mov    BYTE PTR [rbp-0x5e],0x0
   18012acf1:	eb 0d                	jmp    0x18012ad00
   18012acf3:	0f b6 c1             	movzx  eax,cl
   18012acf6:	41 3b cf             	cmp    ecx,r15d
   18012acf9:	41 0f 4f c7          	cmovg  eax,r15d
   18012acfd:	88 45 a2             	mov    BYTE PTR [rbp-0x5e],al
   18012ad00:	f3 0f 10 4e fc       	movss  xmm1,DWORD PTR [rsi-0x4]
   18012ad05:	f3 41 0f 59 cc       	mulss  xmm1,xmm12
   18012ad0a:	0f 28 d1             	movaps xmm2,xmm1
   18012ad0d:	66 0f 3a 0a d2 0c    	roundss xmm2,xmm2,0xc
   18012ad13:	f3 0f 2d ca          	cvtss2si ecx,xmm2
   18012ad17:	85 c9                	test   ecx,ecx
   18012ad19:	79 06                	jns    0x18012ad21
   18012ad1b:	c6 45 a1 00          	mov    BYTE PTR [rbp-0x5f],0x0
   18012ad1f:	eb 0d                	jmp    0x18012ad2e
   18012ad21:	0f b6 c1             	movzx  eax,cl
   18012ad24:	41 3b cf             	cmp    ecx,r15d
   18012ad27:	41 0f 4f c7          	cmovg  eax,r15d
   18012ad2b:	88 45 a1             	mov    BYTE PTR [rbp-0x5f],al
   18012ad2e:	f3 0f 10 0e          	movss  xmm1,DWORD PTR [rsi]
   18012ad32:	f3 41 0f 59 cc       	mulss  xmm1,xmm12
   18012ad37:	0f 28 d1             	movaps xmm2,xmm1
   18012ad3a:	66 0f 3a 0a d2 0c    	roundss xmm2,xmm2,0xc
   18012ad40:	f3 0f 2d ca          	cvtss2si ecx,xmm2
   18012ad44:	85 c9                	test   ecx,ecx
   18012ad46:	79 0e                	jns    0x18012ad56
   18012ad48:	c6 45 a0 00          	mov    BYTE PTR [rbp-0x60],0x0
   18012ad4c:	44 88 7d a3          	mov    BYTE PTR [rbp-0x5d],r15b
   18012ad50:	48 8d 7d a0          	lea    rdi,[rbp-0x60]
   18012ad54:	eb 2d                	jmp    0x18012ad83
   18012ad56:	0f b6 c1             	movzx  eax,cl
   18012ad59:	41 3b cf             	cmp    ecx,r15d
   18012ad5c:	41 0f 4f c7          	cmovg  eax,r15d
   18012ad60:	88 45 a0             	mov    BYTE PTR [rbp-0x60],al
   18012ad63:	44 88 7d a3          	mov    BYTE PTR [rbp-0x5d],r15b
   18012ad67:	48 8d 7d a0          	lea    rdi,[rbp-0x60]
   18012ad6b:	eb 16                	jmp    0x18012ad83
   18012ad6d:	48 8b 05 94 65 4b 00 	mov    rax,QWORD PTR [rip+0x4b6594]        # 0x1805e1308 ; ?Red@Color@g@@2V12@B
   18012ad74:	8b 08                	mov    ecx,DWORD PTR [rax]
   18012ad76:	89 8d 90 00 00 00    	mov    DWORD PTR [rbp+0x90],ecx
   18012ad7c:	48 8d bd 90 00 00 00 	lea    rdi,[rbp+0x90]
   18012ad83:	41 0f 28 c0          	movaps xmm0,xmm8
   18012ad87:	f3 41 0f 58 c7       	addss  xmm0,xmm15
   18012ad8c:	f3 0f 11 85 2c 02 00 	movss  DWORD PTR [rbp+0x22c],xmm0
   18012ad93:	00 
   18012ad94:	41 0f 28 c9          	movaps xmm1,xmm9
   18012ad98:	f3 41 0f 58 cd       	addss  xmm1,xmm13
   18012ad9d:	f3 0f 11 8d 30 02 00 	movss  DWORD PTR [rbp+0x230],xmm1
   18012ada4:	00 
   18012ada5:	41 0f 28 c2          	movaps xmm0,xmm10
   18012ada9:	f3 41 0f 58 c6       	addss  xmm0,xmm14
   18012adae:	f3 0f 11 85 34 02 00 	movss  DWORD PTR [rbp+0x234],xmm0
   18012adb5:	00 
   18012adb6:	48 8d 15 13 bb 16 01 	lea    rdx,[rip+0x116bb13]        # 0x1812968d0
   18012adbd:	48 8d 4d e0          	lea    rcx,[rbp-0x20]
   18012adc1:	e8 da db ee ff       	call   0x1800189a0
   18012adc6:	f2 0f 10 76 e0       	movsd  xmm6,QWORD PTR [rsi-0x20]
   18012adcb:	8b 46 e8             	mov    eax,DWORD PTR [rsi-0x18]
   18012adce:	89 85 28 02 00 00    	mov    DWORD PTR [rbp+0x228],eax
   18012add4:	f3 0f 10 7d e0       	movss  xmm7,DWORD PTR [rbp-0x20]
   18012add9:	f2 0f 11 b5 20 02 00 	movsd  QWORD PTR [rbp+0x220],xmm6
   18012ade0:	00 
   18012ade1:	f3 0f 59 fe          	mulss  xmm7,xmm6
   18012ade5:	f3 0f 10 4d ec       	movss  xmm1,DWORD PTR [rbp-0x14]
   18012adea:	f3 0f 10 a5 24 02 00 	movss  xmm4,DWORD PTR [rbp+0x224]
   18012adf1:	00 
   18012adf2:	f3 0f 59 cc          	mulss  xmm1,xmm4
   18012adf6:	f3 0f 10 45 f8       	movss  xmm0,DWORD PTR [rbp-0x8]
   18012adfb:	f3 0f 10 9d 28 02 00 	movss  xmm3,DWORD PTR [rbp+0x228]
   18012ae02:	00 
   18012ae03:	f3 0f 59 c3          	mulss  xmm0,xmm3
   18012ae07:	f3 0f 58 f9          	addss  xmm7,xmm1
   18012ae0b:	f3 0f 58 f8          	addss  xmm7,xmm0
   18012ae0f:	f3 0f 58 7d 04       	addss  xmm7,DWORD PTR [rbp+0x4]
   18012ae14:	f3 0f 10 6d e4       	movss  xmm5,DWORD PTR [rbp-0x1c]
   18012ae19:	f3 0f 59 ee          	mulss  xmm5,xmm6
   18012ae1d:	f3 0f 10 4d f0       	movss  xmm1,DWORD PTR [rbp-0x10]
   18012ae22:	f3 0f 59 cc          	mulss  xmm1,xmm4
   18012ae26:	f3 0f 10 45 fc       	movss  xmm0,DWORD PTR [rbp-0x4]
   18012ae2b:	f3 0f 59 c3          	mulss  xmm0,xmm3
   18012ae2f:	f3 0f 58 e9          	addss  xmm5,xmm1
   18012ae33:	f3 0f 58 e8          	addss  xmm5,xmm0
   18012ae37:	f3 0f 58 6d 08       	addss  xmm5,DWORD PTR [rbp+0x8]
   18012ae3c:	f3 0f 10 55 e8       	movss  xmm2,DWORD PTR [rbp-0x18]
   18012ae41:	f3 0f 59 d6          	mulss  xmm2,xmm6
   18012ae45:	f3 0f 10 4d f4       	movss  xmm1,DWORD PTR [rbp-0xc]
   18012ae4a:	f3 0f 59 cc          	mulss  xmm1,xmm4
   18012ae4e:	f3 0f 10 45 00       	movss  xmm0,DWORD PTR [rbp+0x0]
   18012ae53:	f3 0f 59 c3          	mulss  xmm0,xmm3
   18012ae57:	f3 0f 58 d1          	addss  xmm2,xmm1
   18012ae5b:	f3 0f 58 d0          	addss  xmm2,xmm0
   18012ae5f:	f3 0f 58 55 0c       	addss  xmm2,DWORD PTR [rbp+0xc]
   18012ae64:	f3 44 0f 58 c7       	addss  xmm8,xmm7
   18012ae69:	f3 44 0f 11 85 38 02 	movss  DWORD PTR [rbp+0x238],xmm8
   18012ae70:	00 00 
   18012ae72:	f3 44 0f 58 cd       	addss  xmm9,xmm5
   18012ae77:	f3 44 0f 11 8d 3c 02 	movss  DWORD PTR [rbp+0x23c],xmm9
   18012ae7e:	00 00 
   18012ae80:	f3 44 0f 58 d2       	addss  xmm10,xmm2
   18012ae85:	f3 44 0f 11 95 40 02 	movss  DWORD PTR [rbp+0x240],xmm10
   18012ae8c:	00 00 
   18012ae8e:	c7 44 24 20 01 00 00 	mov    DWORD PTR [rsp+0x20],0x1
   18012ae95:	00 
   18012ae96:	45 33 c9             	xor    r9d,r9d
   18012ae99:	4c 8b c7             	mov    r8,rdi
   18012ae9c:	48 8d 95 2c 02 00 00 	lea    rdx,[rbp+0x22c]
   18012aea3:	48 8d 8d 38 02 00 00 	lea    rcx,[rbp+0x238]
   18012aeaa:	ff 15 68 58 4b 00    	call   QWORD PTR [rip+0x4b5868]        # 0x1805e0718 ; ?drawLine@d@@YAXAEBV?$Vector3Template@M@m@@0AEBVColor@g@@W4BlendMode@1@W4DepthMode@1@@Z
   18012aeb0:	41 ff c6             	inc    r14d
   18012aeb3:	49 83 c4 08          	add    r12,0x8
   18012aeb7:	41 83 fe 10          	cmp    r14d,0x10
   18012aebb:	0f 82 05 fb ff ff    	jb     0x18012a9c6
   18012aec1:	41 81 fd ff ff 00 00 	cmp    r13d,0xffff
   18012aec8:	0f 85 86 07 00 00    	jne    0x18012b654
   18012aece:	48 8d 0d eb bf 16 01 	lea    rcx,[rip+0x116bfeb]        # 0x181296ec0
   18012aed5:	e8 46 8e ee ff       	call   0x180013d20
   18012aeda:	84 c0                	test   al,al
   18012aedc:	0f 84 72 07 00 00    	je     0x18012b654
   18012aee2:	0f 28 05 77 46 55 00 	movaps xmm0,XMMWORD PTR [rip+0x554677]        # 0x18067f560
   18012aee9:	0f 29 85 a0 00 00 00 	movaps XMMWORD PTR [rbp+0xa0],xmm0
   18012aef0:	0f 28 c8             	movaps xmm1,xmm0
   18012aef3:	0f 29 85 b0 00 00 00 	movaps XMMWORD PTR [rbp+0xb0],xmm0
   18012aefa:	0f 29 85 c0 00 00 00 	movaps XMMWORD PTR [rbp+0xc0],xmm0
   18012af01:	48 8d 15 c8 b9 16 01 	lea    rdx,[rip+0x116b9c8]        # 0x1812968d0
   18012af08:	48 8d 4d e0          	lea    rcx,[rbp-0x20]
   18012af0c:	e8 8f da ee ff       	call   0x1800189a0
   18012af11:	f2 0f 10 76 e0       	movsd  xmm6,QWORD PTR [rsi-0x20]
   18012af16:	8b 46 e8             	mov    eax,DWORD PTR [rsi-0x18]
   18012af19:	89 85 30 01 00 00    	mov    DWORD PTR [rbp+0x130],eax
   18012af1f:	f2 0f 11 b5 28 01 00 	movsd  QWORD PTR [rbp+0x128],xmm6
   18012af26:	00 
   18012af27:	0f 28 ee             	movaps xmm5,xmm6
   18012af2a:	f3 0f 59 6d e0       	mulss  xmm5,DWORD PTR [rbp-0x20]
   18012af2f:	f3 0f 10 a5 2c 01 00 	movss  xmm4,DWORD PTR [rbp+0x12c]
   18012af36:	00 
   18012af37:	0f 28 cc             	movaps xmm1,xmm4
   18012af3a:	f3 0f 59 4d ec       	mulss  xmm1,DWORD PTR [rbp-0x14]
   18012af3f:	f3 0f 10 9d 30 01 00 	movss  xmm3,DWORD PTR [rbp+0x130]
   18012af46:	00 
   18012af47:	0f 28 c3             	movaps xmm0,xmm3
   18012af4a:	f3 0f 59 45 f8       	mulss  xmm0,DWORD PTR [rbp-0x8]
   18012af4f:	f3 0f 58 e9          	addss  xmm5,xmm1
   18012af53:	f3 0f 58 e8          	addss  xmm5,xmm0
   18012af57:	f3 0f 58 6d 04       	addss  xmm5,DWORD PTR [rbp+0x4]
   18012af5c:	0f 28 d6             	movaps xmm2,xmm6
   18012af5f:	f3 0f 59 55 e4       	mulss  xmm2,DWORD PTR [rbp-0x1c]
   18012af64:	0f 28 cc             	movaps xmm1,xmm4
   18012af67:	f3 0f 59 4d f0       	mulss  xmm1,DWORD PTR [rbp-0x10]
   18012af6c:	0f 28 c3             	movaps xmm0,xmm3
   18012af6f:	f3 0f 59 45 fc       	mulss  xmm0,DWORD PTR [rbp-0x4]
   18012af74:	f3 0f 58 d1          	addss  xmm2,xmm1
   18012af78:	f3 0f 58 d0          	addss  xmm2,xmm0
   18012af7c:	f3 0f 58 55 08       	addss  xmm2,DWORD PTR [rbp+0x8]
   18012af81:	f3 0f 59 75 e8       	mulss  xmm6,DWORD PTR [rbp-0x18]
   18012af86:	f3 0f 59 65 f4       	mulss  xmm4,DWORD PTR [rbp-0xc]
   18012af8b:	f3 0f 59 5d 00       	mulss  xmm3,DWORD PTR [rbp+0x0]
   18012af90:	f3 0f 58 f4          	addss  xmm6,xmm4
   18012af94:	f3 0f 58 f3          	addss  xmm6,xmm3
   18012af98:	f3 0f 58 75 0c       	addss  xmm6,DWORD PTR [rbp+0xc]
   18012af9d:	f3 0f 11 ad 28 01 00 	movss  DWORD PTR [rbp+0x128],xmm5
   18012afa4:	00 
   18012afa5:	f3 0f 11 95 2c 01 00 	movss  DWORD PTR [rbp+0x12c],xmm2
   18012afac:	00 
   18012afad:	f3 0f 11 b5 30 01 00 	movss  DWORD PTR [rbp+0x130],xmm6
   18012afb4:	00 
   18012afb5:	c7 44 24 38 08 00 00 	mov    DWORD PTR [rsp+0x38],0x8
   18012afbc:	00 
   18012afbd:	c7 44 24 30 0c 00 00 	mov    DWORD PTR [rsp+0x30],0xc
   18012afc4:	00 
   18012afc5:	c7 44 24 28 01 00 00 	mov    DWORD PTR [rsp+0x28],0x1
   18012afcc:	00 
   18012afcd:	c7 44 24 20 00 00 00 	mov    DWORD PTR [rsp+0x20],0x0
   18012afd4:	00 
   18012afd5:	4c 8b 0d 24 63 4b 00 	mov    r9,QWORD PTR [rip+0x4b6324]        # 0x1805e1300 ; ?White@Color@g@@2V12@B
   18012afdc:	4c 8d 85 a0 00 00 00 	lea    r8,[rbp+0xa0]
   18012afe3:	f3 44 0f 10 1d 88 3d 	movss  xmm11,DWORD PTR [rip+0x553d88]        # 0x18067ed74
   18012afea:	55 00 
   18012afec:	41 0f 28 cb          	movaps xmm1,xmm11
   18012aff0:	48 8d 8d 28 01 00 00 	lea    rcx,[rbp+0x128]
   18012aff7:	ff 15 5b 58 4b 00    	call   QWORD PTR [rip+0x4b585b]        # 0x1805e0858 ; ?fillSphere@d@@YAXAEBV?$Vector3Template@M@m@@MAEBV?$Matrix4x3Template@M@3@AEBVColor@g@@W4BlendMode@1@W4DepthMode@1@HH@Z
   18012affd:	e9 5b 06 00 00       	jmp    0x18012b65d
   18012b002:	80 3d a7 be 16 01 00 	cmp    BYTE PTR [rip+0x116bea7],0x0        # 0x181296eb0
   18012b009:	0f 84 24 01 00 00    	je     0x18012b133
   18012b00f:	0f 28 05 4a 45 55 00 	movaps xmm0,XMMWORD PTR [rip+0x55454a]        # 0x18067f560
   18012b016:	0f 29 85 a0 00 00 00 	movaps XMMWORD PTR [rbp+0xa0],xmm0
   18012b01d:	0f 28 c8             	movaps xmm1,xmm0
   18012b020:	0f 29 85 b0 00 00 00 	movaps XMMWORD PTR [rbp+0xb0],xmm0
   18012b027:	0f 29 85 c0 00 00 00 	movaps XMMWORD PTR [rbp+0xc0],xmm0
   18012b02e:	48 8d 15 9b b8 16 01 	lea    rdx,[rip+0x116b89b]        # 0x1812968d0
   18012b035:	48 8d 4d e0          	lea    rcx,[rbp-0x20]
   18012b039:	e8 62 d9 ee ff       	call   0x1800189a0
   18012b03e:	f2 0f 10 76 e0       	movsd  xmm6,QWORD PTR [rsi-0x20]
   18012b043:	8b 46 e8             	mov    eax,DWORD PTR [rsi-0x18]
   18012b046:	89 85 24 01 00 00    	mov    DWORD PTR [rbp+0x124],eax
   18012b04c:	f3 0f 10 7d e0       	movss  xmm7,DWORD PTR [rbp-0x20]
   18012b051:	f2 0f 11 b5 1c 01 00 	movsd  QWORD PTR [rbp+0x11c],xmm6
   18012b058:	00 
   18012b059:	f3 0f 59 fe          	mulss  xmm7,xmm6
   18012b05d:	f3 0f 10 4d ec       	movss  xmm1,DWORD PTR [rbp-0x14]
   18012b062:	f3 0f 10 ad 20 01 00 	movss  xmm5,DWORD PTR [rbp+0x120]
   18012b069:	00 
   18012b06a:	f3 0f 59 cd          	mulss  xmm1,xmm5
   18012b06e:	f3 0f 10 45 f8       	movss  xmm0,DWORD PTR [rbp-0x8]
   18012b073:	f3 0f 10 9d 24 01 00 	movss  xmm3,DWORD PTR [rbp+0x124]
   18012b07a:	00 
   18012b07b:	f3 0f 59 c3          	mulss  xmm0,xmm3
   18012b07f:	f3 0f 58 f9          	addss  xmm7,xmm1
   18012b083:	f3 0f 58 f8          	addss  xmm7,xmm0
   18012b087:	f3 0f 58 7d 04       	addss  xmm7,DWORD PTR [rbp+0x4]
   18012b08c:	f3 0f 10 65 e4       	movss  xmm4,DWORD PTR [rbp-0x1c]
   18012b091:	f3 0f 59 e6          	mulss  xmm4,xmm6
   18012b095:	f3 0f 10 4d f0       	movss  xmm1,DWORD PTR [rbp-0x10]
   18012b09a:	f3 0f 59 cd          	mulss  xmm1,xmm5
   18012b09e:	f3 0f 10 45 fc       	movss  xmm0,DWORD PTR [rbp-0x4]
   18012b0a3:	f3 0f 59 c3          	mulss  xmm0,xmm3
   18012b0a7:	f3 0f 58 e1          	addss  xmm4,xmm1
   18012b0ab:	f3 0f 58 e0          	addss  xmm4,xmm0
   18012b0af:	f3 0f 58 65 08       	addss  xmm4,DWORD PTR [rbp+0x8]
   18012b0b4:	f3 0f 10 55 e8       	movss  xmm2,DWORD PTR [rbp-0x18]
   18012b0b9:	f3 0f 59 d6          	mulss  xmm2,xmm6
   18012b0bd:	f3 0f 10 4d f4       	movss  xmm1,DWORD PTR [rbp-0xc]
   18012b0c2:	f3 0f 59 cd          	mulss  xmm1,xmm5
   18012b0c6:	f3 0f 10 45 00       	movss  xmm0,DWORD PTR [rbp+0x0]
   18012b0cb:	f3 0f 59 c3          	mulss  xmm0,xmm3
   18012b0cf:	f3 0f 58 d1          	addss  xmm2,xmm1
   18012b0d3:	f3 0f 58 d0          	addss  xmm2,xmm0
   18012b0d7:	f3 0f 58 55 0c       	addss  xmm2,DWORD PTR [rbp+0xc]
   18012b0dc:	f3 0f 11 bd 1c 01 00 	movss  DWORD PTR [rbp+0x11c],xmm7
   18012b0e3:	00 
   18012b0e4:	f3 0f 11 a5 20 01 00 	movss  DWORD PTR [rbp+0x120],xmm4
   18012b0eb:	00 
   18012b0ec:	f3 0f 11 95 24 01 00 	movss  DWORD PTR [rbp+0x124],xmm2
   18012b0f3:	00 
   18012b0f4:	c7 44 24 38 08 00 00 	mov    DWORD PTR [rsp+0x38],0x8
   18012b0fb:	00 
   18012b0fc:	c7 44 24 30 0c 00 00 	mov    DWORD PTR [rsp+0x30],0xc
   18012b103:	00 
   18012b104:	c7 44 24 28 01 00 00 	mov    DWORD PTR [rsp+0x28],0x1
   18012b10b:	00 
   18012b10c:	c7 44 24 20 00 00 00 	mov    DWORD PTR [rsp+0x20],0x0
   18012b113:	00 
   18012b114:	4c 8b 0d e5 61 4b 00 	mov    r9,QWORD PTR [rip+0x4b61e5]        # 0x1805e1300 ; ?White@Color@g@@2V12@B
   18012b11b:	4c 8d 85 a0 00 00 00 	lea    r8,[rbp+0xa0]
   18012b122:	41 0f 28 cb          	movaps xmm1,xmm11
   18012b126:	48 8d 8d 1c 01 00 00 	lea    rcx,[rbp+0x11c]
   18012b12d:	ff 15 25 57 4b 00    	call   QWORD PTR [rip+0x4b5725]        # 0x1805e0858 ; ?fillSphere@d@@YAXAEBV?$Vector3Template@M@m@@MAEBV?$Matrix4x3Template@M@3@AEBVColor@g@@W4BlendMode@1@W4DepthMode@1@HH@Z
   18012b133:	80 3d 36 be 16 01 00 	cmp    BYTE PTR [rip+0x116be36],0x0        # 0x181296f70
   18012b13a:	0f 84 81 fd ff ff    	je     0x18012aec1
   18012b140:	45 33 f6             	xor    r14d,r14d
   18012b143:	f3 0f 10 05 6d 3d 55 	movss  xmm0,DWORD PTR [rip+0x553d6d]        # 0x18067eeb8
   18012b14a:	00 
   18012b14b:	0f 29 85 e0 01 00 00 	movaps XMMWORD PTR [rbp+0x1e0],xmm0
   18012b152:	41 0f 28 c2          	movaps xmm0,xmm10
   18012b156:	0f 29 45 b0          	movaps XMMWORD PTR [rbp-0x50],xmm0
   18012b15a:	48 8d 15 6f b7 16 01 	lea    rdx,[rip+0x116b76f]        # 0x1812968d0
   18012b161:	48 8d 4d e0          	lea    rcx,[rbp-0x20]
   18012b165:	e8 36 d8 ee ff       	call   0x1800189a0
   18012b16a:	f2 0f 10 66 ec       	movsd  xmm4,QWORD PTR [rsi-0x14]
   18012b16f:	8b 46 f4             	mov    eax,DWORD PTR [rsi-0xc]
   18012b172:	89 85 4c 02 00 00    	mov    DWORD PTR [rbp+0x24c],eax
   18012b178:	f3 44 0f 10 5d e0    	movss  xmm11,DWORD PTR [rbp-0x20]
   18012b17e:	f2 0f 11 a5 44 02 00 	movsd  QWORD PTR [rbp+0x244],xmm4
   18012b185:	00 
   18012b186:	f3 44 0f 59 dc       	mulss  xmm11,xmm4
   18012b18b:	f3 0f 10 4d ec       	movss  xmm1,DWORD PTR [rbp-0x14]
   18012b190:	f3 0f 10 9d 48 02 00 	movss  xmm3,DWORD PTR [rbp+0x248]
   18012b197:	00 
   18012b198:	f3 0f 59 cb          	mulss  xmm1,xmm3
   18012b19c:	f3 0f 10 45 f8       	movss  xmm0,DWORD PTR [rbp-0x8]
   18012b1a1:	f3 0f 10 95 4c 02 00 	movss  xmm2,DWORD PTR [rbp+0x24c]
   18012b1a8:	00 
   18012b1a9:	f3 0f 59 c2          	mulss  xmm0,xmm2
   18012b1ad:	f3 44 0f 58 d9       	addss  xmm11,xmm1
   18012b1b2:	f3 44 0f 58 d8       	addss  xmm11,xmm0
   18012b1b7:	f3 44 0f 58 5d 04    	addss  xmm11,DWORD PTR [rbp+0x4]
   18012b1bd:	f3 44 0f 10 4d e4    	movss  xmm9,DWORD PTR [rbp-0x1c]
   18012b1c3:	f3 44 0f 59 cc       	mulss  xmm9,xmm4
   18012b1c8:	f3 0f 10 4d f0       	movss  xmm1,DWORD PTR [rbp-0x10]
   18012b1cd:	f3 0f 59 cb          	mulss  xmm1,xmm3
   18012b1d1:	f3 0f 10 45 fc       	movss  xmm0,DWORD PTR [rbp-0x4]
   18012b1d6:	f3 0f 59 c2          	mulss  xmm0,xmm2
   18012b1da:	f3 44 0f 58 c9       	addss  xmm9,xmm1
   18012b1df:	f3 44 0f 58 c8       	addss  xmm9,xmm0
   18012b1e4:	f3 44 0f 58 4d 08    	addss  xmm9,DWORD PTR [rbp+0x8]
   18012b1ea:	f3 0f 10 75 e8       	movss  xmm6,DWORD PTR [rbp-0x18]
   18012b1ef:	f3 0f 59 f4          	mulss  xmm6,xmm4
   18012b1f3:	f3 0f 10 4d f4       	movss  xmm1,DWORD PTR [rbp-0xc]
   18012b1f8:	f3 0f 59 cb          	mulss  xmm1,xmm3
   18012b1fc:	f3 0f 10 45 00       	movss  xmm0,DWORD PTR [rbp+0x0]
   18012b201:	f3 0f 59 c2          	mulss  xmm0,xmm2
   18012b205:	f3 0f 58 f1          	addss  xmm6,xmm1
   18012b209:	f3 0f 58 f0          	addss  xmm6,xmm0
   18012b20d:	f3 0f 58 75 0c       	addss  xmm6,DWORD PTR [rbp+0xc]
   18012b212:	48 8d 15 b7 b6 16 01 	lea    rdx,[rip+0x116b6b7]        # 0x1812968d0
   18012b219:	48 8d 8d a0 00 00 00 	lea    rcx,[rbp+0xa0]
   18012b220:	e8 7b d7 ee ff       	call   0x1800189a0
   18012b225:	f2 0f 10 66 e0       	movsd  xmm4,QWORD PTR [rsi-0x20]
   18012b22a:	8b 46 e8             	mov    eax,DWORD PTR [rsi-0x18]
   18012b22d:	89 85 e4 02 00 00    	mov    DWORD PTR [rbp+0x2e4],eax
   18012b233:	f3 44 0f 10 95 a0 00 	movss  xmm10,DWORD PTR [rbp+0xa0]
   18012b23a:	00 00 
   18012b23c:	f2 0f 11 a5 dc 02 00 	movsd  QWORD PTR [rbp+0x2dc],xmm4
   18012b243:	00 
   18012b244:	f3 44 0f 59 d4       	mulss  xmm10,xmm4
   18012b249:	f3 0f 10 8d ac 00 00 	movss  xmm1,DWORD PTR [rbp+0xac]
   18012b250:	00 
   18012b251:	f3 0f 10 9d e0 02 00 	movss  xmm3,DWORD PTR [rbp+0x2e0]
   18012b258:	00 
   18012b259:	f3 0f 59 cb          	mulss  xmm1,xmm3
   18012b25d:	f3 0f 10 85 b8 00 00 	movss  xmm0,DWORD PTR [rbp+0xb8]
   18012b264:	00 
   18012b265:	f3 0f 10 95 e4 02 00 	movss  xmm2,DWORD PTR [rbp+0x2e4]
   18012b26c:	00 
   18012b26d:	f3 0f 59 c2          	mulss  xmm0,xmm2
   18012b271:	f3 44 0f 58 d1       	addss  xmm10,xmm1
   18012b276:	f3 44 0f 58 d0       	addss  xmm10,xmm0
   18012b27b:	f3 44 0f 58 95 c4 00 	addss  xmm10,DWORD PTR [rbp+0xc4]
   18012b282:	00 00 
   18012b284:	f3 44 0f 10 85 a4 00 	movss  xmm8,DWORD PTR [rbp+0xa4]
   18012b28b:	00 00 
   18012b28d:	f3 44 0f 59 c4       	mulss  xmm8,xmm4
   18012b292:	f3 0f 10 8d b0 00 00 	movss  xmm1,DWORD PTR [rbp+0xb0]
   18012b299:	00 
   18012b29a:	f3 0f 59 cb          	mulss  xmm1,xmm3
   18012b29e:	f3 0f 10 85 bc 00 00 	movss  xmm0,DWORD PTR [rbp+0xbc]
   18012b2a5:	00 
   18012b2a6:	f3 0f 59 c2          	mulss  xmm0,xmm2
   18012b2aa:	f3 44 0f 58 c1       	addss  xmm8,xmm1
   18012b2af:	f3 44 0f 58 c0       	addss  xmm8,xmm0
   18012b2b4:	f3 44 0f 58 85 c8 00 	addss  xmm8,DWORD PTR [rbp+0xc8]
   18012b2bb:	00 00 
   18012b2bd:	f3 0f 10 bd a8 00 00 	movss  xmm7,DWORD PTR [rbp+0xa8]
   18012b2c4:	00 
   18012b2c5:	f3 0f 59 fc          	mulss  xmm7,xmm4
   18012b2c9:	f3 0f 10 8d b4 00 00 	movss  xmm1,DWORD PTR [rbp+0xb4]
   18012b2d0:	00 
   18012b2d1:	f3 0f 59 cb          	mulss  xmm1,xmm3
   18012b2d5:	f3 0f 10 85 c0 00 00 	movss  xmm0,DWORD PTR [rbp+0xc0]
   18012b2dc:	00 
   18012b2dd:	f3 0f 59 c2          	mulss  xmm0,xmm2
   18012b2e1:	f3 0f 58 f9          	addss  xmm7,xmm1
   18012b2e5:	f3 0f 58 f8          	addss  xmm7,xmm0
   18012b2e9:	f3 0f 58 bd cc 00 00 	addss  xmm7,DWORD PTR [rbp+0xcc]
   18012b2f0:	00 
   18012b2f1:	f3 45 0f 5c d3       	subss  xmm10,xmm11
   18012b2f6:	f3 45 0f 5c c1       	subss  xmm8,xmm9
   18012b2fb:	f3 0f 5c fe          	subss  xmm7,xmm6
   18012b2ff:	41 0f 28 d0          	movaps xmm2,xmm8
   18012b303:	f3 41 0f 59 d0       	mulss  xmm2,xmm8
   18012b308:	41 0f 28 c2          	movaps xmm0,xmm10
   18012b30c:	f3 41 0f 59 c2       	mulss  xmm0,xmm10
   18012b311:	f3 0f 58 d0          	addss  xmm2,xmm0
   18012b315:	0f 28 c7             	movaps xmm0,xmm7
   18012b318:	f3 0f 59 c7          	mulss  xmm0,xmm7
   18012b31c:	f3 0f 58 c2          	addss  xmm0,xmm2
   18012b320:	0f 28 f0             	movaps xmm6,xmm0
   18012b323:	0f 57 ed             	xorps  xmm5,xmm5
   18012b326:	f3 0f 52 de          	rsqrtss xmm3,xmm6
   18012b32a:	0f 28 c8             	movaps xmm1,xmm0
   18012b32d:	f3 0f 59 cb          	mulss  xmm1,xmm3
   18012b331:	f3 0f 59 cb          	mulss  xmm1,xmm3
   18012b335:	0f 28 55 b0          	movaps xmm2,XMMWORD PTR [rbp-0x50]
   18012b339:	f3 0f 5c d1          	subss  xmm2,xmm1
   18012b33d:	0f 28 a5 e0 01 00 00 	movaps xmm4,XMMWORD PTR [rbp+0x1e0]
   18012b344:	f3 0f 59 e3          	mulss  xmm4,xmm3
   18012b348:	f3 0f 59 e2          	mulss  xmm4,xmm2
   18012b34c:	f3 0f c2 f5 00       	cmpeqss xmm6,xmm5
   18012b351:	0f 28 c6             	movaps xmm0,xmm6
   18012b354:	66 0f 38 14 e5       	blendvps xmm4,xmm5,xmm0
   18012b359:	0f 28 c4             	movaps xmm0,xmm4
   18012b35c:	f3 41 0f 59 c2       	mulss  xmm0,xmm10
   18012b361:	f3 0f 11 85 60 02 00 	movss  DWORD PTR [rbp+0x260],xmm0
   18012b368:	00 
   18012b369:	0f 28 cc             	movaps xmm1,xmm4
   18012b36c:	f3 41 0f 59 c8       	mulss  xmm1,xmm8
   18012b371:	f3 0f 11 8d 64 02 00 	movss  DWORD PTR [rbp+0x264],xmm1
   18012b378:	00 
   18012b379:	f3 0f 59 e7          	mulss  xmm4,xmm7
   18012b37d:	f3 0f 11 a5 68 02 00 	movss  DWORD PTR [rbp+0x268],xmm4
   18012b384:	00 
   18012b385:	48 c7 85 b0 01 00 00 	mov    QWORD PTR [rbp+0x1b0],0x0
   18012b38c:	00 00 00 00 
   18012b390:	c7 85 b8 01 00 00 00 	mov    DWORD PTR [rbp+0x1b8],0x0
   18012b397:	00 00 00 
   18012b39a:	48 c7 85 a0 01 00 00 	mov    QWORD PTR [rbp+0x1a0],0x0
   18012b3a1:	00 00 00 00 
   18012b3a5:	c7 85 a8 01 00 00 00 	mov    DWORD PTR [rbp+0x1a8],0x0
   18012b3ac:	00 00 00 
   18012b3af:	4c 8d 85 a0 01 00 00 	lea    r8,[rbp+0x1a0]
   18012b3b6:	48 8d 95 b0 01 00 00 	lea    rdx,[rbp+0x1b0]
   18012b3bd:	48 8d 8d 60 02 00 00 	lea    rcx,[rbp+0x260]
   18012b3c4:	e8 17 f3 00 00       	call   0x18013a6e0
   18012b3c9:	49 63 fe             	movsxd rdi,r14d
   18012b3cc:	48 8b d7             	mov    rdx,rdi
   18012b3cf:	48 8d 8d 50 01 00 00 	lea    rcx,[rbp+0x150]
   18012b3d6:	e8 a5 18 0e 00       	call   0x18020cc80
   18012b3db:	f3 44 0f 10 40 04    	movss  xmm8,DWORD PTR [rax+0x4]
   18012b3e1:	41 0f 28 f0          	movaps xmm6,xmm8
   18012b3e5:	f3 0f 59 b5 a0 01 00 	mulss  xmm6,DWORD PTR [rbp+0x1a0]
   18012b3ec:	00 
   18012b3ed:	41 0f 28 f8          	movaps xmm7,xmm8
   18012b3f1:	f3 0f 59 bd a4 01 00 	mulss  xmm7,DWORD PTR [rbp+0x1a4]
   18012b3f8:	00 
   18012b3f9:	f3 44 0f 59 85 a8 01 	mulss  xmm8,DWORD PTR [rbp+0x1a8]
   18012b400:	00 00 
   18012b402:	48 8b d7             	mov    rdx,rdi
   18012b405:	48 8d 8d 50 01 00 00 	lea    rcx,[rbp+0x150]
   18012b40c:	e8 6f 18 0e 00       	call   0x18020cc80
   18012b411:	f3 44 0f 10 08       	movss  xmm9,DWORD PTR [rax]
   18012b416:	45 0f 28 d1          	movaps xmm10,xmm9
   18012b41a:	f3 44 0f 59 95 b0 01 	mulss  xmm10,DWORD PTR [rbp+0x1b0]
   18012b421:	00 00 
   18012b423:	f3 44 0f 58 d6       	addss  xmm10,xmm6
   18012b428:	45 0f 28 d9          	movaps xmm11,xmm9
   18012b42c:	f3 44 0f 59 9d b4 01 	mulss  xmm11,DWORD PTR [rbp+0x1b4]
   18012b433:	00 00 
   18012b435:	f3 44 0f 58 df       	addss  xmm11,xmm7
   18012b43a:	f3 44 0f 59 8d b8 01 	mulss  xmm9,DWORD PTR [rbp+0x1b8]
   18012b441:	00 00 
   18012b443:	f3 45 0f 58 c8       	addss  xmm9,xmm8
   18012b448:	66 41 0f 6e c6       	movd   xmm0,r14d
   18012b44d:	0f 5b c0             	cvtdq2ps xmm0,xmm0
   18012b450:	f3 0f 59 05 e4 38 55 	mulss  xmm0,DWORD PTR [rip+0x5538e4]        # 0x18067ed3c
   18012b457:	00 
   18012b458:	f3 44 0f 59 d0       	mulss  xmm10,xmm0
   18012b45d:	f3 44 0f 59 d8       	mulss  xmm11,xmm0
   18012b462:	f3 44 0f 59 c8       	mulss  xmm9,xmm0
   18012b467:	48 8d 0d 12 bb 16 01 	lea    rcx,[rip+0x116bb12]        # 0x181296f80
   18012b46e:	e8 ad 88 ee ff       	call   0x180013d20
   18012b473:	84 c0                	test   al,al
   18012b475:	0f 84 9b 00 00 00    	je     0x18012b516
   18012b47b:	f3 0f 10 4e f8       	movss  xmm1,DWORD PTR [rsi-0x8]
   18012b480:	f3 41 0f 59 cc       	mulss  xmm1,xmm12
   18012b485:	0f 28 d1             	movaps xmm2,xmm1
   18012b488:	66 0f 3a 0a d2 0c    	roundss xmm2,xmm2,0xc
   18012b48e:	f3 0f 2d ca          	cvtss2si ecx,xmm2
   18012b492:	85 c9                	test   ecx,ecx
   18012b494:	79 06                	jns    0x18012b49c
   18012b496:	c6 45 a6 00          	mov    BYTE PTR [rbp-0x5a],0x0
   18012b49a:	eb 0d                	jmp    0x18012b4a9
   18012b49c:	0f b6 c1             	movzx  eax,cl
   18012b49f:	41 3b cf             	cmp    ecx,r15d
   18012b4a2:	41 0f 4f c7          	cmovg  eax,r15d
   18012b4a6:	88 45 a6             	mov    BYTE PTR [rbp-0x5a],al
   18012b4a9:	f3 0f 10 4e fc       	movss  xmm1,DWORD PTR [rsi-0x4]
   18012b4ae:	f3 41 0f 59 cc       	mulss  xmm1,xmm12
   18012b4b3:	0f 28 d1             	movaps xmm2,xmm1
   18012b4b6:	66 0f 3a 0a d2 0c    	roundss xmm2,xmm2,0xc
   18012b4bc:	f3 0f 2d ca          	cvtss2si ecx,xmm2
   18012b4c0:	85 c9                	test   ecx,ecx
   18012b4c2:	79 06                	jns    0x18012b4ca
   18012b4c4:	c6 45 a5 00          	mov    BYTE PTR [rbp-0x5b],0x0
   18012b4c8:	eb 0d                	jmp    0x18012b4d7
   18012b4ca:	0f b6 c1             	movzx  eax,cl
   18012b4cd:	41 3b cf             	cmp    ecx,r15d
   18012b4d0:	41 0f 4f c7          	cmovg  eax,r15d
   18012b4d4:	88 45 a5             	mov    BYTE PTR [rbp-0x5b],al
   18012b4d7:	f3 0f 10 0e          	movss  xmm1,DWORD PTR [rsi]
   18012b4db:	f3 41 0f 59 cc       	mulss  xmm1,xmm12
   18012b4e0:	0f 28 d1             	movaps xmm2,xmm1
   18012b4e3:	66 0f 3a 0a d2 0c    	roundss xmm2,xmm2,0xc
   18012b4e9:	f3 0f 2d ca          	cvtss2si ecx,xmm2
   18012b4ed:	85 c9                	test   ecx,ecx
   18012b4ef:	79 0e                	jns    0x18012b4ff
   18012b4f1:	c6 45 a4 00          	mov    BYTE PTR [rbp-0x5c],0x0
   18012b4f5:	44 88 7d a7          	mov    BYTE PTR [rbp-0x59],r15b
   18012b4f9:	48 8d 7d a4          	lea    rdi,[rbp-0x5c]
   18012b4fd:	eb 27                	jmp    0x18012b526
   18012b4ff:	0f b6 c1             	movzx  eax,cl
   18012b502:	41 3b cf             	cmp    ecx,r15d
   18012b505:	41 0f 4f c7          	cmovg  eax,r15d
   18012b509:	88 45 a4             	mov    BYTE PTR [rbp-0x5c],al
   18012b50c:	44 88 7d a7          	mov    BYTE PTR [rbp-0x59],r15b
   18012b510:	48 8d 7d a4          	lea    rdi,[rbp-0x5c]
   18012b514:	eb 10                	jmp    0x18012b526
   18012b516:	48 8b 05 73 4c 4b 00 	mov    rax,QWORD PTR [rip+0x4b4c73]        # 0x1805e0190 ; ?Green@Color@g@@2V12@B
   18012b51d:	8b 08                	mov    ecx,DWORD PTR [rax]
   18012b51f:	89 4d c0             	mov    DWORD PTR [rbp-0x40],ecx
   18012b522:	48 8d 7d c0          	lea    rdi,[rbp-0x40]
   18012b526:	41 0f 28 c2          	movaps xmm0,xmm10
   18012b52a:	f3 41 0f 58 c7       	addss  xmm0,xmm15
   18012b52f:	f3 0f 11 85 80 00 00 	movss  DWORD PTR [rbp+0x80],xmm0
   18012b536:	00 
   18012b537:	41 0f 28 cb          	movaps xmm1,xmm11
   18012b53b:	f3 41 0f 58 cd       	addss  xmm1,xmm13
   18012b540:	f3 0f 11 8d 84 00 00 	movss  DWORD PTR [rbp+0x84],xmm1
   18012b547:	00 
   18012b548:	41 0f 28 c1          	movaps xmm0,xmm9
   18012b54c:	f3 41 0f 58 c6       	addss  xmm0,xmm14
   18012b551:	f3 0f 11 85 88 00 00 	movss  DWORD PTR [rbp+0x88],xmm0
   18012b558:	00 
   18012b559:	48 8d 15 70 b3 16 01 	lea    rdx,[rip+0x116b370]        # 0x1812968d0
   18012b560:	48 8d 4d e0          	lea    rcx,[rbp-0x20]
   18012b564:	e8 37 d4 ee ff       	call   0x1800189a0
   18012b569:	f2 0f 10 76 e0       	movsd  xmm6,QWORD PTR [rsi-0x20]
   18012b56e:	8b 46 e8             	mov    eax,DWORD PTR [rsi-0x18]
   18012b571:	89 85 f8 01 00 00    	mov    DWORD PTR [rbp+0x1f8],eax
   18012b577:	f3 0f 10 7d e0       	movss  xmm7,DWORD PTR [rbp-0x20]
   18012b57c:	f2 0f 11 b5 f0 01 00 	movsd  QWORD PTR [rbp+0x1f0],xmm6
   18012b583:	00 
   18012b584:	f3 0f 59 fe          	mulss  xmm7,xmm6
   18012b588:	f3 0f 10 4d ec       	movss  xmm1,DWORD PTR [rbp-0x14]
   18012b58d:	f3 0f 10 a5 f4 01 00 	movss  xmm4,DWORD PTR [rbp+0x1f4]
   18012b594:	00 
   18012b595:	f3 0f 59 cc          	mulss  xmm1,xmm4
   18012b599:	f3 0f 10 45 f8       	movss  xmm0,DWORD PTR [rbp-0x8]
   18012b59e:	f3 0f 10 9d f8 01 00 	movss  xmm3,DWORD PTR [rbp+0x1f8]
   18012b5a5:	00 
   18012b5a6:	f3 0f 59 c3          	mulss  xmm0,xmm3
   18012b5aa:	f3 0f 58 f9          	addss  xmm7,xmm1
   18012b5ae:	f3 0f 58 f8          	addss  xmm7,xmm0
   18012b5b2:	f3 0f 58 7d 04       	addss  xmm7,DWORD PTR [rbp+0x4]
   18012b5b7:	f3 0f 10 6d e4       	movss  xmm5,DWORD PTR [rbp-0x1c]
   18012b5bc:	f3 0f 59 ee          	mulss  xmm5,xmm6
   18012b5c0:	f3 0f 10 4d f0       	movss  xmm1,DWORD PTR [rbp-0x10]
   18012b5c5:	f3 0f 59 cc          	mulss  xmm1,xmm4
   18012b5c9:	f3 0f 10 45 fc       	movss  xmm0,DWORD PTR [rbp-0x4]
   18012b5ce:	f3 0f 59 c3          	mulss  xmm0,xmm3
   18012b5d2:	f3 0f 58 e9          	addss  xmm5,xmm1
   18012b5d6:	f3 0f 58 e8          	addss  xmm5,xmm0
   18012b5da:	f3 0f 58 6d 08       	addss  xmm5,DWORD PTR [rbp+0x8]
   18012b5df:	f3 0f 10 55 e8       	movss  xmm2,DWORD PTR [rbp-0x18]
   18012b5e4:	f3 0f 59 d6          	mulss  xmm2,xmm6
   18012b5e8:	f3 0f 10 4d f4       	movss  xmm1,DWORD PTR [rbp-0xc]
   18012b5ed:	f3 0f 59 cc          	mulss  xmm1,xmm4
   18012b5f1:	f3 0f 10 45 00       	movss  xmm0,DWORD PTR [rbp+0x0]
   18012b5f6:	f3 0f 59 c3          	mulss  xmm0,xmm3
   18012b5fa:	f3 0f 58 d1          	addss  xmm2,xmm1
   18012b5fe:	f3 0f 58 d0          	addss  xmm2,xmm0
   18012b602:	f3 0f 58 55 0c       	addss  xmm2,DWORD PTR [rbp+0xc]
   18012b607:	f3 44 0f 58 d7       	addss  xmm10,xmm7
   18012b60c:	f3 44 0f 11 55 90    	movss  DWORD PTR [rbp-0x70],xmm10
   18012b612:	f3 44 0f 58 dd       	addss  xmm11,xmm5
   18012b617:	f3 44 0f 11 5d 94    	movss  DWORD PTR [rbp-0x6c],xmm11
   18012b61d:	f3 44 0f 58 ca       	addss  xmm9,xmm2
   18012b622:	f3 44 0f 11 4d 98    	movss  DWORD PTR [rbp-0x68],xmm9
   18012b628:	c7 44 24 20 01 00 00 	mov    DWORD PTR [rsp+0x20],0x1
   18012b62f:	00 
   18012b630:	45 33 c9             	xor    r9d,r9d
   18012b633:	4c 8b c7             	mov    r8,rdi
   18012b636:	48 8d 95 80 00 00 00 	lea    rdx,[rbp+0x80]
   18012b63d:	48 8d 4d 90          	lea    rcx,[rbp-0x70]
   18012b641:	ff 15 d1 50 4b 00    	call   QWORD PTR [rip+0x4b50d1]        # 0x1805e0718 ; ?drawLine@d@@YAXAEBV?$Vector3Template@M@m@@0AEBVColor@g@@W4BlendMode@1@W4DepthMode@1@@Z
   18012b647:	41 ff c6             	inc    r14d
   18012b64a:	41 83 fe 10          	cmp    r14d,0x10
   18012b64e:	0f 82 06 fb ff ff    	jb     0x18012b15a
   18012b654:	f3 44 0f 10 1d 17 37 	movss  xmm11,DWORD PTR [rip+0x553717]        # 0x18067ed74
   18012b65b:	55 00 
   18012b65d:	48 8d 0d 9c ba 16 01 	lea    rcx,[rip+0x116ba9c]        # 0x181297100
   18012b664:	e8 b7 86 ee ff       	call   0x180013d20
   18012b669:	84 c0                	test   al,al
   18012b66b:	0f 84 ed 00 00 00    	je     0x18012b75e
   18012b671:	48 8d 15 58 b2 16 01 	lea    rdx,[rip+0x116b258]        # 0x1812968d0
   18012b678:	48 8d 4d e0          	lea    rcx,[rbp-0x20]
   18012b67c:	e8 1f d3 ee ff       	call   0x1800189a0
   18012b681:	f2 0f 10 76 e0       	movsd  xmm6,QWORD PTR [rsi-0x20]
   18012b686:	8b 46 e8             	mov    eax,DWORD PTR [rsi-0x18]
   18012b689:	89 85 3c 01 00 00    	mov    DWORD PTR [rbp+0x13c],eax
   18012b68f:	f2 0f 11 b5 34 01 00 	movsd  QWORD PTR [rbp+0x134],xmm6
   18012b696:	00 
   18012b697:	0f 28 ee             	movaps xmm5,xmm6
   18012b69a:	f3 0f 59 6d e0       	mulss  xmm5,DWORD PTR [rbp-0x20]
   18012b69f:	f3 0f 10 a5 38 01 00 	movss  xmm4,DWORD PTR [rbp+0x138]
   18012b6a6:	00 
   18012b6a7:	0f 28 cc             	movaps xmm1,xmm4
   18012b6aa:	f3 0f 59 4d ec       	mulss  xmm1,DWORD PTR [rbp-0x14]
   18012b6af:	f3 0f 10 9d 3c 01 00 	movss  xmm3,DWORD PTR [rbp+0x13c]
   18012b6b6:	00 
   18012b6b7:	0f 28 c3             	movaps xmm0,xmm3
   18012b6ba:	f3 0f 59 45 f8       	mulss  xmm0,DWORD PTR [rbp-0x8]
   18012b6bf:	f3 0f 58 e9          	addss  xmm5,xmm1
   18012b6c3:	f3 0f 58 e8          	addss  xmm5,xmm0
   18012b6c7:	f3 0f 58 6d 04       	addss  xmm5,DWORD PTR [rbp+0x4]
   18012b6cc:	0f 28 d6             	movaps xmm2,xmm6
   18012b6cf:	f3 0f 59 55 e4       	mulss  xmm2,DWORD PTR [rbp-0x1c]
   18012b6d4:	0f 28 cc             	movaps xmm1,xmm4
   18012b6d7:	f3 0f 59 4d f0       	mulss  xmm1,DWORD PTR [rbp-0x10]
   18012b6dc:	0f 28 c3             	movaps xmm0,xmm3
   18012b6df:	f3 0f 59 45 fc       	mulss  xmm0,DWORD PTR [rbp-0x4]
   18012b6e4:	f3 0f 58 d1          	addss  xmm2,xmm1
   18012b6e8:	f3 0f 58 d0          	addss  xmm2,xmm0
   18012b6ec:	f3 0f 58 55 08       	addss  xmm2,DWORD PTR [rbp+0x8]
   18012b6f1:	f3 0f 59 75 e8       	mulss  xmm6,DWORD PTR [rbp-0x18]
   18012b6f6:	f3 0f 59 65 f4       	mulss  xmm4,DWORD PTR [rbp-0xc]
   18012b6fb:	f3 0f 59 5d 00       	mulss  xmm3,DWORD PTR [rbp+0x0]
   18012b700:	f3 0f 58 f4          	addss  xmm6,xmm4
   18012b704:	f3 0f 58 f3          	addss  xmm6,xmm3
   18012b708:	f3 0f 58 75 0c       	addss  xmm6,DWORD PTR [rbp+0xc]
   18012b70d:	f3 0f 11 ad 34 01 00 	movss  DWORD PTR [rbp+0x134],xmm5
   18012b714:	00 
   18012b715:	f3 0f 11 95 38 01 00 	movss  DWORD PTR [rbp+0x138],xmm2
   18012b71c:	00 
   18012b71d:	f3 0f 11 b5 3c 01 00 	movss  DWORD PTR [rbp+0x13c],xmm6
   18012b724:	00 
   18012b725:	48 8d 15 ec b1 16 01 	lea    rdx,[rip+0x116b1ec]        # 0x181296918
   18012b72c:	48 8d 8d e8 02 00 00 	lea    rcx,[rbp+0x2e8]
   18012b733:	e8 08 d4 ee ff       	call   0x180018b40
   18012b738:	c7 44 24 20 01 00 00 	mov    DWORD PTR [rsp+0x20],0x1
   18012b73f:	00 
   18012b740:	45 33 c9             	xor    r9d,r9d
   18012b743:	4c 8b 05 b6 5b 4b 00 	mov    r8,QWORD PTR [rip+0x4b5bb6]        # 0x1805e1300 ; ?White@Color@g@@2V12@B
   18012b74a:	48 8d 95 34 01 00 00 	lea    rdx,[rbp+0x134]
   18012b751:	48 8d 8d e8 02 00 00 	lea    rcx,[rbp+0x2e8]
   18012b758:	ff 15 ba 4f 4b 00    	call   QWORD PTR [rip+0x4b4fba]        # 0x1805e0718 ; ?drawLine@d@@YAXAEBV?$Vector3Template@M@m@@0AEBVColor@g@@W4BlendMode@1@W4DepthMode@1@@Z
   18012b75e:	48 83 c6 28          	add    rsi,0x28
   18012b762:	48 8d 46 e0          	lea    rax,[rsi-0x20]
   18012b766:	48 3b c3             	cmp    rax,rbx
   18012b769:	f3 44 0f 10 15 3e 3a 	movss  xmm10,DWORD PTR [rip+0x553a3e]        # 0x18067f1b0
   18012b770:	55 00 
   18012b772:	45 0f 57 c0          	xorps  xmm8,xmm8
   18012b776:	f3 0f 10 35 3a 37 55 	movss  xmm6,DWORD PTR [rip+0x55373a]        # 0x18067eeb8
   18012b77d:	00 
   18012b77e:	0f 85 a5 e8 ff ff    	jne    0x18012a029
   18012b784:	f3 0f 10 bd 40 01 00 	movss  xmm7,DWORD PTR [rbp+0x140]
   18012b78b:	00 
   18012b78c:	f3 44 0f 10 0d 7f 35 	movss  xmm9,DWORD PTR [rip+0x55357f]        # 0x18067ed14
   18012b793:	55 00 
   18012b795:	4c 8b bd f0 03 00 00 	mov    r15,QWORD PTR [rbp+0x3f0]
   18012b79c:	4c 8b a5 c8 01 00 00 	mov    r12,QWORD PTR [rbp+0x1c8]
   18012b7a3:	44 0f b6 ad 08 04 00 	movzx  r13d,BYTE PTR [rbp+0x408]
   18012b7aa:	00 
   18012b7ab:	48 8d 0d 8e b8 16 01 	lea    rcx,[rip+0x116b88e]        # 0x181297040
   18012b7b2:	e8 69 85 ee ff       	call   0x180013d20
   18012b7b7:	84 c0                	test   al,al
   18012b7b9:	0f 84 c3 00 00 00    	je     0x18012b882
   18012b7bf:	66 c7 85 f1 03 00 00 	mov    WORD PTR [rbp+0x3f1],0xffff
   18012b7c6:	ff ff 
   18012b7c8:	c6 85 f0 03 00 00 ff 	mov    BYTE PTR [rbp+0x3f0],0xff
   18012b7cf:	c6 85 f3 03 00 00 10 	mov    BYTE PTR [rbp+0x3f3],0x10
   18012b7d6:	48 8d 15 d3 b2 16 01 	lea    rdx,[rip+0x116b2d3]        # 0x181296ab0
   18012b7dd:	48 8d 4d e0          	lea    rcx,[rbp-0x20]
   18012b7e1:	e8 6a d2 ee ff       	call   0x180018a50
   18012b7e6:	45 33 c9             	xor    r9d,r9d
   18012b7e9:	45 8d 41 03          	lea    r8d,[r9+0x3]
   18012b7ed:	48 8d 95 f0 03 00 00 	lea    rdx,[rbp+0x3f0]
   18012b7f4:	48 8d 4d e0          	lea    rcx,[rbp-0x20]
   18012b7f8:	ff 15 62 50 4b 00    	call   QWORD PTR [rip+0x4b5062]        # 0x1805e0860 ; ?fillFrustum@d@@YAXAEBV?$Matrix4Template@M@m@@AEBVColor@g@@W4BlendMode@1@W4DepthMode@1@@Z
   18012b7fe:	66 c7 85 f1 03 00 00 	mov    WORD PTR [rbp+0x3f1],0xffff
   18012b805:	ff ff 
   18012b807:	c6 85 f0 03 00 00 ff 	mov    BYTE PTR [rbp+0x3f0],0xff
   18012b80e:	c6 85 f3 03 00 00 40 	mov    BYTE PTR [rbp+0x3f3],0x40
   18012b815:	48 8d 15 94 b2 16 01 	lea    rdx,[rip+0x116b294]        # 0x181296ab0
   18012b81c:	48 8d 4d e0          	lea    rcx,[rbp-0x20]
   18012b820:	e8 2b d2 ee ff       	call   0x180018a50
   18012b825:	41 b9 01 00 00 00    	mov    r9d,0x1
   18012b82b:	45 8d 41 02          	lea    r8d,[r9+0x2]
   18012b82f:	48 8d 95 f0 03 00 00 	lea    rdx,[rbp+0x3f0]
   18012b836:	48 8d 4d e0          	lea    rcx,[rbp-0x20]
   18012b83a:	ff 15 20 50 4b 00    	call   QWORD PTR [rip+0x4b5020]        # 0x1805e0860 ; ?fillFrustum@d@@YAXAEBV?$Matrix4Template@M@m@@AEBVColor@g@@W4BlendMode@1@W4DepthMode@1@@Z
   18012b840:	66 c7 85 f1 03 00 00 	mov    WORD PTR [rbp+0x3f1],0xffff
   18012b847:	ff ff 
   18012b849:	c6 85 f0 03 00 00 ff 	mov    BYTE PTR [rbp+0x3f0],0xff
   18012b850:	c6 85 f3 03 00 00 80 	mov    BYTE PTR [rbp+0x3f3],0x80
   18012b857:	48 8d 15 52 b2 16 01 	lea    rdx,[rip+0x116b252]        # 0x181296ab0
   18012b85e:	48 8d 4d e0          	lea    rcx,[rbp-0x20]
   18012b862:	e8 e9 d1 ee ff       	call   0x180018a50
   18012b867:	41 b9 01 00 00 00    	mov    r9d,0x1
   18012b86d:	45 33 c0             	xor    r8d,r8d
   18012b870:	48 8d 95 f0 03 00 00 	lea    rdx,[rbp+0x3f0]
   18012b877:	48 8d 4d e0          	lea    rcx,[rbp-0x20]
   18012b87b:	ff 15 e7 4f 4b 00    	call   QWORD PTR [rip+0x4b4fe7]        # 0x1805e0868 ; ?drawFrustum@d@@YAXAEBV?$Matrix4Template@M@m@@AEBVColor@g@@W4BlendMode@1@W4DepthMode@1@@Z
   18012b881:	90                   	nop
   18012b882:	48 8d 8d 50 01 00 00 	lea    rcx,[rbp+0x150]
   18012b889:	e8 b2 d6 f7 ff       	call   0x1800a8f40
   18012b88e:	90                   	nop
   18012b88f:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012b892:	48 81 c1 38 02 00 00 	add    rcx,0x238
   18012b899:	e8 02 7b ee ff       	call   0x1800133a0
   18012b89e:	48 8b c8             	mov    rcx,rax
   18012b8a1:	e8 ea 78 f7 ff       	call   0x1800a3190
   18012b8a6:	48 8d 0d c3 8c 7e 00 	lea    rcx,[rip+0x7e8cc3]        # 0x180914570
   18012b8ad:	e8 6e 84 ee ff       	call   0x180013d20
   18012b8b2:	84 c0                	test   al,al
   18012b8b4:	74 04                	je     0x18012b8ba
   18012b8b6:	33 c0                	xor    eax,eax
   18012b8b8:	eb 0c                	jmp    0x18012b8c6
   18012b8ba:	48 8d 0d bf 7c 7e 00 	lea    rcx,[rip+0x7e7cbf]        # 0x180913580
   18012b8c1:	e8 fa 21 f0 ff       	call   0x18002dac0
   18012b8c6:	45 0f b6 cd          	movzx  r9d,r13b
   18012b8ca:	44 8b c0             	mov    r8d,eax
   18012b8cd:	48 8d 15 44 72 7e 00 	lea    rdx,[rip+0x7e7244]        # 0x180912b18
   18012b8d4:	49 8b cc             	mov    rcx,r12
   18012b8d7:	e8 f4 a2 ee ff       	call   0x180015bd0
   18012b8dc:	4c 8b ad 48 01 00 00 	mov    r13,QWORD PTR [rbp+0x148]
   18012b8e3:	8b 35 53 6f 6d 00    	mov    esi,DWORD PTR [rip+0x6d6f53]        # 0x18080283c
   18012b8e9:	45 33 e4             	xor    r12d,r12d
   18012b8ec:	bb 40 00 00 00       	mov    ebx,0x40
   18012b8f1:	80 3d 11 71 6d 00 00 	cmp    BYTE PTR [rip+0x6d7111],0x0        # 0x180802a09
   18012b8f8:	0f 84 ea 00 00 00    	je     0x18012b9e8
   18012b8fe:	8b ce                	mov    ecx,esi
   18012b900:	65 48 8b 04 25 58 00 	mov    rax,QWORD PTR gs:0x58
   18012b907:	00 00 
   18012b909:	8b d3                	mov    edx,ebx
   18012b90b:	48 8b 0c c8          	mov    rcx,QWORD PTR [rax+rcx*8]
   18012b90f:	8b 04 0a             	mov    eax,DWORD PTR [rdx+rcx*1]
   18012b912:	39 05 68 b9 16 01    	cmp    DWORD PTR [rip+0x116b968],eax        # 0x181297280
   18012b918:	7e 41                	jle    0x18012b95b
   18012b91a:	48 8d 0d 5f b9 16 01 	lea    rcx,[rip+0x116b95f]        # 0x181297280
   18012b921:	e8 3e c5 45 00       	call   0x180587e64
   18012b926:	83 3d 53 b9 16 01 ff 	cmp    DWORD PTR [rip+0x116b953],0xffffffff        # 0x181297280
   18012b92d:	75 2c                	jne    0x18012b95b
   18012b92f:	48 8d 15 1a 48 50 00 	lea    rdx,[rip+0x50481a]        # 0x180630150 ; 'g_fTransparentReflectionSunShadowKernelSize'
   18012b936:	48 8d 0d 4b b9 16 01 	lea    rcx,[rip+0x116b94b]        # 0x181297288
   18012b93d:	e8 7e 77 ee ff       	call   0x1800130c0
   18012b942:	48 8d 0d 17 2c 4a 00 	lea    rcx,[rip+0x4a2c17]        # 0x1805ce560
   18012b949:	e8 76 c2 45 00       	call   0x180587bc4
   18012b94e:	90                   	nop
   18012b94f:	48 8d 0d 2a b9 16 01 	lea    rcx,[rip+0x116b92a]        # 0x181297280
   18012b956:	e8 a9 c4 45 00       	call   0x180587e04
   18012b95b:	0f 28 c7             	movaps xmm0,xmm7
   18012b95e:	f3 41 0f 58 c1       	addss  xmm0,xmm9
   18012b963:	f3 0f 11 85 f0 03 00 	movss  DWORD PTR [rbp+0x3f0],xmm0
   18012b96a:	00 
   18012b96b:	48 8d 0d 16 b9 16 01 	lea    rcx,[rip+0x116b916]        # 0x181297288
   18012b972:	e8 d9 53 ee ff       	call   0x180010d50
   18012b977:	48 8b c8             	mov    rcx,rax
   18012b97a:	e8 11 53 ee ff       	call   0x180010c90
   18012b97f:	8b c8                	mov    ecx,eax
   18012b981:	45 33 c0             	xor    r8d,r8d
   18012b984:	48 8d 95 f0 03 00 00 	lea    rdx,[rbp+0x3f0]
   18012b98b:	ff 15 ff 45 4b 00    	call   QWORD PTR [rip+0x4b45ff]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   18012b991:	f3 0f 10 85 f0 03 00 	movss  xmm0,DWORD PTR [rbp+0x3f0]
   18012b998:	00 
   18012b999:	f3 0f 11 05 f7 b8 16 	movss  DWORD PTR [rip+0x116b8f7],xmm0        # 0x181297298
   18012b9a0:	01 
   18012b9a1:	48 8d 0d b8 71 7e 00 	lea    rcx,[rip+0x7e71b8]        # 0x180912b60
   18012b9a8:	e8 73 83 ee ff       	call   0x180013d20
   18012b9ad:	41 8b fc             	mov    edi,r12d
   18012b9b0:	84 c0                	test   al,al
   18012b9b2:	40 0f 95 c7          	setne  dil
   18012b9b6:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012b9b9:	48 81 c1 20 02 00 00 	add    rcx,0x220
   18012b9c0:	e8 db 79 ee ff       	call   0x1800133a0
   18012b9c5:	48 8b d8             	mov    rbx,rax
   18012b9c8:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012b9cb:	48 81 c1 50 02 00 00 	add    rcx,0x250
   18012b9d2:	e8 c9 79 ee ff       	call   0x1800133a0
   18012b9d7:	48 8b c8             	mov    rcx,rax
   18012b9da:	44 8b cf             	mov    r9d,edi
   18012b9dd:	4c 8b c3             	mov    r8,rbx
   18012b9e0:	49 8b d5             	mov    rdx,r13
   18012b9e3:	e8 a8 99 f7 ff       	call   0x1800a5390
   18012b9e8:	4c 8d 35 69 49 50 00 	lea    r14,[rip+0x504969]        # 0x180630358 ; 'closestHitMain'
   18012b9ef:	80 3d 14 70 6d 00 00 	cmp    BYTE PTR [rip+0x6d7014],0x0        # 0x180802a0a
   18012b9f6:	0f 84 83 15 00 00    	je     0x18012cf7f
   18012b9fc:	48 8d 0d 3d 73 7e 00 	lea    rcx,[rip+0x7e733d]        # 0x180912d40
   18012ba03:	e8 18 83 ee ff       	call   0x180013d20
   18012ba08:	84 c0                	test   al,al
   18012ba0a:	0f 84 51 15 00 00    	je     0x18012cf61
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
   18012ba75:	48 8d 0d 44 7a 7e 00 	lea    rcx,[rip+0x7e7a44]        # 0x1809134c0
   18012ba7c:	e8 3f 20 f0 ff       	call   0x18002dac0
   18012ba81:	89 85 00 04 00 00    	mov    DWORD PTR [rbp+0x400],eax
   18012ba87:	0f 57 c0             	xorps  xmm0,xmm0
   18012ba8a:	66 0f 7f 45 b0       	movdqa XMMWORD PTR [rbp-0x50],xmm0
   18012ba8f:	c7 44 24 70 ff ff ff 	mov    DWORD PTR [rsp+0x70],0xffffffff
   18012ba96:	ff 
   18012ba97:	48 8d 45 b0          	lea    rax,[rbp-0x50]
   18012ba9b:	48 89 44 24 68       	mov    QWORD PTR [rsp+0x68],rax
   18012baa0:	c7 44 24 60 01 00 00 	mov    DWORD PTR [rsp+0x60],0x1
   18012baa7:	00 
   18012baa8:	f3 0f 11 74 24 58    	movss  DWORD PTR [rsp+0x58],xmm6
   18012baae:	44 89 64 24 50       	mov    DWORD PTR [rsp+0x50],r12d
   18012bab3:	44 89 64 24 48       	mov    DWORD PTR [rsp+0x48],r12d
   18012bab8:	4c 89 64 24 40       	mov    QWORD PTR [rsp+0x40],r12
   18012babd:	4c 89 64 24 38       	mov    QWORD PTR [rsp+0x38],r12
   18012bac2:	44 89 64 24 30       	mov    DWORD PTR [rsp+0x30],r12d
   18012bac7:	c7 44 24 28 05 00 00 	mov    DWORD PTR [rsp+0x28],0x5
   18012bace:	00 
   18012bacf:	c6 44 24 20 17       	mov    BYTE PTR [rsp+0x20],0x17
   18012bad4:	45 33 c9             	xor    r9d,r9d
   18012bad7:	44 8b 05 ae 29 7e 00 	mov    r8d,DWORD PTR [rip+0x7e29ae]        # 0x18090e48c
   18012bade:	8b 15 a4 29 7e 00    	mov    edx,DWORD PTR [rip+0x7e29a4]        # 0x18090e488
   18012bae4:	48 8d 4d e0          	lea    rcx,[rbp-0x20]
   18012bae8:	ff 15 3a 42 4b 00    	call   QWORD PTR [rip+0x4b423a]        # 0x1805dfd28 ; ??0Texture2DInit@d3d@@QEAA@HHHW4PixelFormat@1@HHPEBX_KW4MiscFlags@1@HMHV?$Vector4Template@M@m@@W4TileMode@1@@Z
   18012baee:	48 8b d8             	mov    rbx,rax
   18012baf1:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012baf4:	48 83 c1 20          	add    rcx,0x20
   18012baf8:	e8 a3 78 ee ff       	call   0x1800133a0
   18012bafd:	48 8b c8             	mov    rcx,rax
   18012bb00:	4c 8d 05 a9 46 50 00 	lea    r8,[rip+0x5046a9]        # 0x1806301b0 ; 'DiffuseGITarget'
   18012bb07:	48 8b d3             	mov    rdx,rbx
   18012bb0a:	e8 51 dc f7 ff       	call   0x1800a9760
   18012bb0f:	48 8b d0             	mov    rdx,rax
   18012bb12:	48 8d 0d df 2e 7e 00 	lea    rcx,[rip+0x7e2edf]        # 0x18090e9f8
   18012bb19:	e8 72 c5 ee ff       	call   0x180018090
   18012bb1e:	33 d2                	xor    edx,edx
   18012bb20:	48 8d 8d 90 00 00 00 	lea    rcx,[rbp+0x90]
   18012bb27:	e8 c4 c5 ee ff       	call   0x1800180f0
   18012bb2c:	90                   	nop
   18012bb2d:	0f 57 c0             	xorps  xmm0,xmm0
   18012bb30:	66 0f 7f 45 b0       	movdqa XMMWORD PTR [rbp-0x50],xmm0
   18012bb35:	c7 44 24 70 ff ff ff 	mov    DWORD PTR [rsp+0x70],0xffffffff
   18012bb3c:	ff 
   18012bb3d:	48 8d 45 b0          	lea    rax,[rbp-0x50]
   18012bb41:	48 89 44 24 68       	mov    QWORD PTR [rsp+0x68],rax
   18012bb46:	c7 44 24 60 01 00 00 	mov    DWORD PTR [rsp+0x60],0x1
   18012bb4d:	00 
   18012bb4e:	f3 0f 11 74 24 58    	movss  DWORD PTR [rsp+0x58],xmm6
   18012bb54:	44 89 64 24 50       	mov    DWORD PTR [rsp+0x50],r12d
   18012bb59:	44 89 64 24 48       	mov    DWORD PTR [rsp+0x48],r12d
   18012bb5e:	4c 89 64 24 40       	mov    QWORD PTR [rsp+0x40],r12
   18012bb63:	4c 89 64 24 38       	mov    QWORD PTR [rsp+0x38],r12
   18012bb68:	44 89 64 24 30       	mov    DWORD PTR [rsp+0x30],r12d
   18012bb6d:	c7 44 24 28 05 00 00 	mov    DWORD PTR [rsp+0x28],0x5
   18012bb74:	00 
   18012bb75:	c6 44 24 20 06       	mov    BYTE PTR [rsp+0x20],0x6
   18012bb7a:	45 33 c9             	xor    r9d,r9d
   18012bb7d:	44 8b 05 08 29 7e 00 	mov    r8d,DWORD PTR [rip+0x7e2908]        # 0x18090e48c
   18012bb84:	8b 15 fe 28 7e 00    	mov    edx,DWORD PTR [rip+0x7e28fe]        # 0x18090e488
   18012bb8a:	48 8d 8d a0 00 00 00 	lea    rcx,[rbp+0xa0]
   18012bb91:	ff 15 91 41 4b 00    	call   QWORD PTR [rip+0x4b4191]        # 0x1805dfd28 ; ??0Texture2DInit@d3d@@QEAA@HHHW4PixelFormat@1@HHPEBX_KW4MiscFlags@1@HMHV?$Vector4Template@M@m@@W4TileMode@1@@Z
   18012bb97:	48 8b d8             	mov    rbx,rax
   18012bb9a:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012bb9d:	48 83 c1 20          	add    rcx,0x20
   18012bba1:	e8 fa 77 ee ff       	call   0x1800133a0
   18012bba6:	48 8b c8             	mov    rcx,rax
   18012bba9:	4c 8d 05 f0 45 50 00 	lea    r8,[rip+0x5045f0]        # 0x1806301a0 ; 'IDHitInfo'
   18012bbb0:	48 8b d3             	mov    rdx,rbx
   18012bbb3:	e8 a8 db f7 ff       	call   0x1800a9760
   18012bbb8:	48 8b d0             	mov    rdx,rax
   18012bbbb:	48 8d 8d 90 00 00 00 	lea    rcx,[rbp+0x90]
   18012bbc2:	e8 c9 c4 ee ff       	call   0x180018090
   18012bbc7:	33 d2                	xor    edx,edx
   18012bbc9:	48 8d 4d c0          	lea    rcx,[rbp-0x40]
   18012bbcd:	e8 1e c5 ee ff       	call   0x1800180f0
   18012bbd2:	90                   	nop
   18012bbd3:	83 bd 00 04 00 00 01 	cmp    DWORD PTR [rbp+0x400],0x1
   18012bbda:	0f 86 94 00 00 00    	jbe    0x18012bc74
   18012bbe0:	0f 57 c0             	xorps  xmm0,xmm0
   18012bbe3:	66 0f 7f 45 b0       	movdqa XMMWORD PTR [rbp-0x50],xmm0
   18012bbe8:	c7 44 24 70 ff ff ff 	mov    DWORD PTR [rsp+0x70],0xffffffff
   18012bbef:	ff 
   18012bbf0:	48 8d 45 b0          	lea    rax,[rbp-0x50]
   18012bbf4:	48 89 44 24 68       	mov    QWORD PTR [rsp+0x68],rax
   18012bbf9:	c7 44 24 60 01 00 00 	mov    DWORD PTR [rsp+0x60],0x1
   18012bc00:	00 
   18012bc01:	f3 0f 11 74 24 58    	movss  DWORD PTR [rsp+0x58],xmm6
   18012bc07:	44 89 64 24 50       	mov    DWORD PTR [rsp+0x50],r12d
   18012bc0c:	44 89 64 24 48       	mov    DWORD PTR [rsp+0x48],r12d
   18012bc11:	4c 89 64 24 40       	mov    QWORD PTR [rsp+0x40],r12
   18012bc16:	4c 89 64 24 38       	mov    QWORD PTR [rsp+0x38],r12
   18012bc1b:	44 89 64 24 30       	mov    DWORD PTR [rsp+0x30],r12d
   18012bc20:	c7 44 24 28 05 00 00 	mov    DWORD PTR [rsp+0x28],0x5
   18012bc27:	00 
   18012bc28:	c6 44 24 20 06       	mov    BYTE PTR [rsp+0x20],0x6
   18012bc2d:	45 33 c9             	xor    r9d,r9d
   18012bc30:	44 8b 05 55 28 7e 00 	mov    r8d,DWORD PTR [rip+0x7e2855]        # 0x18090e48c
   18012bc37:	8b 15 4b 28 7e 00    	mov    edx,DWORD PTR [rip+0x7e284b]        # 0x18090e488
   18012bc3d:	48 8d 4d e0          	lea    rcx,[rbp-0x20]
   18012bc41:	ff 15 e1 40 4b 00    	call   QWORD PTR [rip+0x4b40e1]        # 0x1805dfd28 ; ??0Texture2DInit@d3d@@QEAA@HHHW4PixelFormat@1@HHPEBX_KW4MiscFlags@1@HMHV?$Vector4Template@M@m@@W4TileMode@1@@Z
   18012bc47:	48 8b d8             	mov    rbx,rax
   18012bc4a:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012bc4d:	48 83 c1 20          	add    rcx,0x20
   18012bc51:	e8 4a 77 ee ff       	call   0x1800133a0
   18012bc56:	48 8b c8             	mov    rcx,rax
   18012bc59:	4c 8d 05 78 45 50 00 	lea    r8,[rip+0x504578]        # 0x1806301d8 ; 'DiffuseGIWeightTarget'
   18012bc60:	48 8b d3             	mov    rdx,rbx
   18012bc63:	e8 f8 da f7 ff       	call   0x1800a9760
   18012bc68:	48 8b d0             	mov    rdx,rax
   18012bc6b:	48 8d 4d c0          	lea    rcx,[rbp-0x40]
   18012bc6f:	e8 1c c4 ee ff       	call   0x180018090
   18012bc74:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012bc77:	48 81 c1 58 02 00 00 	add    rcx,0x258
   18012bc7e:	e8 1d 77 ee ff       	call   0x1800133a0
   18012bc83:	48 85 c0             	test   rax,rax
   18012bc86:	0f 85 af 00 00 00    	jne    0x18012bd3b
   18012bc8c:	83 bd 00 04 00 00 01 	cmp    DWORD PTR [rbp+0x400],0x1
   18012bc93:	0f 86 a2 00 00 00    	jbe    0x18012bd3b
   18012bc99:	88 85 f0 03 00 00    	mov    BYTE PTR [rbp+0x3f0],al
   18012bc9f:	33 d2                	xor    edx,edx
   18012bca1:	48 8d 8d f0 03 00 00 	lea    rcx,[rbp+0x3f0]
   18012bca8:	e8 93 71 ee ff       	call   0x180012e40
   18012bcad:	48 8b d0             	mov    rdx,rax
   18012bcb0:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012bcb3:	48 81 c1 58 02 00 00 	add    rcx,0x258
   18012bcba:	e8 a1 76 ee ff       	call   0x180013360
   18012bcbf:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012bcc2:	48 81 c1 58 02 00 00 	add    rcx,0x258
   18012bcc9:	e8 d2 76 ee ff       	call   0x1800133a0
   18012bcce:	48 8b c8             	mov    rcx,rax
   18012bcd1:	ba 10 00 00 00       	mov    edx,0x10
   18012bcd6:	44 8d 42 f4          	lea    r8d,[rdx-0xc]
   18012bcda:	ff 15 40 40 4b 00    	call   QWORD PTR [rip+0x4b4040]        # 0x1805dfd20 ; ?allocUAVShaderResource@NativeBufferUtil@d3d@@SAXPEAVNativeBuffer@2@_K1@Z
   18012bce0:	c6 85 f0 03 00 00 00 	mov    BYTE PTR [rbp+0x3f0],0x0
   18012bce7:	33 d2                	xor    edx,edx
   18012bce9:	48 8d 8d f0 03 00 00 	lea    rcx,[rbp+0x3f0]
   18012bcf0:	e8 4b 71 ee ff       	call   0x180012e40
   18012bcf5:	48 8b d0             	mov    rdx,rax
   18012bcf8:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012bcfb:	48 81 c1 60 02 00 00 	add    rcx,0x260
   18012bd02:	e8 59 76 ee ff       	call   0x180013360
   18012bd07:	8b 05 7b 27 7e 00    	mov    eax,DWORD PTR [rip+0x7e277b]        # 0x18090e488
   18012bd0d:	0f af 05 78 27 7e 00 	imul   eax,DWORD PTR [rip+0x7e2778]        # 0x18090e48c
   18012bd14:	c1 e0 02             	shl    eax,0x2
   18012bd17:	48 63 d8             	movsxd rbx,eax
   18012bd1a:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012bd1d:	48 81 c1 60 02 00 00 	add    rcx,0x260
   18012bd24:	e8 77 76 ee ff       	call   0x1800133a0
   18012bd29:	48 8b c8             	mov    rcx,rax
   18012bd2c:	41 b8 04 00 00 00    	mov    r8d,0x4
   18012bd32:	48 8b d3             	mov    rdx,rbx
   18012bd35:	ff 15 e5 3f 4b 00    	call   QWORD PTR [rip+0x4b3fe5]        # 0x1805dfd20 ; ?allocUAVShaderResource@NativeBufferUtil@d3d@@SAXPEAVNativeBuffer@2@_K1@Z
   18012bd3b:	8b ce                	mov    ecx,esi
   18012bd3d:	65 48 8b 04 25 58 00 	mov    rax,QWORD PTR gs:0x58
   18012bd44:	00 00 
   18012bd46:	ba 40 00 00 00       	mov    edx,0x40
   18012bd4b:	8b fa                	mov    edi,edx
   18012bd4d:	48 8b 34 c8          	mov    rsi,QWORD PTR [rax+rcx*8]
   18012bd51:	8b 04 16             	mov    eax,DWORD PTR [rsi+rdx*1]
   18012bd54:	39 05 46 b5 16 01    	cmp    DWORD PTR [rip+0x116b546],eax        # 0x1812972a0
   18012bd5a:	7e 41                	jle    0x18012bd9d
   18012bd5c:	48 8d 0d 3d b5 16 01 	lea    rcx,[rip+0x116b53d]        # 0x1812972a0
   18012bd63:	e8 fc c0 45 00       	call   0x180587e64
   18012bd68:	83 3d 31 b5 16 01 ff 	cmp    DWORD PTR [rip+0x116b531],0xffffffff        # 0x1812972a0
   18012bd6f:	75 2c                	jne    0x18012bd9d
   18012bd71:	48 8d 15 48 44 50 00 	lea    rdx,[rip+0x504448]        # 0x1806301c0 ; 'g_fDiffuseGIRadius'
   18012bd78:	48 8d 0d 29 b5 16 01 	lea    rcx,[rip+0x116b529]        # 0x1812972a8
   18012bd7f:	e8 3c 73 ee ff       	call   0x1800130c0
   18012bd84:	48 8d 0d a5 27 4a 00 	lea    rcx,[rip+0x4a27a5]        # 0x1805ce530
   18012bd8b:	e8 34 be 45 00       	call   0x180587bc4
   18012bd90:	90                   	nop
   18012bd91:	48 8d 0d 08 b5 16 01 	lea    rcx,[rip+0x116b508]        # 0x1812972a0
   18012bd98:	e8 67 c0 45 00       	call   0x180587e04
   18012bd9d:	48 8d 0d cc 87 7e 00 	lea    rcx,[rip+0x7e87cc]        # 0x180914570
   18012bda4:	e8 77 7f ee ff       	call   0x180013d20
   18012bda9:	84 c0                	test   al,al
   18012bdab:	75 09                	jne    0x18012bdb6
   18012bdad:	f3 44 0f 10 15 d2 31 	movss  xmm10,DWORD PTR [rip+0x5531d2]        # 0x18067ef88
   18012bdb4:	55 00 
   18012bdb6:	48 8d 0d 73 70 7e 00 	lea    rcx,[rip+0x7e7073]        # 0x180912e30
   18012bdbd:	e8 de 74 ee ff       	call   0x1800132a0
   18012bdc2:	f3 41 0f 59 c2       	mulss  xmm0,xmm10
   18012bdc7:	f3 0f 11 85 f0 03 00 	movss  DWORD PTR [rbp+0x3f0],xmm0
   18012bdce:	00 
   18012bdcf:	48 8d 0d d2 b4 16 01 	lea    rcx,[rip+0x116b4d2]        # 0x1812972a8
   18012bdd6:	e8 75 4f ee ff       	call   0x180010d50
   18012bddb:	48 8b c8             	mov    rcx,rax
   18012bdde:	e8 ad 4e ee ff       	call   0x180010c90
   18012bde3:	8b c8                	mov    ecx,eax
   18012bde5:	45 33 c0             	xor    r8d,r8d
   18012bde8:	48 8d 95 f0 03 00 00 	lea    rdx,[rbp+0x3f0]
   18012bdef:	ff 15 9b 41 4b 00    	call   QWORD PTR [rip+0x4b419b]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   18012bdf5:	f3 0f 10 85 f0 03 00 	movss  xmm0,DWORD PTR [rbp+0x3f0]
   18012bdfc:	00 
   18012bdfd:	f3 0f 11 05 b3 b4 16 	movss  DWORD PTR [rip+0x116b4b3],xmm0        # 0x1812972b8
   18012be04:	01 
   18012be05:	8b 04 3e             	mov    eax,DWORD PTR [rsi+rdi*1]
   18012be08:	39 05 b2 b4 16 01    	cmp    DWORD PTR [rip+0x116b4b2],eax        # 0x1812972c0
   18012be0e:	7e 41                	jle    0x18012be51
   18012be10:	48 8d 0d a9 b4 16 01 	lea    rcx,[rip+0x116b4a9]        # 0x1812972c0
   18012be17:	e8 48 c0 45 00       	call   0x180587e64
   18012be1c:	83 3d 9d b4 16 01 ff 	cmp    DWORD PTR [rip+0x116b49d],0xffffffff        # 0x1812972c0
   18012be23:	75 2c                	jne    0x18012be51
   18012be25:	48 8d 15 dc 43 50 00 	lea    rdx,[rip+0x5043dc]        # 0x180630208 ; 'g_fDiffuseGIOffset'
   18012be2c:	48 8d 0d 95 b4 16 01 	lea    rcx,[rip+0x116b495]        # 0x1812972c8
   18012be33:	e8 88 72 ee ff       	call   0x1800130c0
   18012be38:	48 8d 0d c1 26 4a 00 	lea    rcx,[rip+0x4a26c1]        # 0x1805ce500
   18012be3f:	e8 80 bd 45 00       	call   0x180587bc4
   18012be44:	90                   	nop
   18012be45:	48 8d 0d 74 b4 16 01 	lea    rcx,[rip+0x116b474]        # 0x1812972c0
   18012be4c:	e8 b3 bf 45 00       	call   0x180587e04
   18012be51:	48 8d 0d 98 70 7e 00 	lea    rcx,[rip+0x7e7098]        # 0x180912ef0
   18012be58:	e8 43 74 ee ff       	call   0x1800132a0
   18012be5d:	f3 0f 11 85 f0 03 00 	movss  DWORD PTR [rbp+0x3f0],xmm0
   18012be64:	00 
   18012be65:	48 8d 0d 5c b4 16 01 	lea    rcx,[rip+0x116b45c]        # 0x1812972c8
   18012be6c:	e8 df 4e ee ff       	call   0x180010d50
   18012be71:	48 8b c8             	mov    rcx,rax
   18012be74:	e8 17 4e ee ff       	call   0x180010c90
   18012be79:	8b c8                	mov    ecx,eax
   18012be7b:	45 33 c0             	xor    r8d,r8d
   18012be7e:	48 8d 95 f0 03 00 00 	lea    rdx,[rbp+0x3f0]
   18012be85:	ff 15 05 41 4b 00    	call   QWORD PTR [rip+0x4b4105]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   18012be8b:	f3 0f 10 85 f0 03 00 	movss  xmm0,DWORD PTR [rbp+0x3f0]
   18012be92:	00 
   18012be93:	f3 0f 11 05 3d b4 16 	movss  DWORD PTR [rip+0x116b43d],xmm0        # 0x1812972d8
   18012be9a:	01 
   18012be9b:	8b 04 3e             	mov    eax,DWORD PTR [rsi+rdi*1]
   18012be9e:	39 05 3c b4 16 01    	cmp    DWORD PTR [rip+0x116b43c],eax        # 0x1812972e0
   18012bea4:	7e 41                	jle    0x18012bee7
   18012bea6:	48 8d 0d 33 b4 16 01 	lea    rcx,[rip+0x116b433]        # 0x1812972e0
   18012bead:	e8 b2 bf 45 00       	call   0x180587e64
   18012beb2:	83 3d 27 b4 16 01 ff 	cmp    DWORD PTR [rip+0x116b427],0xffffffff        # 0x1812972e0
   18012beb9:	75 2c                	jne    0x18012bee7
   18012bebb:	48 8d 15 2e 43 50 00 	lea    rdx,[rip+0x50432e]        # 0x1806301f0 ; 'g_uRTDiffuseRayCount'
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
   18012bf07:	ff 15 83 40 4b 00    	call   QWORD PTR [rip+0x4b4083]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   18012bf0d:	8b 85 08 04 00 00    	mov    eax,DWORD PTR [rbp+0x408]
   18012bf13:	89 05 df b3 16 01    	mov    DWORD PTR [rip+0x116b3df],eax        # 0x1812972f8
   18012bf19:	8b 04 3e             	mov    eax,DWORD PTR [rsi+rdi*1]
   18012bf1c:	39 05 de b3 16 01    	cmp    DWORD PTR [rip+0x116b3de],eax        # 0x181297300
   18012bf22:	7e 41                	jle    0x18012bf65
   18012bf24:	48 8d 0d d5 b3 16 01 	lea    rcx,[rip+0x116b3d5]        # 0x181297300
   18012bf2b:	e8 34 bf 45 00       	call   0x180587e64
   18012bf30:	83 3d c9 b3 16 01 ff 	cmp    DWORD PTR [rip+0x116b3c9],0xffffffff        # 0x181297300
   18012bf37:	75 2c                	jne    0x18012bf65
   18012bf39:	48 8d 15 f0 42 50 00 	lea    rdx,[rip+0x5042f0]        # 0x180630230 ; 'g_uDGIPassCount'
   18012bf40:	48 8d 0d c1 b3 16 01 	lea    rcx,[rip+0x116b3c1]        # 0x181297308
   18012bf47:	e8 04 58 f0 ff       	call   0x180031750
   18012bf4c:	48 8d 0d 4d 25 4a 00 	lea    rcx,[rip+0x4a254d]        # 0x1805ce4a0
   18012bf53:	e8 6c bc 45 00       	call   0x180587bc4
   18012bf58:	90                   	nop
   18012bf59:	48 8d 0d a0 b3 16 01 	lea    rcx,[rip+0x116b3a0]        # 0x181297300
   18012bf60:	e8 9f be 45 00       	call   0x180587e04
   18012bf65:	48 8d 0d 9c b3 16 01 	lea    rcx,[rip+0x116b39c]        # 0x181297308
   18012bf6c:	e8 df 4d ee ff       	call   0x180010d50
   18012bf71:	48 8b c8             	mov    rcx,rax
   18012bf74:	e8 17 4d ee ff       	call   0x180010c90
   18012bf79:	8b c8                	mov    ecx,eax
   18012bf7b:	45 33 c0             	xor    r8d,r8d
   18012bf7e:	48 8d 95 00 04 00 00 	lea    rdx,[rbp+0x400]
   18012bf85:	ff 15 05 40 4b 00    	call   QWORD PTR [rip+0x4b4005]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   18012bf8b:	8b 85 00 04 00 00    	mov    eax,DWORD PTR [rbp+0x400]
   18012bf91:	89 05 81 b3 16 01    	mov    DWORD PTR [rip+0x116b381],eax        # 0x181297318
   18012bf97:	44 89 a5 f0 03 00 00 	mov    DWORD PTR [rbp+0x3f0],r12d
   18012bf9e:	85 c0                	test   eax,eax
   18012bfa0:	0f 84 97 0c 00 00    	je     0x18012cc3d
   18012bfa6:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012bfa9:	48 81 c1 40 02 00 00 	add    rcx,0x240
   18012bfb0:	e8 eb 73 ee ff       	call   0x1800133a0
   18012bfb5:	48 8b c8             	mov    rcx,rax
   18012bfb8:	e8 b3 72 f7 ff       	call   0x1800a3270
   18012bfbd:	8b 04 3e             	mov    eax,DWORD PTR [rsi+rdi*1]
   18012bfc0:	39 05 5a b3 16 01    	cmp    DWORD PTR [rip+0x116b35a],eax        # 0x181297320
   18012bfc6:	7e 41                	jle    0x18012c009
   18012bfc8:	48 8d 0d 51 b3 16 01 	lea    rcx,[rip+0x116b351]        # 0x181297320
   18012bfcf:	e8 90 be 45 00       	call   0x180587e64
   18012bfd4:	83 3d 45 b3 16 01 ff 	cmp    DWORD PTR [rip+0x116b345],0xffffffff        # 0x181297320
   18012bfdb:	75 2c                	jne    0x18012c009
   18012bfdd:	48 8d 15 3c 42 50 00 	lea    rdx,[rip+0x50423c]        # 0x180630220 ; 'g_uDGIPass'
   18012bfe4:	48 8d 0d 3d b3 16 01 	lea    rcx,[rip+0x116b33d]        # 0x181297328
   18012bfeb:	e8 60 57 f0 ff       	call   0x180031750
   18012bff0:	48 8d 0d 79 24 4a 00 	lea    rcx,[rip+0x4a2479]        # 0x1805ce470
   18012bff7:	e8 c8 bb 45 00       	call   0x180587bc4
   18012bffc:	90                   	nop
   18012bffd:	48 8d 0d 1c b3 16 01 	lea    rcx,[rip+0x116b31c]        # 0x181297320
   18012c004:	e8 fb bd 45 00       	call   0x180587e04
   18012c009:	48 8d 0d 18 b3 16 01 	lea    rcx,[rip+0x116b318]        # 0x181297328
   18012c010:	e8 3b 4d ee ff       	call   0x180010d50
   18012c015:	48 8b c8             	mov    rcx,rax
   18012c018:	e8 73 4c ee ff       	call   0x180010c90
   18012c01d:	8b c8                	mov    ecx,eax
   18012c01f:	45 33 c0             	xor    r8d,r8d
   18012c022:	48 8d 95 f0 03 00 00 	lea    rdx,[rbp+0x3f0]
   18012c029:	ff 15 61 3f 4b 00    	call   QWORD PTR [rip+0x4b3f61]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   18012c02f:	8b 85 f0 03 00 00    	mov    eax,DWORD PTR [rbp+0x3f0]
   18012c035:	89 05 fd b2 16 01    	mov    DWORD PTR [rip+0x116b2fd],eax        # 0x181297338
   18012c03b:	48 8d 0d b6 29 7e 00 	lea    rcx,[rip+0x7e29b6]        # 0x18090e9f8
   18012c042:	e8 59 73 ee ff       	call   0x1800133a0
   18012c047:	48 8b c8             	mov    rcx,rax
   18012c04a:	ff 15 e0 3c 4b 00    	call   QWORD PTR [rip+0x4b3ce0]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   18012c050:	48 8b c8             	mov    rcx,rax
   18012c053:	ff 15 1f 3f 4b 00    	call   QWORD PTR [rip+0x4b3f1f]        # 0x1805dff78 ; ?prepareForConsuming@NativeTexture@d3d@@QEAAXXZ
   18012c059:	83 bd 00 04 00 00 01 	cmp    DWORD PTR [rbp+0x400],0x1
   18012c060:	76 1b                	jbe    0x18012c07d
   18012c062:	48 8d 4d c0          	lea    rcx,[rbp-0x40]
   18012c066:	e8 35 73 ee ff       	call   0x1800133a0
   18012c06b:	48 8b c8             	mov    rcx,rax
   18012c06e:	ff 15 bc 3c 4b 00    	call   QWORD PTR [rip+0x4b3cbc]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   18012c074:	48 8b c8             	mov    rcx,rax
   18012c077:	ff 15 fb 3e 4b 00    	call   QWORD PTR [rip+0x4b3efb]        # 0x1805dff78 ; ?prepareForConsuming@NativeTexture@d3d@@QEAAXXZ
   18012c07d:	8b 85 f0 03 00 00    	mov    eax,DWORD PTR [rbp+0x3f0]
   18012c083:	85 c0                	test   eax,eax
   18012c085:	0f 84 52 04 00 00    	je     0x18012c4dd
   18012c08b:	8b 04 3e             	mov    eax,DWORD PTR [rsi+rdi*1]
   18012c08e:	39 05 ac b2 16 01    	cmp    DWORD PTR [rip+0x116b2ac],eax        # 0x181297340
   18012c094:	7e 41                	jle    0x18012c0d7
   18012c096:	48 8d 0d a3 b2 16 01 	lea    rcx,[rip+0x116b2a3]        # 0x181297340
   18012c09d:	e8 c2 bd 45 00       	call   0x180587e64
   18012c0a2:	83 3d 97 b2 16 01 ff 	cmp    DWORD PTR [rip+0x116b297],0xffffffff        # 0x181297340
   18012c0a9:	75 2c                	jne    0x18012c0d7
   18012c0ab:	48 8d 15 ae 41 50 00 	lea    rdx,[rip+0x5041ae]        # 0x180630260 ; 'g_tDiffuseGIColorSource'
   18012c0b2:	48 8d 0d 8f b2 16 01 	lea    rcx,[rip+0x116b28f]        # 0x181297348
   18012c0b9:	e8 32 70 ee ff       	call   0x1800130f0
   18012c0be:	48 8d 0d 7b 23 4a 00 	lea    rcx,[rip+0x4a237b]        # 0x1805ce440
   18012c0c5:	e8 fa ba 45 00       	call   0x180587bc4
   18012c0ca:	90                   	nop
   18012c0cb:	48 8d 0d 6e b2 16 01 	lea    rcx,[rip+0x116b26e]        # 0x181297340
   18012c0d2:	e8 2d bd 45 00       	call   0x180587e04
   18012c0d7:	48 8d 0d 1a 29 7e 00 	lea    rcx,[rip+0x7e291a]        # 0x18090e9f8
   18012c0de:	e8 bd 72 ee ff       	call   0x1800133a0
   18012c0e3:	48 85 c0             	test   rax,rax
   18012c0e6:	74 0e                	je     0x18012c0f6
   18012c0e8:	48 8b c8             	mov    rcx,rax
   18012c0eb:	ff 15 3f 3c 4b 00    	call   QWORD PTR [rip+0x4b3c3f]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   18012c0f1:	48 8b d8             	mov    rbx,rax
   18012c0f4:	eb 03                	jmp    0x18012c0f9
   18012c0f6:	49 8b dc             	mov    rbx,r12
   18012c0f9:	48 8d 0d 48 b2 16 01 	lea    rcx,[rip+0x116b248]        # 0x181297348
   18012c100:	e8 4b 4c ee ff       	call   0x180010d50
   18012c105:	48 8b c8             	mov    rcx,rax
   18012c108:	e8 83 4b ee ff       	call   0x180010c90
   18012c10d:	8b c8                	mov    ecx,eax
   18012c10f:	ff 15 73 3b 4b 00    	call   QWORD PTR [rip+0x4b3b73]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   18012c115:	8b d0                	mov    edx,eax
   18012c117:	4c 8b c3             	mov    r8,rbx
   18012c11a:	48 8d 4d b0          	lea    rcx,[rbp-0x50]
   18012c11e:	ff 15 5c 3b 4b 00    	call   QWORD PTR [rip+0x4b3b5c]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   18012c124:	48 8d 0d 1d b2 16 01 	lea    rcx,[rip+0x116b21d]        # 0x181297348
   18012c12b:	e8 20 4c ee ff       	call   0x180010d50
   18012c130:	48 8b c8             	mov    rcx,rax
   18012c133:	e8 58 4b ee ff       	call   0x180010c90
   18012c138:	8b c8                	mov    ecx,eax
   18012c13a:	45 33 c0             	xor    r8d,r8d
   18012c13d:	48 8d 55 b0          	lea    rdx,[rbp-0x50]
   18012c141:	ff 15 49 3e 4b 00    	call   QWORD PTR [rip+0x4b3e49]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   18012c147:	48 8d 0d fa b1 16 01 	lea    rcx,[rip+0x116b1fa]        # 0x181297348
   18012c14e:	e8 fd 4b ee ff       	call   0x180010d50
   18012c153:	48 8b c8             	mov    rcx,rax
   18012c156:	e8 35 4b ee ff       	call   0x180010c90
   18012c15b:	8b c8                	mov    ecx,eax
   18012c15d:	ff 15 25 3b 4b 00    	call   QWORD PTR [rip+0x4b3b25]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   18012c163:	8b d0                	mov    edx,eax
   18012c165:	4c 8b c3             	mov    r8,rbx
   18012c168:	48 8d 0d e9 b1 16 01 	lea    rcx,[rip+0x116b1e9]        # 0x181297358
   18012c16f:	ff 15 1b 3b 4b 00    	call   QWORD PTR [rip+0x4b3b1b]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   18012c175:	8b 04 3e             	mov    eax,DWORD PTR [rsi+rdi*1]
   18012c178:	39 05 ea b1 16 01    	cmp    DWORD PTR [rip+0x116b1ea],eax        # 0x181297368
   18012c17e:	7e 41                	jle    0x18012c1c1
   18012c180:	48 8d 0d e1 b1 16 01 	lea    rcx,[rip+0x116b1e1]        # 0x181297368
   18012c187:	e8 d8 bc 45 00       	call   0x180587e64
   18012c18c:	83 3d d5 b1 16 01 ff 	cmp    DWORD PTR [rip+0x116b1d5],0xffffffff        # 0x181297368
   18012c193:	75 2c                	jne    0x18012c1c1
   18012c195:	48 8d 15 a4 40 50 00 	lea    rdx,[rip+0x5040a4]        # 0x180630240 ; 'g_tDiffuseGIWeightSource'
   18012c19c:	48 8d 0d cd b1 16 01 	lea    rcx,[rip+0x116b1cd]        # 0x181297370
   18012c1a3:	e8 48 6f ee ff       	call   0x1800130f0
   18012c1a8:	48 8d 0d 61 22 4a 00 	lea    rcx,[rip+0x4a2261]        # 0x1805ce410
   18012c1af:	e8 10 ba 45 00       	call   0x180587bc4
   18012c1b4:	90                   	nop
   18012c1b5:	48 8d 0d ac b1 16 01 	lea    rcx,[rip+0x116b1ac]        # 0x181297368
   18012c1bc:	e8 43 bc 45 00       	call   0x180587e04
   18012c1c1:	48 8d 4d c0          	lea    rcx,[rbp-0x40]
   18012c1c5:	e8 d6 71 ee ff       	call   0x1800133a0
   18012c1ca:	48 85 c0             	test   rax,rax
   18012c1cd:	74 0e                	je     0x18012c1dd
   18012c1cf:	48 8b c8             	mov    rcx,rax
   18012c1d2:	ff 15 58 3b 4b 00    	call   QWORD PTR [rip+0x4b3b58]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   18012c1d8:	48 8b d8             	mov    rbx,rax
   18012c1db:	eb 03                	jmp    0x18012c1e0
   18012c1dd:	49 8b dc             	mov    rbx,r12
   18012c1e0:	48 8d 0d 89 b1 16 01 	lea    rcx,[rip+0x116b189]        # 0x181297370
   18012c1e7:	e8 64 4b ee ff       	call   0x180010d50
   18012c1ec:	48 8b c8             	mov    rcx,rax
   18012c1ef:	e8 9c 4a ee ff       	call   0x180010c90
   18012c1f4:	8b c8                	mov    ecx,eax
   18012c1f6:	ff 15 8c 3a 4b 00    	call   QWORD PTR [rip+0x4b3a8c]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   18012c1fc:	8b d0                	mov    edx,eax
   18012c1fe:	4c 8b c3             	mov    r8,rbx
   18012c201:	48 8d 8d e0 01 00 00 	lea    rcx,[rbp+0x1e0]
   18012c208:	ff 15 72 3a 4b 00    	call   QWORD PTR [rip+0x4b3a72]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   18012c20e:	48 8d 0d 5b b1 16 01 	lea    rcx,[rip+0x116b15b]        # 0x181297370
   18012c215:	e8 36 4b ee ff       	call   0x180010d50
   18012c21a:	48 8b c8             	mov    rcx,rax
   18012c21d:	e8 6e 4a ee ff       	call   0x180010c90
   18012c222:	8b c8                	mov    ecx,eax
   18012c224:	45 33 c0             	xor    r8d,r8d
   18012c227:	48 8d 95 e0 01 00 00 	lea    rdx,[rbp+0x1e0]
   18012c22e:	ff 15 5c 3d 4b 00    	call   QWORD PTR [rip+0x4b3d5c]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   18012c234:	48 8d 0d 35 b1 16 01 	lea    rcx,[rip+0x116b135]        # 0x181297370
   18012c23b:	e8 10 4b ee ff       	call   0x180010d50
   18012c240:	48 8b c8             	mov    rcx,rax
   18012c243:	e8 48 4a ee ff       	call   0x180010c90
   18012c248:	8b c8                	mov    ecx,eax
   18012c24a:	ff 15 38 3a 4b 00    	call   QWORD PTR [rip+0x4b3a38]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   18012c250:	8b d0                	mov    edx,eax
   18012c252:	4c 8b c3             	mov    r8,rbx
   18012c255:	48 8d 0d 24 b1 16 01 	lea    rcx,[rip+0x116b124]        # 0x181297380
   18012c25c:	ff 15 2e 3a 4b 00    	call   QWORD PTR [rip+0x4b3a2e]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   18012c262:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012c265:	48 81 c1 58 02 00 00 	add    rcx,0x258
   18012c26c:	e8 2f 71 ee ff       	call   0x1800133a0
   18012c271:	48 8b c8             	mov    rcx,rax
   18012c274:	ff 15 56 3a 4b 00    	call   QWORD PTR [rip+0x4b3a56]        # 0x1805dfcd0 ; ?transitionFromConsumeToUAV@NativeBufferUtilDX12@d3d@@SAXPEAVNativeBuffer@2@@Z
   18012c27a:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012c27d:	48 81 c1 60 02 00 00 	add    rcx,0x260
   18012c284:	e8 17 71 ee ff       	call   0x1800133a0
   18012c289:	48 8b c8             	mov    rcx,rax
   18012c28c:	ff 15 3e 3a 4b 00    	call   QWORD PTR [rip+0x4b3a3e]        # 0x1805dfcd0 ; ?transitionFromConsumeToUAV@NativeBufferUtilDX12@d3d@@SAXPEAVNativeBuffer@2@@Z
   18012c292:	8b 04 3e             	mov    eax,DWORD PTR [rsi+rdi*1]
   18012c295:	39 05 f5 b0 16 01    	cmp    DWORD PTR [rip+0x116b0f5],eax        # 0x181297390
   18012c29b:	7e 41                	jle    0x18012c2de
   18012c29d:	48 8d 0d ec b0 16 01 	lea    rcx,[rip+0x116b0ec]        # 0x181297390
   18012c2a4:	e8 bb bb 45 00       	call   0x180587e64
   18012c2a9:	83 3d e0 b0 16 01 ff 	cmp    DWORD PTR [rip+0x116b0e0],0xffffffff        # 0x181297390
   18012c2b0:	75 2c                	jne    0x18012c2de
   18012c2b2:	48 8d 15 df 3f 50 00 	lea    rdx,[rip+0x503fdf]        # 0x180630298 ; 'g_rwbDGICompactionCountBuffer'
   18012c2b9:	48 8d 0d d8 b0 16 01 	lea    rcx,[rip+0x116b0d8]        # 0x181297398
   18012c2c0:	e8 ab 6d ee ff       	call   0x180013070
   18012c2c5:	48 8d 0d 14 21 4a 00 	lea    rcx,[rip+0x4a2114]        # 0x1805ce3e0
   18012c2cc:	e8 f3 b8 45 00       	call   0x180587bc4
   18012c2d1:	90                   	nop
   18012c2d2:	48 8d 0d b7 b0 16 01 	lea    rcx,[rip+0x116b0b7]        # 0x181297390
   18012c2d9:	e8 26 bb 45 00       	call   0x180587e04
   18012c2de:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012c2e1:	48 81 c1 58 02 00 00 	add    rcx,0x258
   18012c2e8:	e8 b3 70 ee ff       	call   0x1800133a0
   18012c2ed:	48 8b d8             	mov    rbx,rax
   18012c2f0:	48 8d 0d a1 b0 16 01 	lea    rcx,[rip+0x116b0a1]        # 0x181297398
   18012c2f7:	e8 54 4a ee ff       	call   0x180010d50
   18012c2fc:	48 8b c8             	mov    rcx,rax
   18012c2ff:	e8 8c 49 ee ff       	call   0x180010c90
   18012c304:	8b c8                	mov    ecx,eax
   18012c306:	ff 15 7c 39 4b 00    	call   QWORD PTR [rip+0x4b397c]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   18012c30c:	8b d0                	mov    edx,eax
   18012c30e:	4c 8b c3             	mov    r8,rbx
   18012c311:	48 8d 8d c8 01 00 00 	lea    rcx,[rbp+0x1c8]
   18012c318:	ff 15 7a 39 4b 00    	call   QWORD PTR [rip+0x4b397a]        # 0x1805dfc98 ; ??0ShaderBuffer@d3d@@QEAA@W4ResourceType@1@PEAVNativeBuffer@1@@Z
   18012c31e:	48 8d 0d 73 b0 16 01 	lea    rcx,[rip+0x116b073]        # 0x181297398
   18012c325:	e8 26 4a ee ff       	call   0x180010d50
   18012c32a:	48 8b c8             	mov    rcx,rax
   18012c32d:	e8 5e 49 ee ff       	call   0x180010c90
   18012c332:	8b c8                	mov    ecx,eax
   18012c334:	45 33 c0             	xor    r8d,r8d
   18012c337:	48 8d 95 c8 01 00 00 	lea    rdx,[rbp+0x1c8]
   18012c33e:	ff 15 4c 3c 4b 00    	call   QWORD PTR [rip+0x4b3c4c]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   18012c344:	48 8d 0d 4d b0 16 01 	lea    rcx,[rip+0x116b04d]        # 0x181297398
   18012c34b:	e8 00 4a ee ff       	call   0x180010d50
   18012c350:	48 8b c8             	mov    rcx,rax
   18012c353:	e8 38 49 ee ff       	call   0x180010c90
   18012c358:	8b c8                	mov    ecx,eax
   18012c35a:	ff 15 28 39 4b 00    	call   QWORD PTR [rip+0x4b3928]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   18012c360:	8b d0                	mov    edx,eax
   18012c362:	4c 8b c3             	mov    r8,rbx
   18012c365:	48 8d 0d 3c b0 16 01 	lea    rcx,[rip+0x116b03c]        # 0x1812973a8
   18012c36c:	ff 15 2e 39 4b 00    	call   QWORD PTR [rip+0x4b392e]        # 0x1805dfca0 ; ?setBuffer@ShaderBuffer@d3d@@QEAAXW4ResourceType@2@PEAVNativeBuffer@2@@Z
   18012c372:	8b 04 3e             	mov    eax,DWORD PTR [rsi+rdi*1]
   18012c375:	39 05 35 b0 16 01    	cmp    DWORD PTR [rip+0x116b035],eax        # 0x1812973b0
   18012c37b:	7e 41                	jle    0x18012c3be
   18012c37d:	48 8d 0d 2c b0 16 01 	lea    rcx,[rip+0x116b02c]        # 0x1812973b0
   18012c384:	e8 db ba 45 00       	call   0x180587e64
   18012c389:	83 3d 20 b0 16 01 ff 	cmp    DWORD PTR [rip+0x116b020],0xffffffff        # 0x1812973b0
   18012c390:	75 2c                	jne    0x18012c3be
   18012c392:	48 8d 15 df 3e 50 00 	lea    rdx,[rip+0x503edf]        # 0x180630278 ; 'g_rwbDGICompactionIndexBuffer'
   18012c399:	48 8d 0d 18 b0 16 01 	lea    rcx,[rip+0x116b018]        # 0x1812973b8
   18012c3a0:	e8 cb 6c ee ff       	call   0x180013070
   18012c3a5:	48 8d 0d 04 20 4a 00 	lea    rcx,[rip+0x4a2004]        # 0x1805ce3b0
   18012c3ac:	e8 13 b8 45 00       	call   0x180587bc4
   18012c3b1:	90                   	nop
   18012c3b2:	48 8d 0d f7 af 16 01 	lea    rcx,[rip+0x116aff7]        # 0x1812973b0
   18012c3b9:	e8 46 ba 45 00       	call   0x180587e04
   18012c3be:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012c3c1:	48 81 c1 60 02 00 00 	add    rcx,0x260
   18012c3c8:	e8 d3 6f ee ff       	call   0x1800133a0
   18012c3cd:	48 8b d8             	mov    rbx,rax
   18012c3d0:	48 8d 0d e1 af 16 01 	lea    rcx,[rip+0x116afe1]        # 0x1812973b8
   18012c3d7:	e8 74 49 ee ff       	call   0x180010d50
   18012c3dc:	48 8b c8             	mov    rcx,rax
   18012c3df:	e8 ac 48 ee ff       	call   0x180010c90
   18012c3e4:	8b c8                	mov    ecx,eax
   18012c3e6:	ff 15 9c 38 4b 00    	call   QWORD PTR [rip+0x4b389c]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   18012c3ec:	8b d0                	mov    edx,eax
   18012c3ee:	4c 8b c3             	mov    r8,rbx
   18012c3f1:	48 8d 8d 48 01 00 00 	lea    rcx,[rbp+0x148]
   18012c3f8:	ff 15 9a 38 4b 00    	call   QWORD PTR [rip+0x4b389a]        # 0x1805dfc98 ; ??0ShaderBuffer@d3d@@QEAA@W4ResourceType@1@PEAVNativeBuffer@1@@Z
   18012c3fe:	48 8d 0d b3 af 16 01 	lea    rcx,[rip+0x116afb3]        # 0x1812973b8
   18012c405:	e8 46 49 ee ff       	call   0x180010d50
   18012c40a:	48 8b c8             	mov    rcx,rax
   18012c40d:	e8 7e 48 ee ff       	call   0x180010c90
   18012c412:	8b c8                	mov    ecx,eax
   18012c414:	45 33 c0             	xor    r8d,r8d
   18012c417:	48 8d 95 48 01 00 00 	lea    rdx,[rbp+0x148]
   18012c41e:	ff 15 6c 3b 4b 00    	call   QWORD PTR [rip+0x4b3b6c]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   18012c424:	48 8d 0d 8d af 16 01 	lea    rcx,[rip+0x116af8d]        # 0x1812973b8
   18012c42b:	e8 20 49 ee ff       	call   0x180010d50
   18012c430:	48 8b c8             	mov    rcx,rax
   18012c433:	e8 58 48 ee ff       	call   0x180010c90
   18012c438:	8b c8                	mov    ecx,eax
   18012c43a:	ff 15 48 38 4b 00    	call   QWORD PTR [rip+0x4b3848]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   18012c440:	8b d0                	mov    edx,eax
   18012c442:	4c 8b c3             	mov    r8,rbx
   18012c445:	48 8d 0d 7c af 16 01 	lea    rcx,[rip+0x116af7c]        # 0x1812973c8
   18012c44c:	ff 15 4e 38 4b 00    	call   QWORD PTR [rip+0x4b384e]        # 0x1805dfca0 ; ?setBuffer@ShaderBuffer@d3d@@QEAAXW4ResourceType@2@PEAVNativeBuffer@2@@Z
   18012c452:	48 8b 0d 8f 14 16 01 	mov    rcx,QWORD PTR [rip+0x116148f]        # 0x18128d8e8
   18012c459:	48 85 c9             	test   rcx,rcx
   18012c45c:	75 3f                	jne    0x18012c49d
   18012c45e:	48 8d 0d 6b 3e 50 00 	lea    rcx,[rip+0x503e6b]        # 0x1806302d0 ; 'rt_deferred_diffusegi.rfx'
   18012c465:	e8 b6 6a 0a 00       	call   0x1801d2f20
   18012c46a:	48 8b c8             	mov    rcx,rax
   18012c46d:	48 8d 15 44 3e 50 00 	lea    rdx,[rip+0x503e44]        # 0x1806302b8 ; 'clear_compaction_count'
   18012c474:	e8 67 68 0a 00       	call   0x1801d2ce0
   18012c479:	48 8b c8             	mov    rcx,rax
   18012c47c:	33 d2                	xor    edx,edx
   18012c47e:	e8 6d 0f 0b 00       	call   0x1801dd3f0
   18012c483:	48 89 05 5e 14 16 01 	mov    QWORD PTR [rip+0x116145e],rax        # 0x18128d8e8
   18012c48a:	48 8d 0d 57 14 16 01 	lea    rcx,[rip+0x1161457]        # 0x18128d8e8
   18012c491:	e8 6a c2 0a 00       	call   0x1801d8700
   18012c496:	48 8b 0d 4b 14 16 01 	mov    rcx,QWORD PTR [rip+0x116144b]        # 0x18128d8e8
   18012c49d:	e8 ae f2 0a 00       	call   0x1801db750
   18012c4a2:	c7 45 d0 01 00 00 00 	mov    DWORD PTR [rbp-0x30],0x1
   18012c4a9:	c7 45 d4 01 00 00 00 	mov    DWORD PTR [rbp-0x2c],0x1
   18012c4b0:	f2 0f 10 45 d0       	movsd  xmm0,QWORD PTR [rbp-0x30]
   18012c4b5:	f2 0f 11 85 d0 01 00 	movsd  QWORD PTR [rbp+0x1d0],xmm0
   18012c4bc:	00 
   18012c4bd:	c7 85 d8 01 00 00 01 	mov    DWORD PTR [rbp+0x1d8],0x1
   18012c4c4:	00 00 00 
   18012c4c7:	48 8d 95 d0 01 00 00 	lea    rdx,[rbp+0x1d0]
   18012c4ce:	49 8b cd             	mov    rcx,r13
   18012c4d1:	ff 15 e9 37 4b 00    	call   QWORD PTR [rip+0x4b37e9]        # 0x1805dfcc0 ; ?dispatch@DeviceUtil@d3d@@SAXAEAVDeviceState@2@V?$Vector3Template@H@m@@@Z
   18012c4d7:	8b 85 f0 03 00 00    	mov    eax,DWORD PTR [rbp+0x3f0]
   18012c4dd:	41 8b dc             	mov    ebx,r12d
   18012c4e0:	85 c0                	test   eax,eax
   18012c4e2:	0f 95 c3             	setne  bl
   18012c4e5:	41 bc 02 00 00 00    	mov    r12d,0x2
   18012c4eb:	41 8b d4             	mov    edx,r12d
   18012c4ee:	41 8d 4c 24 ff       	lea    ecx,[r12-0x1]
   18012c4f3:	ff 15 a7 2f 4b 00    	call   QWORD PTR [rip+0x4b2fa7]        # 0x1805df4a0 ; ?beginPipelineSetup@DeviceUtilRaytracing@d3d@@SAXHH@Z
   18012c4f9:	48 8b 0d e0 13 16 01 	mov    rcx,QWORD PTR [rip+0x11613e0]        # 0x18128d8e0
   18012c500:	48 85 c9             	test   rcx,rcx
   18012c503:	75 35                	jne    0x18012c53a
   18012c505:	48 8d 0d 04 3e 50 00 	lea    rcx,[rip+0x503e04]        # 0x180630310 ; 'rt_diffusegi.rfx'
   18012c50c:	e8 0f 6a 0a 00       	call   0x1801d2f20
   18012c511:	48 8b c8             	mov    rcx,rax
   18012c514:	48 8d 15 d5 3d 50 00 	lea    rdx,[rip+0x503dd5]        # 0x1806302f0 ; 'rt_diffuseGIDeferredShading'
   18012c51b:	e8 c0 67 0a 00       	call   0x1801d2ce0
   18012c520:	48 89 05 b9 13 16 01 	mov    QWORD PTR [rip+0x11613b9],rax        # 0x18128d8e0
   18012c527:	48 8d 0d b2 13 16 01 	lea    rcx,[rip+0x11613b2]        # 0x18128d8e0
   18012c52e:	e8 cd c1 0a 00       	call   0x1801d8700
   18012c533:	48 8b 0d a6 13 16 01 	mov    rcx,QWORD PTR [rip+0x11613a6]        # 0x18128d8e0
   18012c53a:	8b d3                	mov    edx,ebx
   18012c53c:	e8 0f 0f 0b 00       	call   0x1801dd450
   18012c541:	48 8b c8             	mov    rcx,rax
   18012c544:	49 8b d5             	mov    rdx,r13
   18012c547:	e8 c4 f2 0a 00       	call   0x1801db810
   18012c54c:	48 8d 0d e5 3d 50 00 	lea    rcx,[rip+0x503de5]        # 0x180630338 ; 'rayGenMain'
   18012c553:	ff 15 4f 2f 4b 00    	call   QWORD PTR [rip+0x4b2f4f]        # 0x1805df4a8 ; ?setRayGeneration@DeviceUtilRaytracing@d3d@@SAXPEBD@Z
   18012c559:	48 8d 15 c8 3d 50 00 	lea    rdx,[rip+0x503dc8]        # 0x180630328 ; 'missMain'
   18012c560:	33 c9                	xor    ecx,ecx
   18012c562:	ff 15 48 2f 4b 00    	call   QWORD PTR [rip+0x4b2f48]        # 0x1805df4b0 ; ?setMiss@DeviceUtilRaytracing@d3d@@SAXHPEBD@Z
   18012c568:	4c 89 74 24 20       	mov    QWORD PTR [rsp+0x20],r14
   18012c56d:	45 33 c9             	xor    r9d,r9d
   18012c570:	45 33 c0             	xor    r8d,r8d
   18012c573:	33 d2                	xor    edx,edx
   18012c575:	33 c9                	xor    ecx,ecx
   18012c577:	ff 15 3b 2f 4b 00    	call   QWORD PTR [rip+0x4b2f3b]        # 0x1805df4b8 ; ?setHitGroup@DeviceUtilRaytracing@d3d@@SAXHHPEBD00@Z
   18012c57d:	4c 89 74 24 20       	mov    QWORD PTR [rsp+0x20],r14
   18012c582:	4c 8d 0d bf 3d 50 00 	lea    r9,[rip+0x503dbf]        # 0x180630348 ; 'anyHitAlphaTest'
   18012c589:	45 33 c0             	xor    r8d,r8d
   18012c58c:	41 8d 50 01          	lea    edx,[r8+0x1]
   18012c590:	33 c9                	xor    ecx,ecx
   18012c592:	ff 15 20 2f 4b 00    	call   QWORD PTR [rip+0x4b2f20]        # 0x1805df4b8 ; ?setHitGroup@DeviceUtilRaytracing@d3d@@SAXHHPEBD00@Z
   18012c598:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012c59b:	48 81 c1 20 02 00 00 	add    rcx,0x220
   18012c5a2:	e8 f9 6d ee ff       	call   0x1800133a0
   18012c5a7:	48 8b c8             	mov    rcx,rax
   18012c5aa:	e8 f1 bb f7 ff       	call   0x1800a81a0
   18012c5af:	8b d8                	mov    ebx,eax
   18012c5b1:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012c5b4:	48 81 c1 20 02 00 00 	add    rcx,0x220
   18012c5bb:	e8 e0 6d ee ff       	call   0x1800133a0
   18012c5c0:	48 8b c8             	mov    rcx,rax
   18012c5c3:	e8 c8 bb f7 ff       	call   0x1800a8190
   18012c5c8:	4c 8b c8             	mov    r9,rax
   18012c5cb:	89 5c 24 20          	mov    DWORD PTR [rsp+0x20],ebx
   18012c5cf:	45 8b c4             	mov    r8d,r12d
   18012c5d2:	ba 01 00 00 00       	mov    edx,0x1
   18012c5d7:	8b ca                	mov    ecx,edx
   18012c5d9:	ff 15 e1 2e 4b 00    	call   QWORD PTR [rip+0x4b2ee1]        # 0x1805df4c0 ; ?endPipelineSetup@DeviceUtilRaytracing@d3d@@SAXHHHPEBURaytracingGeometryBinding@2@H@Z
   18012c5df:	8b 15 a7 1e 7e 00    	mov    edx,DWORD PTR [rip+0x7e1ea7]        # 0x18090e48c
   18012c5e5:	8b 0d 9d 1e 7e 00    	mov    ecx,DWORD PTR [rip+0x7e1e9d]        # 0x18090e488
   18012c5eb:	ff 15 d7 2e 4b 00    	call   QWORD PTR [rip+0x4b2ed7]        # 0x1805df4c8 ; ?raytrace@DeviceUtilRaytracing@d3d@@SAXHH@Z
   18012c5f1:	8b 8d f0 03 00 00    	mov    ecx,DWORD PTR [rbp+0x3f0]
   18012c5f7:	8d 41 01             	lea    eax,[rcx+0x1]
   18012c5fa:	3b 85 00 04 00 00    	cmp    eax,DWORD PTR [rbp+0x400]
   18012c600:	41 0f 94 c6          	sete   r14b
   18012c604:	85 c9                	test   ecx,ecx
   18012c606:	41 0f 94 c4          	sete   r12b
   18012c60a:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012c60d:	48 81 c1 40 02 00 00 	add    rcx,0x240
   18012c614:	e8 87 6d ee ff       	call   0x1800133a0
   18012c619:	48 8b c8             	mov    rcx,rax
   18012c61c:	e8 1f 73 f7 ff       	call   0x1800a3940
   18012c621:	48 8d 0d d0 23 7e 00 	lea    rcx,[rip+0x7e23d0]        # 0x18090e9f8
   18012c628:	e8 73 6d ee ff       	call   0x1800133a0
   18012c62d:	48 8b c8             	mov    rcx,rax
   18012c630:	ff 15 fa 36 4b 00    	call   QWORD PTR [rip+0x4b36fa]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   18012c636:	48 8b c8             	mov    rcx,rax
   18012c639:	ff 15 31 39 4b 00    	call   QWORD PTR [rip+0x4b3931]        # 0x1805dff70 ; ?prepareAsUAV@NativeTexture@d3d@@QEAAXXZ
   18012c63f:	8b 04 3e             	mov    eax,DWORD PTR [rsi+rdi*1]
   18012c642:	39 05 88 ad 16 01    	cmp    DWORD PTR [rip+0x116ad88],eax        # 0x1812973d0
   18012c648:	7e 41                	jle    0x18012c68b
   18012c64a:	48 8d 0d 7f ad 16 01 	lea    rcx,[rip+0x116ad7f]        # 0x1812973d0
   18012c651:	e8 0e b8 45 00       	call   0x180587e64
   18012c656:	83 3d 73 ad 16 01 ff 	cmp    DWORD PTR [rip+0x116ad73],0xffffffff        # 0x1812973d0
   18012c65d:	75 2c                	jne    0x18012c68b
   18012c65f:	48 8d 15 22 3d 50 00 	lea    rdx,[rip+0x503d22]        # 0x180630388 ; 'g_rwtDiffuseGIColorTarget'
   18012c666:	48 8d 0d 6b ad 16 01 	lea    rcx,[rip+0x116ad6b]        # 0x1812973d8
   18012c66d:	e8 7e 6a ee ff       	call   0x1800130f0
   18012c672:	48 8d 0d 07 1d 4a 00 	lea    rcx,[rip+0x4a1d07]        # 0x1805ce380
   18012c679:	e8 46 b5 45 00       	call   0x180587bc4
   18012c67e:	90                   	nop
   18012c67f:	48 8d 0d 4a ad 16 01 	lea    rcx,[rip+0x116ad4a]        # 0x1812973d0
   18012c686:	e8 79 b7 45 00       	call   0x180587e04
   18012c68b:	48 8d 0d 66 23 7e 00 	lea    rcx,[rip+0x7e2366]        # 0x18090e9f8
   18012c692:	e8 09 6d ee ff       	call   0x1800133a0
   18012c697:	48 85 c0             	test   rax,rax
   18012c69a:	74 0e                	je     0x18012c6aa
   18012c69c:	48 8b c8             	mov    rcx,rax
   18012c69f:	ff 15 8b 36 4b 00    	call   QWORD PTR [rip+0x4b368b]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   18012c6a5:	48 8b d8             	mov    rbx,rax
   18012c6a8:	eb 02                	jmp    0x18012c6ac
   18012c6aa:	33 db                	xor    ebx,ebx
   18012c6ac:	48 8d 0d 25 ad 16 01 	lea    rcx,[rip+0x116ad25]        # 0x1812973d8
   18012c6b3:	e8 98 46 ee ff       	call   0x180010d50
   18012c6b8:	48 8b c8             	mov    rcx,rax
   18012c6bb:	e8 d0 45 ee ff       	call   0x180010c90
   18012c6c0:	8b c8                	mov    ecx,eax
   18012c6c2:	ff 15 c0 35 4b 00    	call   QWORD PTR [rip+0x4b35c0]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   18012c6c8:	8b d0                	mov    edx,eax
   18012c6ca:	4c 8b c3             	mov    r8,rbx
   18012c6cd:	48 8d 8d 50 01 00 00 	lea    rcx,[rbp+0x150]
   18012c6d4:	ff 15 a6 35 4b 00    	call   QWORD PTR [rip+0x4b35a6]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   18012c6da:	48 8d 0d f7 ac 16 01 	lea    rcx,[rip+0x116acf7]        # 0x1812973d8
   18012c6e1:	e8 6a 46 ee ff       	call   0x180010d50
   18012c6e6:	48 8b c8             	mov    rcx,rax
   18012c6e9:	e8 a2 45 ee ff       	call   0x180010c90
   18012c6ee:	8b c8                	mov    ecx,eax
   18012c6f0:	45 33 c0             	xor    r8d,r8d
   18012c6f3:	48 8d 95 50 01 00 00 	lea    rdx,[rbp+0x150]
   18012c6fa:	ff 15 90 38 4b 00    	call   QWORD PTR [rip+0x4b3890]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   18012c700:	48 8d 0d d1 ac 16 01 	lea    rcx,[rip+0x116acd1]        # 0x1812973d8
   18012c707:	e8 44 46 ee ff       	call   0x180010d50
   18012c70c:	48 8b c8             	mov    rcx,rax
   18012c70f:	e8 7c 45 ee ff       	call   0x180010c90
   18012c714:	8b c8                	mov    ecx,eax
   18012c716:	ff 15 6c 35 4b 00    	call   QWORD PTR [rip+0x4b356c]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   18012c71c:	8b d0                	mov    edx,eax
   18012c71e:	4c 8b c3             	mov    r8,rbx
   18012c721:	48 8d 0d c0 ac 16 01 	lea    rcx,[rip+0x116acc0]        # 0x1812973e8
   18012c728:	ff 15 62 35 4b 00    	call   QWORD PTR [rip+0x4b3562]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   18012c72e:	45 84 e4             	test   r12b,r12b
   18012c731:	0f 85 f9 01 00 00    	jne    0x18012c930
   18012c737:	45 84 f6             	test   r14b,r14b
   18012c73a:	0f 85 f0 01 00 00    	jne    0x18012c930
   18012c740:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012c743:	48 81 c1 58 02 00 00 	add    rcx,0x258
   18012c74a:	e8 51 6c ee ff       	call   0x1800133a0
   18012c74f:	48 8b c8             	mov    rcx,rax
   18012c752:	ff 15 98 35 4b 00    	call   QWORD PTR [rip+0x4b3598]        # 0x1805dfcf0 ; ?transitionFromUAVToConsume@NativeBufferUtilDX12@d3d@@SAXPEAVNativeBuffer@2@@Z
   18012c758:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012c75b:	48 81 c1 60 02 00 00 	add    rcx,0x260
   18012c762:	e8 39 6c ee ff       	call   0x1800133a0
   18012c767:	48 8b c8             	mov    rcx,rax
   18012c76a:	ff 15 80 35 4b 00    	call   QWORD PTR [rip+0x4b3580]        # 0x1805dfcf0 ; ?transitionFromUAVToConsume@NativeBufferUtilDX12@d3d@@SAXPEAVNativeBuffer@2@@Z
   18012c770:	8b 04 3e             	mov    eax,DWORD PTR [rsi+rdi*1]
   18012c773:	39 05 7f ac 16 01    	cmp    DWORD PTR [rip+0x116ac7f],eax        # 0x1812973f8
   18012c779:	7e 41                	jle    0x18012c7bc
   18012c77b:	48 8d 0d 76 ac 16 01 	lea    rcx,[rip+0x116ac76]        # 0x1812973f8
   18012c782:	e8 dd b6 45 00       	call   0x180587e64
   18012c787:	83 3d 6a ac 16 01 ff 	cmp    DWORD PTR [rip+0x116ac6a],0xffffffff        # 0x1812973f8
   18012c78e:	75 2c                	jne    0x18012c7bc
   18012c790:	48 8d 15 d1 3b 50 00 	lea    rdx,[rip+0x503bd1]        # 0x180630368 ; 'g_bDGICompactionCountBuffer'
   18012c797:	48 8d 0d 62 ac 16 01 	lea    rcx,[rip+0x116ac62]        # 0x181297400
   18012c79e:	e8 cd 68 ee ff       	call   0x180013070
   18012c7a3:	48 8d 0d a6 1b 4a 00 	lea    rcx,[rip+0x4a1ba6]        # 0x1805ce350
   18012c7aa:	e8 15 b4 45 00       	call   0x180587bc4
   18012c7af:	90                   	nop
   18012c7b0:	48 8d 0d 41 ac 16 01 	lea    rcx,[rip+0x116ac41]        # 0x1812973f8
   18012c7b7:	e8 48 b6 45 00       	call   0x180587e04
   18012c7bc:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012c7bf:	48 81 c1 58 02 00 00 	add    rcx,0x258
   18012c7c6:	e8 d5 6b ee ff       	call   0x1800133a0
   18012c7cb:	48 8b d8             	mov    rbx,rax
   18012c7ce:	48 8d 0d 2b ac 16 01 	lea    rcx,[rip+0x116ac2b]        # 0x181297400
   18012c7d5:	e8 76 45 ee ff       	call   0x180010d50
   18012c7da:	48 8b c8             	mov    rcx,rax
   18012c7dd:	e8 ae 44 ee ff       	call   0x180010c90
   18012c7e2:	8b c8                	mov    ecx,eax
   18012c7e4:	ff 15 9e 34 4b 00    	call   QWORD PTR [rip+0x4b349e]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   18012c7ea:	8b d0                	mov    edx,eax
   18012c7ec:	4c 8b c3             	mov    r8,rbx
   18012c7ef:	48 8d 8d c0 01 00 00 	lea    rcx,[rbp+0x1c0]
   18012c7f6:	ff 15 9c 34 4b 00    	call   QWORD PTR [rip+0x4b349c]        # 0x1805dfc98 ; ??0ShaderBuffer@d3d@@QEAA@W4ResourceType@1@PEAVNativeBuffer@1@@Z
   18012c7fc:	48 8d 0d fd ab 16 01 	lea    rcx,[rip+0x116abfd]        # 0x181297400
   18012c803:	e8 48 45 ee ff       	call   0x180010d50
   18012c808:	48 8b c8             	mov    rcx,rax
   18012c80b:	e8 80 44 ee ff       	call   0x180010c90
   18012c810:	8b c8                	mov    ecx,eax
   18012c812:	45 33 c0             	xor    r8d,r8d
   18012c815:	48 8d 95 c0 01 00 00 	lea    rdx,[rbp+0x1c0]
   18012c81c:	ff 15 6e 37 4b 00    	call   QWORD PTR [rip+0x4b376e]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   18012c822:	48 8d 0d d7 ab 16 01 	lea    rcx,[rip+0x116abd7]        # 0x181297400
   18012c829:	e8 22 45 ee ff       	call   0x180010d50
   18012c82e:	48 8b c8             	mov    rcx,rax
   18012c831:	e8 5a 44 ee ff       	call   0x180010c90
   18012c836:	8b c8                	mov    ecx,eax
   18012c838:	ff 15 4a 34 4b 00    	call   QWORD PTR [rip+0x4b344a]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   18012c83e:	8b d0                	mov    edx,eax
   18012c840:	4c 8b c3             	mov    r8,rbx
   18012c843:	48 8d 0d c6 ab 16 01 	lea    rcx,[rip+0x116abc6]        # 0x181297410
   18012c84a:	ff 15 50 34 4b 00    	call   QWORD PTR [rip+0x4b3450]        # 0x1805dfca0 ; ?setBuffer@ShaderBuffer@d3d@@QEAAXW4ResourceType@2@PEAVNativeBuffer@2@@Z
   18012c850:	8b 04 3e             	mov    eax,DWORD PTR [rsi+rdi*1]
   18012c853:	39 05 bf ab 16 01    	cmp    DWORD PTR [rip+0x116abbf],eax        # 0x181297418
   18012c859:	7e 41                	jle    0x18012c89c
   18012c85b:	48 8d 0d b6 ab 16 01 	lea    rcx,[rip+0x116abb6]        # 0x181297418
   18012c862:	e8 fd b5 45 00       	call   0x180587e64
   18012c867:	83 3d aa ab 16 01 ff 	cmp    DWORD PTR [rip+0x116abaa],0xffffffff        # 0x181297418
   18012c86e:	75 2c                	jne    0x18012c89c
   18012c870:	48 8d 15 51 3b 50 00 	lea    rdx,[rip+0x503b51]        # 0x1806303c8 ; 'g_bDGICompactionIndexBuffer'
   18012c877:	48 8d 0d a2 ab 16 01 	lea    rcx,[rip+0x116aba2]        # 0x181297420
   18012c87e:	e8 ed 67 ee ff       	call   0x180013070
   18012c883:	48 8d 0d 96 1a 4a 00 	lea    rcx,[rip+0x4a1a96]        # 0x1805ce320
   18012c88a:	e8 35 b3 45 00       	call   0x180587bc4
   18012c88f:	90                   	nop
   18012c890:	48 8d 0d 81 ab 16 01 	lea    rcx,[rip+0x116ab81]        # 0x181297418
   18012c897:	e8 68 b5 45 00       	call   0x180587e04
   18012c89c:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012c89f:	48 81 c1 60 02 00 00 	add    rcx,0x260
   18012c8a6:	e8 f5 6a ee ff       	call   0x1800133a0
   18012c8ab:	48 8b d8             	mov    rbx,rax
   18012c8ae:	48 8d 0d 6b ab 16 01 	lea    rcx,[rip+0x116ab6b]        # 0x181297420
   18012c8b5:	e8 96 44 ee ff       	call   0x180010d50
   18012c8ba:	48 8b c8             	mov    rcx,rax
   18012c8bd:	e8 ce 43 ee ff       	call   0x180010c90
   18012c8c2:	8b c8                	mov    ecx,eax
   18012c8c4:	ff 15 be 33 4b 00    	call   QWORD PTR [rip+0x4b33be]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   18012c8ca:	8b d0                	mov    edx,eax
   18012c8cc:	4c 8b c3             	mov    r8,rbx
   18012c8cf:	48 8d 8d 40 01 00 00 	lea    rcx,[rbp+0x140]
   18012c8d6:	ff 15 bc 33 4b 00    	call   QWORD PTR [rip+0x4b33bc]        # 0x1805dfc98 ; ??0ShaderBuffer@d3d@@QEAA@W4ResourceType@1@PEAVNativeBuffer@1@@Z
   18012c8dc:	48 8d 0d 3d ab 16 01 	lea    rcx,[rip+0x116ab3d]        # 0x181297420
   18012c8e3:	e8 68 44 ee ff       	call   0x180010d50
   18012c8e8:	48 8b c8             	mov    rcx,rax
   18012c8eb:	e8 a0 43 ee ff       	call   0x180010c90
   18012c8f0:	8b c8                	mov    ecx,eax
   18012c8f2:	45 33 c0             	xor    r8d,r8d
   18012c8f5:	48 8d 95 40 01 00 00 	lea    rdx,[rbp+0x140]
   18012c8fc:	ff 15 8e 36 4b 00    	call   QWORD PTR [rip+0x4b368e]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   18012c902:	48 8d 0d 17 ab 16 01 	lea    rcx,[rip+0x116ab17]        # 0x181297420
   18012c909:	e8 42 44 ee ff       	call   0x180010d50
   18012c90e:	48 8b c8             	mov    rcx,rax
   18012c911:	e8 7a 43 ee ff       	call   0x180010c90
   18012c916:	8b c8                	mov    ecx,eax
   18012c918:	ff 15 6a 33 4b 00    	call   QWORD PTR [rip+0x4b336a]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   18012c91e:	8b d0                	mov    edx,eax
   18012c920:	4c 8b c3             	mov    r8,rbx
   18012c923:	48 8d 0d 06 ab 16 01 	lea    rcx,[rip+0x116ab06]        # 0x181297430
   18012c92a:	ff 15 70 33 4b 00    	call   QWORD PTR [rip+0x4b3370]        # 0x1805dfca0 ; ?setBuffer@ShaderBuffer@d3d@@QEAAXW4ResourceType@2@PEAVNativeBuffer@2@@Z
   18012c930:	83 bd 00 04 00 00 01 	cmp    DWORD PTR [rbp+0x400],0x1
   18012c937:	0f 86 07 01 00 00    	jbe    0x18012ca44
   18012c93d:	48 8d 4d c0          	lea    rcx,[rbp-0x40]
   18012c941:	e8 5a 6a ee ff       	call   0x1800133a0
   18012c946:	48 8b c8             	mov    rcx,rax
   18012c949:	ff 15 e1 33 4b 00    	call   QWORD PTR [rip+0x4b33e1]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   18012c94f:	48 8b c8             	mov    rcx,rax
   18012c952:	ff 15 18 36 4b 00    	call   QWORD PTR [rip+0x4b3618]        # 0x1805dff70 ; ?prepareAsUAV@NativeTexture@d3d@@QEAAXXZ
   18012c958:	8b 04 3e             	mov    eax,DWORD PTR [rsi+rdi*1]
   18012c95b:	39 05 d7 aa 16 01    	cmp    DWORD PTR [rip+0x116aad7],eax        # 0x181297438
   18012c961:	7e 41                	jle    0x18012c9a4
   18012c963:	48 8d 0d ce aa 16 01 	lea    rcx,[rip+0x116aace]        # 0x181297438
   18012c96a:	e8 f5 b4 45 00       	call   0x180587e64
   18012c96f:	83 3d c2 aa 16 01 ff 	cmp    DWORD PTR [rip+0x116aac2],0xffffffff        # 0x181297438
   18012c976:	75 2c                	jne    0x18012c9a4
   18012c978:	48 8d 15 29 3a 50 00 	lea    rdx,[rip+0x503a29]        # 0x1806303a8 ; 'g_rwtDiffuseGIWeightTarget'
   18012c97f:	48 8d 0d ba aa 16 01 	lea    rcx,[rip+0x116aaba]        # 0x181297440
   18012c986:	e8 65 67 ee ff       	call   0x1800130f0
   18012c98b:	48 8d 0d 5e 19 4a 00 	lea    rcx,[rip+0x4a195e]        # 0x1805ce2f0
   18012c992:	e8 2d b2 45 00       	call   0x180587bc4
   18012c997:	90                   	nop
   18012c998:	48 8d 0d 99 aa 16 01 	lea    rcx,[rip+0x116aa99]        # 0x181297438
   18012c99f:	e8 60 b4 45 00       	call   0x180587e04
   18012c9a4:	48 8d 4d c0          	lea    rcx,[rbp-0x40]
   18012c9a8:	e8 f3 69 ee ff       	call   0x1800133a0
   18012c9ad:	48 85 c0             	test   rax,rax
   18012c9b0:	74 0e                	je     0x18012c9c0
   18012c9b2:	48 8b c8             	mov    rcx,rax
   18012c9b5:	ff 15 75 33 4b 00    	call   QWORD PTR [rip+0x4b3375]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   18012c9bb:	48 8b d8             	mov    rbx,rax
   18012c9be:	eb 02                	jmp    0x18012c9c2
   18012c9c0:	33 db                	xor    ebx,ebx
   18012c9c2:	48 8d 0d 77 aa 16 01 	lea    rcx,[rip+0x116aa77]        # 0x181297440
   18012c9c9:	e8 82 43 ee ff       	call   0x180010d50
   18012c9ce:	48 8b c8             	mov    rcx,rax
   18012c9d1:	e8 ba 42 ee ff       	call   0x180010c90
   18012c9d6:	8b c8                	mov    ecx,eax
   18012c9d8:	ff 15 aa 32 4b 00    	call   QWORD PTR [rip+0x4b32aa]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   18012c9de:	8b d0                	mov    edx,eax
   18012c9e0:	4c 8b c3             	mov    r8,rbx
   18012c9e3:	48 8d 8d e8 02 00 00 	lea    rcx,[rbp+0x2e8]
   18012c9ea:	ff 15 90 32 4b 00    	call   QWORD PTR [rip+0x4b3290]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   18012c9f0:	48 8d 0d 49 aa 16 01 	lea    rcx,[rip+0x116aa49]        # 0x181297440
   18012c9f7:	e8 54 43 ee ff       	call   0x180010d50
   18012c9fc:	48 8b c8             	mov    rcx,rax
   18012c9ff:	e8 8c 42 ee ff       	call   0x180010c90
   18012ca04:	8b c8                	mov    ecx,eax
   18012ca06:	45 33 c0             	xor    r8d,r8d
   18012ca09:	48 8d 95 e8 02 00 00 	lea    rdx,[rbp+0x2e8]
   18012ca10:	ff 15 7a 35 4b 00    	call   QWORD PTR [rip+0x4b357a]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   18012ca16:	48 8d 0d 23 aa 16 01 	lea    rcx,[rip+0x116aa23]        # 0x181297440
   18012ca1d:	e8 2e 43 ee ff       	call   0x180010d50
   18012ca22:	48 8b c8             	mov    rcx,rax
   18012ca25:	e8 66 42 ee ff       	call   0x180010c90
   18012ca2a:	8b c8                	mov    ecx,eax
   18012ca2c:	ff 15 56 32 4b 00    	call   QWORD PTR [rip+0x4b3256]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   18012ca32:	8b d0                	mov    edx,eax
   18012ca34:	4c 8b c3             	mov    r8,rbx
   18012ca37:	48 8d 0d 12 aa 16 01 	lea    rcx,[rip+0x116aa12]        # 0x181297450
   18012ca3e:	ff 15 4c 32 4b 00    	call   QWORD PTR [rip+0x4b324c]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   18012ca44:	8b 04 3e             	mov    eax,DWORD PTR [rsi+rdi*1]
   18012ca47:	39 05 13 aa 16 01    	cmp    DWORD PTR [rip+0x116aa13],eax        # 0x181297460
   18012ca4d:	7e 41                	jle    0x18012ca90
   18012ca4f:	48 8d 0d 0a aa 16 01 	lea    rcx,[rip+0x116aa0a]        # 0x181297460
   18012ca56:	e8 09 b4 45 00       	call   0x180587e64
   18012ca5b:	83 3d fe a9 16 01 ff 	cmp    DWORD PTR [rip+0x116a9fe],0xffffffff        # 0x181297460
   18012ca62:	75 2c                	jne    0x18012ca90
   18012ca64:	48 8d 15 95 39 50 00 	lea    rdx,[rip+0x503995]        # 0x180630400 ; 'g_rwtHitInfo'
   18012ca6b:	48 8d 0d f6 a9 16 01 	lea    rcx,[rip+0x116a9f6]        # 0x181297468
   18012ca72:	e8 79 66 ee ff       	call   0x1800130f0
   18012ca77:	48 8d 0d 42 18 4a 00 	lea    rcx,[rip+0x4a1842]        # 0x1805ce2c0
   18012ca7e:	e8 41 b1 45 00       	call   0x180587bc4
   18012ca83:	90                   	nop
   18012ca84:	48 8d 0d d5 a9 16 01 	lea    rcx,[rip+0x116a9d5]        # 0x181297460
   18012ca8b:	e8 74 b3 45 00       	call   0x180587e04
   18012ca90:	48 8d 8d 90 00 00 00 	lea    rcx,[rbp+0x90]
   18012ca97:	e8 04 69 ee ff       	call   0x1800133a0
   18012ca9c:	48 85 c0             	test   rax,rax
   18012ca9f:	74 0e                	je     0x18012caaf
   18012caa1:	48 8b c8             	mov    rcx,rax
   18012caa4:	ff 15 86 32 4b 00    	call   QWORD PTR [rip+0x4b3286]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   18012caaa:	48 8b d8             	mov    rbx,rax
   18012caad:	eb 02                	jmp    0x18012cab1
   18012caaf:	33 db                	xor    ebx,ebx
   18012cab1:	48 8d 0d b0 a9 16 01 	lea    rcx,[rip+0x116a9b0]        # 0x181297468
   18012cab8:	e8 93 42 ee ff       	call   0x180010d50
   18012cabd:	48 8b c8             	mov    rcx,rax
   18012cac0:	e8 cb 41 ee ff       	call   0x180010c90
   18012cac5:	8b c8                	mov    ecx,eax
   18012cac7:	ff 15 bb 31 4b 00    	call   QWORD PTR [rip+0x4b31bb]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   18012cacd:	8b d0                	mov    edx,eax
   18012cacf:	4c 8b c3             	mov    r8,rbx
   18012cad2:	48 8d 8d f0 01 00 00 	lea    rcx,[rbp+0x1f0]
   18012cad9:	ff 15 a1 31 4b 00    	call   QWORD PTR [rip+0x4b31a1]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   18012cadf:	48 8d 0d 82 a9 16 01 	lea    rcx,[rip+0x116a982]        # 0x181297468
   18012cae6:	e8 65 42 ee ff       	call   0x180010d50
   18012caeb:	48 8b c8             	mov    rcx,rax
   18012caee:	e8 9d 41 ee ff       	call   0x180010c90
   18012caf3:	8b c8                	mov    ecx,eax
   18012caf5:	45 33 c0             	xor    r8d,r8d
   18012caf8:	48 8d 95 f0 01 00 00 	lea    rdx,[rbp+0x1f0]
   18012caff:	ff 15 8b 34 4b 00    	call   QWORD PTR [rip+0x4b348b]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   18012cb05:	48 8d 0d 5c a9 16 01 	lea    rcx,[rip+0x116a95c]        # 0x181297468
   18012cb0c:	e8 3f 42 ee ff       	call   0x180010d50
   18012cb11:	48 8b c8             	mov    rcx,rax
   18012cb14:	e8 77 41 ee ff       	call   0x180010c90
   18012cb19:	8b c8                	mov    ecx,eax
   18012cb1b:	ff 15 67 31 4b 00    	call   QWORD PTR [rip+0x4b3167]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   18012cb21:	8b d0                	mov    edx,eax
   18012cb23:	4c 8b c3             	mov    r8,rbx
   18012cb26:	48 8d 0d 4b a9 16 01 	lea    rcx,[rip+0x116a94b]        # 0x181297478
   18012cb2d:	ff 15 5d 31 4b 00    	call   QWORD PTR [rip+0x4b315d]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   18012cb33:	83 bd 00 04 00 00 01 	cmp    DWORD PTR [rbp+0x400],0x1
   18012cb3a:	75 08                	jne    0x18012cb44
   18012cb3c:	41 be 20 00 00 00    	mov    r14d,0x20
   18012cb42:	eb 19                	jmp    0x18012cb5d
   18012cb44:	45 84 e4             	test   r12b,r12b
   18012cb47:	74 08                	je     0x18012cb51
   18012cb49:	41 be 04 00 00 00    	mov    r14d,0x4
   18012cb4f:	eb 0c                	jmp    0x18012cb5d
   18012cb51:	41 0f b6 c6          	movzx  eax,r14b
   18012cb55:	44 8d 34 c5 08 00 00 	lea    r14d,[rax*8+0x8]
   18012cb5c:	00 
   18012cb5d:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012cb60:	48 81 c1 f8 00 00 00 	add    rcx,0xf8
   18012cb67:	e8 34 68 ee ff       	call   0x1800133a0
   18012cb6c:	48 8b c8             	mov    rcx,rax
   18012cb6f:	e8 3c 45 02 00       	call   0x1801510b0
   18012cb74:	45 33 e4             	xor    r12d,r12d
   18012cb77:	41 8b dc             	mov    ebx,r12d
   18012cb7a:	85 c0                	test   eax,eax
   18012cb7c:	b8 02 00 00 00       	mov    eax,0x2
   18012cb81:	0f 4f d8             	cmovg  ebx,eax
   18012cb84:	41 0b de             	or     ebx,r14d
   18012cb87:	48 8b 0d 4a 0d 16 01 	mov    rcx,QWORD PTR [rip+0x1160d4a]        # 0x18128d8d8
   18012cb8e:	48 85 c9             	test   rcx,rcx
   18012cb91:	75 35                	jne    0x18012cbc8
   18012cb93:	48 8d 0d 36 37 50 00 	lea    rcx,[rip+0x503736]        # 0x1806302d0 ; 'rt_deferred_diffusegi.rfx'
   18012cb9a:	e8 81 63 0a 00       	call   0x1801d2f20
   18012cb9f:	48 8b c8             	mov    rcx,rax
   18012cba2:	48 8d 15 3f 38 50 00 	lea    rdx,[rip+0x50383f]        # 0x1806303e8 ; 'rt_deferredDiffuseGI'
   18012cba9:	e8 32 61 0a 00       	call   0x1801d2ce0
   18012cbae:	48 89 05 23 0d 16 01 	mov    QWORD PTR [rip+0x1160d23],rax        # 0x18128d8d8
   18012cbb5:	48 8d 0d 1c 0d 16 01 	lea    rcx,[rip+0x1160d1c]        # 0x18128d8d8
   18012cbbc:	e8 3f bb 0a 00       	call   0x1801d8700
   18012cbc1:	48 8b 0d 10 0d 16 01 	mov    rcx,QWORD PTR [rip+0x1160d10]        # 0x18128d8d8
   18012cbc8:	8b d3                	mov    edx,ebx
   18012cbca:	e8 81 08 0b 00       	call   0x1801dd450
   18012cbcf:	48 8b c8             	mov    rcx,rax
   18012cbd2:	e8 79 eb 0a 00       	call   0x1801db750
   18012cbd7:	8b 0d af 18 7e 00    	mov    ecx,DWORD PTR [rip+0x7e18af]        # 0x18090e48c
   18012cbdd:	83 c1 0f             	add    ecx,0xf
   18012cbe0:	c1 e9 04             	shr    ecx,0x4
   18012cbe3:	8b 05 9f 18 7e 00    	mov    eax,DWORD PTR [rip+0x7e189f]        # 0x18090e488
   18012cbe9:	83 c0 07             	add    eax,0x7
   18012cbec:	c1 e8 03             	shr    eax,0x3
   18012cbef:	89 45 90             	mov    DWORD PTR [rbp-0x70],eax
   18012cbf2:	89 4d 94             	mov    DWORD PTR [rbp-0x6c],ecx
   18012cbf5:	f2 0f 10 45 90       	movsd  xmm0,QWORD PTR [rbp-0x70]
   18012cbfa:	f2 0f 11 85 80 00 00 	movsd  QWORD PTR [rbp+0x80],xmm0
   18012cc01:	00 
   18012cc02:	c7 85 88 00 00 00 01 	mov    DWORD PTR [rbp+0x88],0x1
   18012cc09:	00 00 00 
   18012cc0c:	48 8d 95 80 00 00 00 	lea    rdx,[rbp+0x80]
   18012cc13:	49 8b cd             	mov    rcx,r13
   18012cc16:	ff 15 a4 30 4b 00    	call   QWORD PTR [rip+0x4b30a4]        # 0x1805dfcc0 ; ?dispatch@DeviceUtil@d3d@@SAXAEAVDeviceState@2@V?$Vector3Template@H@m@@@Z
   18012cc1c:	8b 85 f0 03 00 00    	mov    eax,DWORD PTR [rbp+0x3f0]
   18012cc22:	ff c0                	inc    eax
   18012cc24:	89 85 f0 03 00 00    	mov    DWORD PTR [rbp+0x3f0],eax
   18012cc2a:	3b 85 00 04 00 00    	cmp    eax,DWORD PTR [rbp+0x400]
   18012cc30:	4c 8d 35 21 37 50 00 	lea    r14,[rip+0x503721]        # 0x180630358 ; 'closestHitMain'
   18012cc37:	0f 82 69 f3 ff ff    	jb     0x18012bfa6
   18012cc3d:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012cc40:	48 81 c1 40 02 00 00 	add    rcx,0x240
   18012cc47:	e8 54 67 ee ff       	call   0x1800133a0
   18012cc4c:	48 8b c8             	mov    rcx,rax
   18012cc4f:	e8 3c 65 f7 ff       	call   0x1800a3190
   18012cc54:	8b 04 3e             	mov    eax,DWORD PTR [rsi+rdi*1]
   18012cc57:	39 05 2b a8 16 01    	cmp    DWORD PTR [rip+0x116a82b],eax        # 0x181297488
   18012cc5d:	7e 41                	jle    0x18012cca0
   18012cc5f:	48 8d 0d 22 a8 16 01 	lea    rcx,[rip+0x116a822]        # 0x181297488
   18012cc66:	e8 f9 b1 45 00       	call   0x180587e64
   18012cc6b:	83 3d 16 a8 16 01 ff 	cmp    DWORD PTR [rip+0x116a816],0xffffffff        # 0x181297488
   18012cc72:	75 2c                	jne    0x18012cca0
   18012cc74:	48 8d 15 b5 37 50 00 	lea    rdx,[rip+0x5037b5]        # 0x180630430 ; 'g_tHitInfo'
   18012cc7b:	48 8d 0d 0e a8 16 01 	lea    rcx,[rip+0x116a80e]        # 0x181297490
   18012cc82:	e8 69 64 ee ff       	call   0x1800130f0
   18012cc87:	48 8d 0d 02 16 4a 00 	lea    rcx,[rip+0x4a1602]        # 0x1805ce290
   18012cc8e:	e8 31 af 45 00       	call   0x180587bc4
   18012cc93:	90                   	nop
   18012cc94:	48 8d 0d ed a7 16 01 	lea    rcx,[rip+0x116a7ed]        # 0x181297488
   18012cc9b:	e8 64 b1 45 00       	call   0x180587e04
   18012cca0:	48 8d 8d 90 00 00 00 	lea    rcx,[rbp+0x90]
   18012cca7:	e8 f4 66 ee ff       	call   0x1800133a0
   18012ccac:	48 85 c0             	test   rax,rax
   18012ccaf:	74 0e                	je     0x18012ccbf
   18012ccb1:	48 8b c8             	mov    rcx,rax
   18012ccb4:	ff 15 76 30 4b 00    	call   QWORD PTR [rip+0x4b3076]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   18012ccba:	48 8b d8             	mov    rbx,rax
   18012ccbd:	eb 03                	jmp    0x18012ccc2
   18012ccbf:	49 8b dc             	mov    rbx,r12
   18012ccc2:	48 8d 0d c7 a7 16 01 	lea    rcx,[rip+0x116a7c7]        # 0x181297490
   18012ccc9:	e8 82 40 ee ff       	call   0x180010d50
   18012ccce:	48 8b c8             	mov    rcx,rax
   18012ccd1:	e8 ba 3f ee ff       	call   0x180010c90
   18012ccd6:	8b c8                	mov    ecx,eax
   18012ccd8:	ff 15 aa 2f 4b 00    	call   QWORD PTR [rip+0x4b2faa]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   18012ccde:	8b d0                	mov    edx,eax
   18012cce0:	4c 8b c3             	mov    r8,rbx
   18012cce3:	48 8d 4d b0          	lea    rcx,[rbp-0x50]
   18012cce7:	ff 15 93 2f 4b 00    	call   QWORD PTR [rip+0x4b2f93]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   18012cced:	48 8d 0d 9c a7 16 01 	lea    rcx,[rip+0x116a79c]        # 0x181297490
   18012ccf4:	e8 57 40 ee ff       	call   0x180010d50
   18012ccf9:	48 8b c8             	mov    rcx,rax
   18012ccfc:	e8 8f 3f ee ff       	call   0x180010c90
   18012cd01:	8b c8                	mov    ecx,eax
   18012cd03:	45 33 c0             	xor    r8d,r8d
   18012cd06:	48 8d 55 b0          	lea    rdx,[rbp-0x50]
   18012cd0a:	ff 15 80 32 4b 00    	call   QWORD PTR [rip+0x4b3280]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   18012cd10:	48 8d 0d 79 a7 16 01 	lea    rcx,[rip+0x116a779]        # 0x181297490
   18012cd17:	e8 34 40 ee ff       	call   0x180010d50
   18012cd1c:	48 8b c8             	mov    rcx,rax
   18012cd1f:	e8 6c 3f ee ff       	call   0x180010c90
   18012cd24:	8b c8                	mov    ecx,eax
   18012cd26:	ff 15 5c 2f 4b 00    	call   QWORD PTR [rip+0x4b2f5c]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   18012cd2c:	8b d0                	mov    edx,eax
   18012cd2e:	4c 8b c3             	mov    r8,rbx
   18012cd31:	48 8d 0d 68 a7 16 01 	lea    rcx,[rip+0x116a768]        # 0x1812974a0
   18012cd38:	ff 15 52 2f 4b 00    	call   QWORD PTR [rip+0x4b2f52]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   18012cd3e:	48 8d 0d 2b 78 7e 00 	lea    rcx,[rip+0x7e782b]        # 0x180914570
   18012cd45:	e8 d6 6f ee ff       	call   0x180013d20
   18012cd4a:	84 c0                	test   al,al
   18012cd4c:	74 11                	je     0x18012cd5f
   18012cd4e:	48 8d 0d 73 1d 7e 00 	lea    rcx,[rip+0x7e1d73]        # 0x18090eac8
   18012cd55:	e8 96 70 ee ff       	call   0x180013df0
   18012cd5a:	e9 ec 01 00 00       	jmp    0x18012cf4b
   18012cd5f:	48 8d 0d 9a 66 7e 00 	lea    rcx,[rip+0x7e669a]        # 0x180913400
   18012cd66:	e8 55 0d f0 ff       	call   0x18002dac0
   18012cd6b:	85 c0                	test   eax,eax
   18012cd6d:	7e 31                	jle    0x18012cda0
   18012cd6f:	48 8d 0d 8a 66 7e 00 	lea    rcx,[rip+0x7e668a]        # 0x180913400
   18012cd76:	e8 45 0d f0 ff       	call   0x18002dac0
   18012cd7b:	8b d8                	mov    ebx,eax
   18012cd7d:	48 8d 0d 74 1c 7e 00 	lea    rcx,[rip+0x7e1c74]        # 0x18090e9f8
   18012cd84:	e8 17 66 ee ff       	call   0x1800133a0
   18012cd89:	48 8b c8             	mov    rcx,rax
   18012cd8c:	44 8b c3             	mov    r8d,ebx
   18012cd8f:	48 8d 15 32 1d 7e 00 	lea    rdx,[rip+0x7e1d32]        # 0x18090eac8
   18012cd96:	e8 f5 7e ee ff       	call   0x180014c90
   18012cd9b:	e9 ab 01 00 00       	jmp    0x18012cf4b
   18012cda0:	48 8d 0d 21 1d 7e 00 	lea    rcx,[rip+0x7e1d21]        # 0x18090eac8
   18012cda7:	e8 74 b6 00 00       	call   0x180138420
   18012cdac:	84 c0                	test   al,al
   18012cdae:	75 6f                	jne    0x18012ce1f
   18012cdb0:	48 8d 0d 11 1d 7e 00 	lea    rcx,[rip+0x7e1d11]        # 0x18090eac8
   18012cdb7:	e8 e4 65 ee ff       	call   0x1800133a0
   18012cdbc:	48 8b c8             	mov    rcx,rax
   18012cdbf:	ff 15 23 2f 4b 00    	call   QWORD PTR [rip+0x4b2f23]        # 0x1805dfce8 ; ?getWidth@Texture2D@d3d@@QEBAHXZ
   18012cdc5:	3b 05 bd 16 7e 00    	cmp    eax,DWORD PTR [rip+0x7e16bd]        # 0x18090e488
   18012cdcb:	75 52                	jne    0x18012ce1f
   18012cdcd:	48 8d 0d f4 1c 7e 00 	lea    rcx,[rip+0x7e1cf4]        # 0x18090eac8
   18012cdd4:	e8 c7 65 ee ff       	call   0x1800133a0
   18012cdd9:	48 8b c8             	mov    rcx,rax
   18012cddc:	ff 15 fe 2e 4b 00    	call   QWORD PTR [rip+0x4b2efe]        # 0x1805dfce0 ; ?getHeight@Texture2D@d3d@@QEBAHXZ
   18012cde2:	3b 05 a4 16 7e 00    	cmp    eax,DWORD PTR [rip+0x7e16a4]        # 0x18090e48c
   18012cde8:	75 35                	jne    0x18012ce1f
   18012cdea:	48 8d 0d 07 1c 7e 00 	lea    rcx,[rip+0x7e1c07]        # 0x18090e9f8
   18012cdf1:	e8 aa 65 ee ff       	call   0x1800133a0
   18012cdf6:	48 8b c8             	mov    rcx,rax
   18012cdf9:	ff 15 39 2f 4b 00    	call   QWORD PTR [rip+0x4b2f39]        # 0x1805dfd38 ; ?getFormat@Texture2D@d3d@@QEBA?AW4PixelFormat@2@XZ
   18012cdff:	0f b6 d8             	movzx  ebx,al
   18012ce02:	48 8d 0d bf 1c 7e 00 	lea    rcx,[rip+0x7e1cbf]        # 0x18090eac8
   18012ce09:	e8 92 65 ee ff       	call   0x1800133a0
   18012ce0e:	48 8b c8             	mov    rcx,rax
   18012ce11:	ff 15 21 2f 4b 00    	call   QWORD PTR [rip+0x4b2f21]        # 0x1805dfd38 ; ?getFormat@Texture2D@d3d@@QEBA?AW4PixelFormat@2@XZ
   18012ce17:	3a c3                	cmp    al,bl
   18012ce19:	0f 84 ae 00 00 00    	je     0x18012cecd
   18012ce1f:	0f 57 c0             	xorps  xmm0,xmm0
   18012ce22:	66 0f 7f 45 b0       	movdqa XMMWORD PTR [rbp-0x50],xmm0
   18012ce27:	48 8d 0d ca 1b 7e 00 	lea    rcx,[rip+0x7e1bca]        # 0x18090e9f8
   18012ce2e:	e8 6d 65 ee ff       	call   0x1800133a0
   18012ce33:	48 8b c8             	mov    rcx,rax
   18012ce36:	ff 15 fc 2e 4b 00    	call   QWORD PTR [rip+0x4b2efc]        # 0x1805dfd38 ; ?getFormat@Texture2D@d3d@@QEBA?AW4PixelFormat@2@XZ
   18012ce3c:	c7 44 24 70 ff ff ff 	mov    DWORD PTR [rsp+0x70],0xffffffff
   18012ce43:	ff 
   18012ce44:	48 8d 4d b0          	lea    rcx,[rbp-0x50]
   18012ce48:	48 89 4c 24 68       	mov    QWORD PTR [rsp+0x68],rcx
   18012ce4d:	c7 44 24 60 01 00 00 	mov    DWORD PTR [rsp+0x60],0x1
   18012ce54:	00 
   18012ce55:	f3 0f 11 74 24 58    	movss  DWORD PTR [rsp+0x58],xmm6
   18012ce5b:	44 89 64 24 50       	mov    DWORD PTR [rsp+0x50],r12d
   18012ce60:	44 89 64 24 48       	mov    DWORD PTR [rsp+0x48],r12d
   18012ce65:	4c 89 64 24 40       	mov    QWORD PTR [rsp+0x40],r12
   18012ce6a:	4c 89 64 24 38       	mov    QWORD PTR [rsp+0x38],r12
   18012ce6f:	44 89 64 24 30       	mov    DWORD PTR [rsp+0x30],r12d
   18012ce74:	c7 44 24 28 05 00 00 	mov    DWORD PTR [rsp+0x28],0x5
   18012ce7b:	00 
   18012ce7c:	88 44 24 20          	mov    BYTE PTR [rsp+0x20],al
   18012ce80:	41 b9 01 00 00 00    	mov    r9d,0x1
   18012ce86:	44 8b 05 ff 15 7e 00 	mov    r8d,DWORD PTR [rip+0x7e15ff]        # 0x18090e48c
   18012ce8d:	8b 15 f5 15 7e 00    	mov    edx,DWORD PTR [rip+0x7e15f5]        # 0x18090e488
   18012ce93:	48 8d 4d e0          	lea    rcx,[rbp-0x20]
   18012ce97:	ff 15 8b 2e 4b 00    	call   QWORD PTR [rip+0x4b2e8b]        # 0x1805dfd28 ; ??0Texture2DInit@d3d@@QEAA@HHHW4PixelFormat@1@HHPEBX_KW4MiscFlags@1@HMHV?$Vector4Template@M@m@@W4TileMode@1@@Z
   18012ce9d:	48 8b d8             	mov    rbx,rax
   18012cea0:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012cea3:	48 83 c1 20          	add    rcx,0x20
   18012cea7:	e8 f4 64 ee ff       	call   0x1800133a0
   18012ceac:	48 8b c8             	mov    rcx,rax
   18012ceaf:	4c 8d 05 ba 29 4f 00 	lea    r8,[rip+0x4f29ba]        # 0x18061f870 ; 'm_pColorHistory'
   18012ceb6:	48 8b d3             	mov    rdx,rbx
   18012ceb9:	e8 a2 c8 f7 ff       	call   0x1800a9760
   18012cebe:	48 8b d0             	mov    rdx,rax
   18012cec1:	48 8d 0d 00 1c 7e 00 	lea    rcx,[rip+0x7e1c00]        # 0x18090eac8
   18012cec8:	e8 c3 b1 ee ff       	call   0x180018090
   18012cecd:	48 c7 45 90 00 00 00 	mov    QWORD PTR [rbp-0x70],0x0
   18012ced4:	00 
   18012ced5:	44 89 65 98          	mov    DWORD PTR [rbp-0x68],r12d
   18012ced9:	48 c7 85 80 00 00 00 	mov    QWORD PTR [rbp+0x80],0x0
   18012cee0:	00 00 00 00 
   18012cee4:	44 89 a5 88 00 00 00 	mov    DWORD PTR [rbp+0x88],r12d
   18012ceeb:	48 8d 0d 06 1b 7e 00 	lea    rcx,[rip+0x7e1b06]        # 0x18090e9f8
   18012cef2:	e8 a9 64 ee ff       	call   0x1800133a0
   18012cef7:	48 8b c8             	mov    rcx,rax
   18012cefa:	ff 15 30 2e 4b 00    	call   QWORD PTR [rip+0x4b2e30]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   18012cf00:	48 8b d8             	mov    rbx,rax
   18012cf03:	48 8d 0d be 1b 7e 00 	lea    rcx,[rip+0x7e1bbe]        # 0x18090eac8
   18012cf0a:	e8 91 64 ee ff       	call   0x1800133a0
   18012cf0f:	48 8b c8             	mov    rcx,rax
   18012cf12:	ff 15 18 2e 4b 00    	call   QWORD PTR [rip+0x4b2e18]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   18012cf18:	48 8b c8             	mov    rcx,rax
   18012cf1b:	4c 89 64 24 40       	mov    QWORD PTR [rsp+0x40],r12
   18012cf20:	48 8d 45 90          	lea    rax,[rbp-0x70]
   18012cf24:	48 89 44 24 38       	mov    QWORD PTR [rsp+0x38],rax
   18012cf29:	44 89 64 24 30       	mov    DWORD PTR [rsp+0x30],r12d
   18012cf2e:	44 89 64 24 28       	mov    DWORD PTR [rsp+0x28],r12d
   18012cf33:	48 89 5c 24 20       	mov    QWORD PTR [rsp+0x20],rbx
   18012cf38:	4c 8d 8d 80 00 00 00 	lea    r9,[rbp+0x80]
   18012cf3f:	45 33 c0             	xor    r8d,r8d
   18012cf42:	33 d2                	xor    edx,edx
   18012cf44:	ff 15 fe 2d 4b 00    	call   QWORD PTR [rip+0x4b2dfe]        # 0x1805dfd48 ; ?copyRegion@NativeTextureUtil@d3d@@SAXPEAVNativeTexture@2@HHAEBV?$Vector3Template@H@m@@0HH1PEBV45@@Z
   18012cf4a:	90                   	nop
   18012cf4b:	48 8d 4d c0          	lea    rcx,[rbp-0x40]
   18012cf4f:	e8 6c b1 ee ff       	call   0x1800180c0
   18012cf54:	90                   	nop
   18012cf55:	48 8d 8d 90 00 00 00 	lea    rcx,[rbp+0x90]
   18012cf5c:	e8 5f b1 ee ff       	call   0x1800180c0
   18012cf61:	80 3d a2 5a 6d 00 00 	cmp    BYTE PTR [rip+0x6d5aa2],0x0        # 0x180802a0a
   18012cf68:	74 15                	je     0x18012cf7f
   18012cf6a:	48 8d 0d 2f 61 7e 00 	lea    rcx,[rip+0x7e612f]        # 0x1809130a0
   18012cf71:	e8 aa 6d ee ff       	call   0x180013d20
   18012cf76:	84 c0                	test   al,al
   18012cf78:	74 05                	je     0x18012cf7f
   18012cf7a:	41 b6 01             	mov    r14b,0x1
   18012cf7d:	eb 03                	jmp    0x18012cf82
   18012cf7f:	45 32 f6             	xor    r14b,r14b
   18012cf82:	8b 0d b4 58 6d 00    	mov    ecx,DWORD PTR [rip+0x6d58b4]        # 0x18080283c
   18012cf88:	65 48 8b 04 25 58 00 	mov    rax,QWORD PTR gs:0x58
   18012cf8f:	00 00 
   18012cf91:	ba 40 00 00 00       	mov    edx,0x40
   18012cf96:	8b fa                	mov    edi,edx
   18012cf98:	48 8b 34 c8          	mov    rsi,QWORD PTR [rax+rcx*8]
   18012cf9c:	8b 04 16             	mov    eax,DWORD PTR [rsi+rdx*1]
   18012cf9f:	39 05 0b a5 16 01    	cmp    DWORD PTR [rip+0x116a50b],eax        # 0x1812974b0
   18012cfa5:	7e 33                	jle    0x18012cfda
   18012cfa7:	48 8d 0d 02 a5 16 01 	lea    rcx,[rip+0x116a502]        # 0x1812974b0
   18012cfae:	e8 b1 ae 45 00       	call   0x180587e64
   18012cfb3:	83 3d f6 a4 16 01 ff 	cmp    DWORD PTR [rip+0x116a4f6],0xffffffff        # 0x1812974b0
   18012cfba:	75 1e                	jne    0x18012cfda
   18012cfbc:	48 8d 0d 0d ef 95 00 	lea    rcx,[rip+0x95ef0d]        # 0x180a8bed0
   18012cfc3:	e8 58 6d ee ff       	call   0x180013d20
   18012cfc8:	88 05 e6 a4 16 01    	mov    BYTE PTR [rip+0x116a4e6],al        # 0x1812974b4
   18012cfce:	48 8d 0d db a4 16 01 	lea    rcx,[rip+0x116a4db]        # 0x1812974b0
   18012cfd5:	e8 2a ae 45 00       	call   0x180587e04
   18012cfda:	45 84 f6             	test   r14b,r14b
   18012cfdd:	0f 84 fe 05 00 00    	je     0x18012d5e1
   18012cfe3:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012cfe6:	48 81 c1 18 04 00 00 	add    rcx,0x418
   18012cfed:	e8 ae 63 ee ff       	call   0x1800133a0
   18012cff2:	48 8b c8             	mov    rcx,rax
   18012cff5:	ff 15 35 2d 4b 00    	call   QWORD PTR [rip+0x4b2d35]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   18012cffb:	48 8b c8             	mov    rcx,rax
   18012cffe:	ff 15 7c 2f 4b 00    	call   QWORD PTR [rip+0x4b2f7c]        # 0x1805dff80 ; ?prepareForProducing@NativeTexture@d3d@@QEAAXXZ
   18012d004:	80 3d c5 08 16 01 00 	cmp    BYTE PTR [rip+0x11608c5],0x0        # 0x18128d8d0
   18012d00b:	75 20                	jne    0x18012d02d
   18012d00d:	48 8d 0d bc ee 95 00 	lea    rcx,[rip+0x95eebc]        # 0x180a8bed0
   18012d014:	e8 07 6d ee ff       	call   0x180013d20
   18012d019:	88 05 95 a4 16 01    	mov    BYTE PTR [rip+0x116a495],al        # 0x1812974b4
   18012d01f:	33 d2                	xor    edx,edx
   18012d021:	48 8d 0d a8 ee 95 00 	lea    rcx,[rip+0x95eea8]        # 0x180a8bed0
   18012d028:	e8 e3 6c ee ff       	call   0x180013d10
   18012d02d:	8b 04 3e             	mov    eax,DWORD PTR [rsi+rdi*1]
   18012d030:	39 05 82 a4 16 01    	cmp    DWORD PTR [rip+0x116a482],eax        # 0x1812974b8
   18012d036:	7e 41                	jle    0x18012d079
   18012d038:	48 8d 0d 79 a4 16 01 	lea    rcx,[rip+0x116a479]        # 0x1812974b8
   18012d03f:	e8 20 ae 45 00       	call   0x180587e64
   18012d044:	83 3d 6d a4 16 01 ff 	cmp    DWORD PTR [rip+0x116a46d],0xffffffff        # 0x1812974b8
   18012d04b:	75 2c                	jne    0x18012d079
   18012d04d:	48 8d 15 bc 33 50 00 	lea    rdx,[rip+0x5033bc]        # 0x180630410 ; 'g_rwtDeferredShadowTarget'
   18012d054:	48 8d 0d 65 a4 16 01 	lea    rcx,[rip+0x116a465]        # 0x1812974c0
   18012d05b:	e8 90 60 ee ff       	call   0x1800130f0
   18012d060:	48 8d 0d f9 11 4a 00 	lea    rcx,[rip+0x4a11f9]        # 0x1805ce260
   18012d067:	e8 58 ab 45 00       	call   0x180587bc4
   18012d06c:	90                   	nop
   18012d06d:	48 8d 0d 44 a4 16 01 	lea    rcx,[rip+0x116a444]        # 0x1812974b8
   18012d074:	e8 8b ad 45 00       	call   0x180587e04
   18012d079:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012d07c:	48 81 c1 18 04 00 00 	add    rcx,0x418
   18012d083:	e8 18 63 ee ff       	call   0x1800133a0
   18012d088:	48 85 c0             	test   rax,rax
   18012d08b:	74 0e                	je     0x18012d09b
   18012d08d:	48 8b c8             	mov    rcx,rax
   18012d090:	ff 15 9a 2c 4b 00    	call   QWORD PTR [rip+0x4b2c9a]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   18012d096:	48 8b d8             	mov    rbx,rax
   18012d099:	eb 03                	jmp    0x18012d09e
   18012d09b:	49 8b dc             	mov    rbx,r12
   18012d09e:	48 8d 0d 1b a4 16 01 	lea    rcx,[rip+0x116a41b]        # 0x1812974c0
   18012d0a5:	e8 a6 3c ee ff       	call   0x180010d50
   18012d0aa:	48 8b c8             	mov    rcx,rax
   18012d0ad:	e8 de 3b ee ff       	call   0x180010c90
   18012d0b2:	8b c8                	mov    ecx,eax
   18012d0b4:	ff 15 ce 2b 4b 00    	call   QWORD PTR [rip+0x4b2bce]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   18012d0ba:	8b d0                	mov    edx,eax
   18012d0bc:	4c 8b c3             	mov    r8,rbx
   18012d0bf:	48 8d 4d b0          	lea    rcx,[rbp-0x50]
   18012d0c3:	ff 15 b7 2b 4b 00    	call   QWORD PTR [rip+0x4b2bb7]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   18012d0c9:	48 8d 0d f0 a3 16 01 	lea    rcx,[rip+0x116a3f0]        # 0x1812974c0
   18012d0d0:	e8 7b 3c ee ff       	call   0x180010d50
   18012d0d5:	48 8b c8             	mov    rcx,rax
   18012d0d8:	e8 b3 3b ee ff       	call   0x180010c90
   18012d0dd:	8b c8                	mov    ecx,eax
   18012d0df:	45 33 c0             	xor    r8d,r8d
   18012d0e2:	48 8d 55 b0          	lea    rdx,[rbp-0x50]
   18012d0e6:	ff 15 a4 2e 4b 00    	call   QWORD PTR [rip+0x4b2ea4]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   18012d0ec:	48 8d 0d cd a3 16 01 	lea    rcx,[rip+0x116a3cd]        # 0x1812974c0
   18012d0f3:	e8 58 3c ee ff       	call   0x180010d50
   18012d0f8:	48 8b c8             	mov    rcx,rax
   18012d0fb:	e8 90 3b ee ff       	call   0x180010c90
   18012d100:	8b c8                	mov    ecx,eax
   18012d102:	ff 15 80 2b 4b 00    	call   QWORD PTR [rip+0x4b2b80]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   18012d108:	8b d0                	mov    edx,eax
   18012d10a:	4c 8b c3             	mov    r8,rbx
   18012d10d:	48 8d 0d bc a3 16 01 	lea    rcx,[rip+0x116a3bc]        # 0x1812974d0
   18012d114:	ff 15 76 2b 4b 00    	call   QWORD PTR [rip+0x4b2b76]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   18012d11a:	8b 04 3e             	mov    eax,DWORD PTR [rsi+rdi*1]
   18012d11d:	39 05 bd a3 16 01    	cmp    DWORD PTR [rip+0x116a3bd],eax        # 0x1812974e0
   18012d123:	7e 41                	jle    0x18012d166
   18012d125:	48 8d 0d b4 a3 16 01 	lea    rcx,[rip+0x116a3b4]        # 0x1812974e0
   18012d12c:	e8 33 ad 45 00       	call   0x180587e64
   18012d131:	83 3d a8 a3 16 01 ff 	cmp    DWORD PTR [rip+0x116a3a8],0xffffffff        # 0x1812974e0
   18012d138:	75 2c                	jne    0x18012d166
   18012d13a:	48 8d 15 17 33 50 00 	lea    rdx,[rip+0x503317]        # 0x180630458 ; 'g_fShadowKernelSize'
   18012d141:	48 8d 0d a0 a3 16 01 	lea    rcx,[rip+0x116a3a0]        # 0x1812974e8
   18012d148:	e8 73 5f ee ff       	call   0x1800130c0
   18012d14d:	48 8d 0d dc 10 4a 00 	lea    rcx,[rip+0x4a10dc]        # 0x1805ce230
   18012d154:	e8 6b aa 45 00       	call   0x180587bc4
   18012d159:	90                   	nop
   18012d15a:	48 8d 0d 7f a3 16 01 	lea    rcx,[rip+0x116a37f]        # 0x1812974e0
   18012d161:	e8 9e ac 45 00       	call   0x180587e04
   18012d166:	f3 41 0f 58 f9       	addss  xmm7,xmm9
   18012d16b:	f3 0f 11 bd f0 03 00 	movss  DWORD PTR [rbp+0x3f0],xmm7
   18012d172:	00 
   18012d173:	48 8d 0d 6e a3 16 01 	lea    rcx,[rip+0x116a36e]        # 0x1812974e8
   18012d17a:	e8 d1 3b ee ff       	call   0x180010d50
   18012d17f:	48 8b c8             	mov    rcx,rax
   18012d182:	e8 09 3b ee ff       	call   0x180010c90
   18012d187:	8b c8                	mov    ecx,eax
   18012d189:	45 33 c0             	xor    r8d,r8d
   18012d18c:	48 8d 95 f0 03 00 00 	lea    rdx,[rbp+0x3f0]
   18012d193:	ff 15 f7 2d 4b 00    	call   QWORD PTR [rip+0x4b2df7]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   18012d199:	f3 0f 10 85 f0 03 00 	movss  xmm0,DWORD PTR [rbp+0x3f0]
   18012d1a0:	00 
   18012d1a1:	f3 0f 11 05 4f a3 16 	movss  DWORD PTR [rip+0x116a34f],xmm0        # 0x1812974f8
   18012d1a8:	01 
   18012d1a9:	8b 04 3e             	mov    eax,DWORD PTR [rsi+rdi*1]
   18012d1ac:	39 05 4e a3 16 01    	cmp    DWORD PTR [rip+0x116a34e],eax        # 0x181297500
   18012d1b2:	7e 41                	jle    0x18012d1f5
   18012d1b4:	48 8d 0d 45 a3 16 01 	lea    rcx,[rip+0x116a345]        # 0x181297500
   18012d1bb:	e8 a4 ac 45 00       	call   0x180587e64
   18012d1c0:	83 3d 39 a3 16 01 ff 	cmp    DWORD PTR [rip+0x116a339],0xffffffff        # 0x181297500
   18012d1c7:	75 2c                	jne    0x18012d1f5
   18012d1c9:	48 8d 15 70 32 50 00 	lea    rdx,[rip+0x503270]        # 0x180630440 ; 'g_uRTShadowRayCount'
   18012d1d0:	48 8d 0d 31 a3 16 01 	lea    rcx,[rip+0x116a331]        # 0x181297508
   18012d1d7:	e8 74 45 f0 ff       	call   0x180031750
   18012d1dc:	48 8d 0d 1d 10 4a 00 	lea    rcx,[rip+0x4a101d]        # 0x1805ce200
   18012d1e3:	e8 dc a9 45 00       	call   0x180587bc4
   18012d1e8:	90                   	nop
   18012d1e9:	48 8d 0d 10 a3 16 01 	lea    rcx,[rip+0x116a310]        # 0x181297500
   18012d1f0:	e8 0f ac 45 00       	call   0x180587e04
   18012d1f5:	48 8d 0d e4 69 7e 00 	lea    rcx,[rip+0x7e69e4]        # 0x180913be0
   18012d1fc:	e8 bf 08 f0 ff       	call   0x18002dac0
   18012d201:	89 85 f0 03 00 00    	mov    DWORD PTR [rbp+0x3f0],eax
   18012d207:	48 8d 0d fa a2 16 01 	lea    rcx,[rip+0x116a2fa]        # 0x181297508
   18012d20e:	e8 3d 3b ee ff       	call   0x180010d50
   18012d213:	48 8b c8             	mov    rcx,rax
   18012d216:	e8 75 3a ee ff       	call   0x180010c90
   18012d21b:	8b c8                	mov    ecx,eax
   18012d21d:	45 33 c0             	xor    r8d,r8d
   18012d220:	48 8d 95 f0 03 00 00 	lea    rdx,[rbp+0x3f0]
   18012d227:	ff 15 63 2d 4b 00    	call   QWORD PTR [rip+0x4b2d63]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   18012d22d:	8b 85 f0 03 00 00    	mov    eax,DWORD PTR [rbp+0x3f0]
   18012d233:	89 05 df a2 16 01    	mov    DWORD PTR [rip+0x116a2df],eax        # 0x181297518
   18012d239:	ba 02 00 00 00       	mov    edx,0x2
   18012d23e:	8d 4a ff             	lea    ecx,[rdx-0x1]
   18012d241:	ff 15 59 22 4b 00    	call   QWORD PTR [rip+0x4b2259]        # 0x1805df4a0 ; ?beginPipelineSetup@DeviceUtilRaytracing@d3d@@SAXHH@Z
   18012d247:	48 8b 0d 7a 06 16 01 	mov    rcx,QWORD PTR [rip+0x116067a]        # 0x18128d8c8
   18012d24e:	48 85 c9             	test   rcx,rcx
   18012d251:	75 3f                	jne    0x18012d292
   18012d253:	48 8d 0d 26 32 50 00 	lea    rcx,[rip+0x503226]        # 0x180630480 ; 'rt_sunshadow.rfx'
   18012d25a:	e8 c1 5c 0a 00       	call   0x1801d2f20
   18012d25f:	48 8b c8             	mov    rcx,rax
   18012d262:	48 8d 15 07 32 50 00 	lea    rdx,[rip+0x503207]        # 0x180630470 ; 'rt_SunShadow'
   18012d269:	e8 72 5a 0a 00       	call   0x1801d2ce0
   18012d26e:	48 8b c8             	mov    rcx,rax
   18012d271:	33 d2                	xor    edx,edx
   18012d273:	e8 78 01 0b 00       	call   0x1801dd3f0
   18012d278:	48 89 05 49 06 16 01 	mov    QWORD PTR [rip+0x1160649],rax        # 0x18128d8c8
   18012d27f:	48 8d 0d 42 06 16 01 	lea    rcx,[rip+0x1160642]        # 0x18128d8c8
   18012d286:	e8 75 b4 0a 00       	call   0x1801d8700
   18012d28b:	48 8b 0d 36 06 16 01 	mov    rcx,QWORD PTR [rip+0x1160636]        # 0x18128d8c8
   18012d292:	49 8b d5             	mov    rdx,r13
   18012d295:	e8 76 e5 0a 00       	call   0x1801db810
   18012d29a:	48 8d 0d 07 32 50 00 	lea    rcx,[rip+0x503207]        # 0x1806304a8 ; 'rayGenSunShadow'
   18012d2a1:	ff 15 01 22 4b 00    	call   QWORD PTR [rip+0x4b2201]        # 0x1805df4a8 ; ?setRayGeneration@DeviceUtilRaytracing@d3d@@SAXPEBD@Z
   18012d2a7:	48 8d 15 ea 31 50 00 	lea    rdx,[rip+0x5031ea]        # 0x180630498 ; 'missShadow'
   18012d2ae:	33 c9                	xor    ecx,ecx
   18012d2b0:	ff 15 fa 21 4b 00    	call   QWORD PTR [rip+0x4b21fa]        # 0x1805df4b0 ; ?setMiss@DeviceUtilRaytracing@d3d@@SAXHPEBD@Z
   18012d2b6:	48 8d 05 13 32 50 00 	lea    rax,[rip+0x503213]        # 0x1806304d0 ; 'hitClosestShadow_Opaque'
   18012d2bd:	48 89 44 24 20       	mov    QWORD PTR [rsp+0x20],rax
   18012d2c2:	45 33 c9             	xor    r9d,r9d
   18012d2c5:	45 33 c0             	xor    r8d,r8d
   18012d2c8:	33 d2                	xor    edx,edx
   18012d2ca:	33 c9                	xor    ecx,ecx
   18012d2cc:	ff 15 e6 21 4b 00    	call   QWORD PTR [rip+0x4b21e6]        # 0x1805df4b8 ; ?setHitGroup@DeviceUtilRaytracing@d3d@@SAXHHPEBD00@Z
   18012d2d2:	4c 89 64 24 20       	mov    QWORD PTR [rsp+0x20],r12
   18012d2d7:	4c 8d 0d da 31 50 00 	lea    r9,[rip+0x5031da]        # 0x1806304b8 ; 'hitAnyShadow_AlphaTest'
   18012d2de:	45 33 c0             	xor    r8d,r8d
   18012d2e1:	41 8d 50 01          	lea    edx,[r8+0x1]
   18012d2e5:	33 c9                	xor    ecx,ecx
   18012d2e7:	ff 15 cb 21 4b 00    	call   QWORD PTR [rip+0x4b21cb]        # 0x1805df4b8 ; ?setHitGroup@DeviceUtilRaytracing@d3d@@SAXHHPEBD00@Z
   18012d2ed:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012d2f0:	48 81 c1 20 02 00 00 	add    rcx,0x220
   18012d2f7:	e8 a4 60 ee ff       	call   0x1800133a0
   18012d2fc:	48 8b c8             	mov    rcx,rax
   18012d2ff:	e8 9c ae f7 ff       	call   0x1800a81a0
   18012d304:	8b d8                	mov    ebx,eax
   18012d306:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012d309:	48 81 c1 20 02 00 00 	add    rcx,0x220
   18012d310:	e8 8b 60 ee ff       	call   0x1800133a0
   18012d315:	48 8b c8             	mov    rcx,rax
   18012d318:	e8 73 ae f7 ff       	call   0x1800a8190
   18012d31d:	4c 8b c8             	mov    r9,rax
   18012d320:	89 5c 24 20          	mov    DWORD PTR [rsp+0x20],ebx
   18012d324:	ba 04 00 00 00       	mov    edx,0x4
   18012d329:	8d 4a fd             	lea    ecx,[rdx-0x3]
   18012d32c:	44 8d 42 fe          	lea    r8d,[rdx-0x2]
   18012d330:	ff 15 8a 21 4b 00    	call   QWORD PTR [rip+0x4b218a]        # 0x1805df4c0 ; ?endPipelineSetup@DeviceUtilRaytracing@d3d@@SAXHHHPEBURaytracingGeometryBinding@2@H@Z
   18012d336:	8b 15 50 11 7e 00    	mov    edx,DWORD PTR [rip+0x7e1150]        # 0x18090e48c
   18012d33c:	8b 0d 46 11 7e 00    	mov    ecx,DWORD PTR [rip+0x7e1146]        # 0x18090e488
   18012d342:	ff 15 80 21 4b 00    	call   QWORD PTR [rip+0x4b2180]        # 0x1805df4c8 ; ?raytrace@DeviceUtilRaytracing@d3d@@SAXHH@Z
   18012d348:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012d34b:	48 81 c1 18 04 00 00 	add    rcx,0x418
   18012d352:	e8 49 60 ee ff       	call   0x1800133a0
   18012d357:	48 8b c8             	mov    rcx,rax
   18012d35a:	ff 15 d0 29 4b 00    	call   QWORD PTR [rip+0x4b29d0]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   18012d360:	48 8b c8             	mov    rcx,rax
   18012d363:	ff 15 0f 2c 4b 00    	call   QWORD PTR [rip+0x4b2c0f]        # 0x1805dff78 ; ?prepareForConsuming@NativeTexture@d3d@@QEAAXXZ
   18012d369:	8b 04 3e             	mov    eax,DWORD PTR [rsi+rdi*1]
   18012d36c:	39 05 ae a1 16 01    	cmp    DWORD PTR [rip+0x116a1ae],eax        # 0x181297520
   18012d372:	7e 41                	jle    0x18012d3b5
   18012d374:	48 8d 0d a5 a1 16 01 	lea    rcx,[rip+0x116a1a5]        # 0x181297520
   18012d37b:	e8 e4 aa 45 00       	call   0x180587e64
   18012d380:	83 3d 99 a1 16 01 ff 	cmp    DWORD PTR [rip+0x116a199],0xffffffff        # 0x181297520
   18012d387:	75 2c                	jne    0x18012d3b5
   18012d389:	48 8d 15 70 31 50 00 	lea    rdx,[rip+0x503170]        # 0x180630500 ; 'g_tRTShadowResult'
   18012d390:	48 8d 0d 91 a1 16 01 	lea    rcx,[rip+0x116a191]        # 0x181297528
   18012d397:	e8 54 5d ee ff       	call   0x1800130f0
   18012d39c:	48 8d 0d 2d 0e 4a 00 	lea    rcx,[rip+0x4a0e2d]        # 0x1805ce1d0
   18012d3a3:	e8 1c a8 45 00       	call   0x180587bc4
   18012d3a8:	90                   	nop
   18012d3a9:	48 8d 0d 70 a1 16 01 	lea    rcx,[rip+0x116a170]        # 0x181297520
   18012d3b0:	e8 4f aa 45 00       	call   0x180587e04
   18012d3b5:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012d3b8:	48 81 c1 18 04 00 00 	add    rcx,0x418
   18012d3bf:	e8 dc 5f ee ff       	call   0x1800133a0
   18012d3c4:	48 85 c0             	test   rax,rax
   18012d3c7:	74 0e                	je     0x18012d3d7
   18012d3c9:	48 8b c8             	mov    rcx,rax
   18012d3cc:	ff 15 5e 29 4b 00    	call   QWORD PTR [rip+0x4b295e]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   18012d3d2:	48 8b d8             	mov    rbx,rax
   18012d3d5:	eb 03                	jmp    0x18012d3da
   18012d3d7:	49 8b dc             	mov    rbx,r12
   18012d3da:	48 8d 0d 47 a1 16 01 	lea    rcx,[rip+0x116a147]        # 0x181297528
   18012d3e1:	e8 6a 39 ee ff       	call   0x180010d50
   18012d3e6:	48 8b c8             	mov    rcx,rax
   18012d3e9:	e8 a2 38 ee ff       	call   0x180010c90
   18012d3ee:	8b c8                	mov    ecx,eax
   18012d3f0:	ff 15 92 28 4b 00    	call   QWORD PTR [rip+0x4b2892]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   18012d3f6:	8b d0                	mov    edx,eax
   18012d3f8:	4c 8b c3             	mov    r8,rbx
   18012d3fb:	48 8d 4d b0          	lea    rcx,[rbp-0x50]
   18012d3ff:	ff 15 7b 28 4b 00    	call   QWORD PTR [rip+0x4b287b]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   18012d405:	48 8d 0d 1c a1 16 01 	lea    rcx,[rip+0x116a11c]        # 0x181297528
   18012d40c:	e8 3f 39 ee ff       	call   0x180010d50
   18012d411:	48 8b c8             	mov    rcx,rax
   18012d414:	e8 77 38 ee ff       	call   0x180010c90
   18012d419:	8b c8                	mov    ecx,eax
   18012d41b:	45 33 c0             	xor    r8d,r8d
   18012d41e:	48 8d 55 b0          	lea    rdx,[rbp-0x50]
   18012d422:	ff 15 68 2b 4b 00    	call   QWORD PTR [rip+0x4b2b68]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   18012d428:	48 8d 0d f9 a0 16 01 	lea    rcx,[rip+0x116a0f9]        # 0x181297528
   18012d42f:	e8 1c 39 ee ff       	call   0x180010d50
   18012d434:	48 8b c8             	mov    rcx,rax
   18012d437:	e8 54 38 ee ff       	call   0x180010c90
   18012d43c:	8b c8                	mov    ecx,eax
   18012d43e:	ff 15 44 28 4b 00    	call   QWORD PTR [rip+0x4b2844]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   18012d444:	8b d0                	mov    edx,eax
   18012d446:	4c 8b c3             	mov    r8,rbx
   18012d449:	48 8d 0d e8 a0 16 01 	lea    rcx,[rip+0x116a0e8]        # 0x181297538
   18012d450:	ff 15 3a 28 4b 00    	call   QWORD PTR [rip+0x4b283a]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   18012d456:	8b 04 3e             	mov    eax,DWORD PTR [rsi+rdi*1]
   18012d459:	39 05 e9 a0 16 01    	cmp    DWORD PTR [rip+0x116a0e9],eax        # 0x181297548
   18012d45f:	7e 41                	jle    0x18012d4a2
   18012d461:	48 8d 0d e0 a0 16 01 	lea    rcx,[rip+0x116a0e0]        # 0x181297548
   18012d468:	e8 f7 a9 45 00       	call   0x180587e64
   18012d46d:	83 3d d4 a0 16 01 ff 	cmp    DWORD PTR [rip+0x116a0d4],0xffffffff        # 0x181297548
   18012d474:	75 2c                	jne    0x18012d4a2
   18012d476:	48 8d 15 6b 30 50 00 	lea    rdx,[rip+0x50306b]        # 0x1806304e8 ; 'g_rwtRTShadowResult'
   18012d47d:	48 8d 0d cc a0 16 01 	lea    rcx,[rip+0x116a0cc]        # 0x181297550
   18012d484:	e8 67 5c ee ff       	call   0x1800130f0
   18012d489:	48 8d 0d 10 0d 4a 00 	lea    rcx,[rip+0x4a0d10]        # 0x1805ce1a0
   18012d490:	e8 2f a7 45 00       	call   0x180587bc4
   18012d495:	90                   	nop
   18012d496:	48 8d 0d ab a0 16 01 	lea    rcx,[rip+0x116a0ab]        # 0x181297548
   18012d49d:	e8 62 a9 45 00       	call   0x180587e04
   18012d4a2:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012d4a5:	48 81 c1 88 03 00 00 	add    rcx,0x388
   18012d4ac:	e8 ef 5e ee ff       	call   0x1800133a0
   18012d4b1:	48 85 c0             	test   rax,rax
   18012d4b4:	74 0e                	je     0x18012d4c4
   18012d4b6:	48 8b c8             	mov    rcx,rax
   18012d4b9:	ff 15 71 28 4b 00    	call   QWORD PTR [rip+0x4b2871]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   18012d4bf:	48 8b d8             	mov    rbx,rax
   18012d4c2:	eb 03                	jmp    0x18012d4c7
   18012d4c4:	49 8b dc             	mov    rbx,r12
   18012d4c7:	48 8d 0d 82 a0 16 01 	lea    rcx,[rip+0x116a082]        # 0x181297550
   18012d4ce:	e8 7d 38 ee ff       	call   0x180010d50
   18012d4d3:	48 8b c8             	mov    rcx,rax
   18012d4d6:	e8 b5 37 ee ff       	call   0x180010c90
   18012d4db:	8b c8                	mov    ecx,eax
   18012d4dd:	ff 15 a5 27 4b 00    	call   QWORD PTR [rip+0x4b27a5]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   18012d4e3:	8b d0                	mov    edx,eax
   18012d4e5:	4c 8b c3             	mov    r8,rbx
   18012d4e8:	48 8d 4d b0          	lea    rcx,[rbp-0x50]
   18012d4ec:	ff 15 8e 27 4b 00    	call   QWORD PTR [rip+0x4b278e]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   18012d4f2:	48 8d 0d 57 a0 16 01 	lea    rcx,[rip+0x116a057]        # 0x181297550
   18012d4f9:	e8 52 38 ee ff       	call   0x180010d50
   18012d4fe:	48 8b c8             	mov    rcx,rax
   18012d501:	e8 8a 37 ee ff       	call   0x180010c90
   18012d506:	8b c8                	mov    ecx,eax
   18012d508:	45 33 c0             	xor    r8d,r8d
   18012d50b:	48 8d 55 b0          	lea    rdx,[rbp-0x50]
   18012d50f:	ff 15 7b 2a 4b 00    	call   QWORD PTR [rip+0x4b2a7b]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   18012d515:	48 8d 0d 34 a0 16 01 	lea    rcx,[rip+0x116a034]        # 0x181297550
   18012d51c:	e8 2f 38 ee ff       	call   0x180010d50
   18012d521:	48 8b c8             	mov    rcx,rax
   18012d524:	e8 67 37 ee ff       	call   0x180010c90
   18012d529:	8b c8                	mov    ecx,eax
   18012d52b:	ff 15 57 27 4b 00    	call   QWORD PTR [rip+0x4b2757]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   18012d531:	8b d0                	mov    edx,eax
   18012d533:	4c 8b c3             	mov    r8,rbx
   18012d536:	48 8d 0d 23 a0 16 01 	lea    rcx,[rip+0x116a023]        # 0x181297560
   18012d53d:	ff 15 4d 27 4b 00    	call   QWORD PTR [rip+0x4b274d]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   18012d543:	48 8b 0d 76 03 16 01 	mov    rcx,QWORD PTR [rip+0x1160376]        # 0x18128d8c0
   18012d54a:	48 85 c9             	test   rcx,rcx
   18012d54d:	75 3f                	jne    0x18012d58e
   18012d54f:	48 8d 0d da 2f 50 00 	lea    rcx,[rip+0x502fda]        # 0x180630530 ; 'rt_sunshadow_copy.rfx'
   18012d556:	e8 c5 59 0a 00       	call   0x1801d2f20
   18012d55b:	48 8b c8             	mov    rcx,rax
   18012d55e:	48 8d 15 b3 2f 50 00 	lea    rdx,[rip+0x502fb3]        # 0x180630518 ; 'rt_SunShadow_ResultCopy'
   18012d565:	e8 76 57 0a 00       	call   0x1801d2ce0
   18012d56a:	48 8b c8             	mov    rcx,rax
   18012d56d:	33 d2                	xor    edx,edx
   18012d56f:	e8 7c fe 0a 00       	call   0x1801dd3f0
   18012d574:	48 89 05 45 03 16 01 	mov    QWORD PTR [rip+0x1160345],rax        # 0x18128d8c0
   18012d57b:	48 8d 0d 3e 03 16 01 	lea    rcx,[rip+0x116033e]        # 0x18128d8c0
   18012d582:	e8 79 b1 0a 00       	call   0x1801d8700
   18012d587:	48 8b 0d 32 03 16 01 	mov    rcx,QWORD PTR [rip+0x1160332]        # 0x18128d8c0
   18012d58e:	e8 bd e1 0a 00       	call   0x1801db750
   18012d593:	8b 05 f3 0e 7e 00    	mov    eax,DWORD PTR [rip+0x7e0ef3]        # 0x18090e48c
   18012d599:	83 c0 0f             	add    eax,0xf
   18012d59c:	99                   	cdq
   18012d59d:	83 e2 0f             	and    edx,0xf
   18012d5a0:	8d 0c 02             	lea    ecx,[rdx+rax*1]
   18012d5a3:	c1 f9 04             	sar    ecx,0x4
   18012d5a6:	8b 05 dc 0e 7e 00    	mov    eax,DWORD PTR [rip+0x7e0edc]        # 0x18090e488
   18012d5ac:	83 c0 0f             	add    eax,0xf
   18012d5af:	99                   	cdq
   18012d5b0:	83 e2 0f             	and    edx,0xf
   18012d5b3:	03 c2                	add    eax,edx
   18012d5b5:	c1 f8 04             	sar    eax,0x4
   18012d5b8:	89 45 d0             	mov    DWORD PTR [rbp-0x30],eax
   18012d5bb:	89 4d d4             	mov    DWORD PTR [rbp-0x2c],ecx
   18012d5be:	f2 0f 10 45 d0       	movsd  xmm0,QWORD PTR [rbp-0x30]
   18012d5c3:	f2 0f 11 45 d0       	movsd  QWORD PTR [rbp-0x30],xmm0
   18012d5c8:	c7 45 d8 01 00 00 00 	mov    DWORD PTR [rbp-0x28],0x1
   18012d5cf:	48 8d 55 d0          	lea    rdx,[rbp-0x30]
   18012d5d3:	49 8b cd             	mov    rcx,r13
   18012d5d6:	ff 15 e4 26 4b 00    	call   QWORD PTR [rip+0x4b26e4]        # 0x1805dfcc0 ; ?dispatch@DeviceUtil@d3d@@SAXAEAVDeviceState@2@V?$Vector3Template@H@m@@@Z
   18012d5dc:	41 b4 01             	mov    r12b,0x1
   18012d5df:	eb 1f                	jmp    0x18012d600
   18012d5e1:	80 3d e8 02 16 01 00 	cmp    BYTE PTR [rip+0x11602e8],0x0        # 0x18128d8d0
   18012d5e8:	74 13                	je     0x18012d5fd
   18012d5ea:	0f b6 15 c3 9e 16 01 	movzx  edx,BYTE PTR [rip+0x1169ec3]        # 0x1812974b4
   18012d5f1:	48 8d 0d d8 e8 95 00 	lea    rcx,[rip+0x95e8d8]        # 0x180a8bed0
   18012d5f8:	e8 13 67 ee ff       	call   0x180013d10
   18012d5fd:	45 32 e4             	xor    r12b,r12b
   18012d600:	44 88 35 c9 02 16 01 	mov    BYTE PTR [rip+0x11602c9],r14b        # 0x18128d8d0
   18012d607:	80 3d fc 53 6d 00 00 	cmp    BYTE PTR [rip+0x6d53fc],0x0        # 0x180802a0a
   18012d60e:	0f 84 ac 02 00 00    	je     0x18012d8c0
   18012d614:	48 8d 0d f5 5c 7e 00 	lea    rcx,[rip+0x7e5cf5]        # 0x180913310
   18012d61b:	e8 00 67 ee ff       	call   0x180013d20
   18012d620:	84 c0                	test   al,al
   18012d622:	0f 84 98 02 00 00    	je     0x18012d8c0
   18012d628:	8b 04 3e             	mov    eax,DWORD PTR [rsi+rdi*1]
   18012d62b:	39 05 3f 9f 16 01    	cmp    DWORD PTR [rip+0x1169f3f],eax        # 0x181297570
   18012d631:	7e 41                	jle    0x18012d674
   18012d633:	48 8d 0d 36 9f 16 01 	lea    rcx,[rip+0x1169f36]        # 0x181297570
   18012d63a:	e8 25 a8 45 00       	call   0x180587e64
   18012d63f:	83 3d 2a 9f 16 01 ff 	cmp    DWORD PTR [rip+0x1169f2a],0xffffffff        # 0x181297570
   18012d646:	75 2c                	jne    0x18012d674
   18012d648:	48 8d 15 09 2f 50 00 	lea    rdx,[rip+0x502f09]        # 0x180630558 ; 'g_rwtAOTarget'
   18012d64f:	48 8d 0d 22 9f 16 01 	lea    rcx,[rip+0x1169f22]        # 0x181297578
   18012d656:	e8 95 5a ee ff       	call   0x1800130f0
   18012d65b:	48 8d 0d 0e 0b 4a 00 	lea    rcx,[rip+0x4a0b0e]        # 0x1805ce170
   18012d662:	e8 5d a5 45 00       	call   0x180587bc4
   18012d667:	90                   	nop
   18012d668:	48 8d 0d 01 9f 16 01 	lea    rcx,[rip+0x1169f01]        # 0x181297570
   18012d66f:	e8 90 a7 45 00       	call   0x180587e04
   18012d674:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012d677:	48 81 c1 d8 00 00 00 	add    rcx,0xd8
   18012d67e:	e8 1d 5d ee ff       	call   0x1800133a0
   18012d683:	48 8b c8             	mov    rcx,rax
   18012d686:	e8 b5 87 11 00       	call   0x180245e40
   18012d68b:	48 85 c0             	test   rax,rax
   18012d68e:	74 0e                	je     0x18012d69e
   18012d690:	48 8b c8             	mov    rcx,rax
   18012d693:	ff 15 97 26 4b 00    	call   QWORD PTR [rip+0x4b2697]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   18012d699:	48 8b d8             	mov    rbx,rax
   18012d69c:	eb 02                	jmp    0x18012d6a0
   18012d69e:	33 db                	xor    ebx,ebx
   18012d6a0:	48 8d 0d d1 9e 16 01 	lea    rcx,[rip+0x1169ed1]        # 0x181297578
   18012d6a7:	e8 a4 36 ee ff       	call   0x180010d50
   18012d6ac:	48 8b c8             	mov    rcx,rax
   18012d6af:	e8 dc 35 ee ff       	call   0x180010c90
   18012d6b4:	8b c8                	mov    ecx,eax
   18012d6b6:	ff 15 cc 25 4b 00    	call   QWORD PTR [rip+0x4b25cc]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   18012d6bc:	8b d0                	mov    edx,eax
   18012d6be:	4c 8b c3             	mov    r8,rbx
   18012d6c1:	48 8d 4d b0          	lea    rcx,[rbp-0x50]
   18012d6c5:	ff 15 b5 25 4b 00    	call   QWORD PTR [rip+0x4b25b5]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   18012d6cb:	48 8d 0d a6 9e 16 01 	lea    rcx,[rip+0x1169ea6]        # 0x181297578
   18012d6d2:	e8 79 36 ee ff       	call   0x180010d50
   18012d6d7:	48 8b c8             	mov    rcx,rax
   18012d6da:	e8 b1 35 ee ff       	call   0x180010c90
   18012d6df:	8b c8                	mov    ecx,eax
   18012d6e1:	45 33 c0             	xor    r8d,r8d
   18012d6e4:	48 8d 55 b0          	lea    rdx,[rbp-0x50]
   18012d6e8:	ff 15 a2 28 4b 00    	call   QWORD PTR [rip+0x4b28a2]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   18012d6ee:	48 8d 0d 83 9e 16 01 	lea    rcx,[rip+0x1169e83]        # 0x181297578
   18012d6f5:	e8 56 36 ee ff       	call   0x180010d50
   18012d6fa:	48 8b c8             	mov    rcx,rax
   18012d6fd:	e8 8e 35 ee ff       	call   0x180010c90
   18012d702:	8b c8                	mov    ecx,eax
   18012d704:	ff 15 7e 25 4b 00    	call   QWORD PTR [rip+0x4b257e]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   18012d70a:	8b d0                	mov    edx,eax
   18012d70c:	4c 8b c3             	mov    r8,rbx
   18012d70f:	48 8d 0d 72 9e 16 01 	lea    rcx,[rip+0x1169e72]        # 0x181297588
   18012d716:	ff 15 74 25 4b 00    	call   QWORD PTR [rip+0x4b2574]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   18012d71c:	8b 04 3e             	mov    eax,DWORD PTR [rsi+rdi*1]
   18012d71f:	39 05 73 9e 16 01    	cmp    DWORD PTR [rip+0x1169e73],eax        # 0x181297598
   18012d725:	7e 41                	jle    0x18012d768
   18012d727:	48 8d 0d 6a 9e 16 01 	lea    rcx,[rip+0x1169e6a]        # 0x181297598
   18012d72e:	e8 31 a7 45 00       	call   0x180587e64
   18012d733:	83 3d 5e 9e 16 01 ff 	cmp    DWORD PTR [rip+0x1169e5e],0xffffffff        # 0x181297598
   18012d73a:	75 2c                	jne    0x18012d768
   18012d73c:	48 8d 15 05 2e 50 00 	lea    rdx,[rip+0x502e05]        # 0x180630548 ; 'g_uRTAORayCount'
   18012d743:	48 8d 0d 56 9e 16 01 	lea    rcx,[rip+0x1169e56]        # 0x1812975a0
   18012d74a:	e8 01 40 f0 ff       	call   0x180031750
   18012d74f:	48 8d 0d ea 09 4a 00 	lea    rcx,[rip+0x4a09ea]        # 0x1805ce140
   18012d756:	e8 69 a4 45 00       	call   0x180587bc4
   18012d75b:	90                   	nop
   18012d75c:	48 8d 0d 35 9e 16 01 	lea    rcx,[rip+0x1169e35]        # 0x181297598
   18012d763:	e8 9c a6 45 00       	call   0x180587e04
   18012d768:	48 8d 0d 31 65 7e 00 	lea    rcx,[rip+0x7e6531]        # 0x180913ca0
   18012d76f:	e8 4c 03 f0 ff       	call   0x18002dac0
   18012d774:	89 85 f0 03 00 00    	mov    DWORD PTR [rbp+0x3f0],eax
   18012d77a:	48 8d 0d 1f 9e 16 01 	lea    rcx,[rip+0x1169e1f]        # 0x1812975a0
   18012d781:	e8 ca 35 ee ff       	call   0x180010d50
   18012d786:	48 8b c8             	mov    rcx,rax
   18012d789:	e8 02 35 ee ff       	call   0x180010c90
   18012d78e:	8b c8                	mov    ecx,eax
   18012d790:	45 33 c0             	xor    r8d,r8d
   18012d793:	48 8d 95 f0 03 00 00 	lea    rdx,[rbp+0x3f0]
   18012d79a:	ff 15 f0 27 4b 00    	call   QWORD PTR [rip+0x4b27f0]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   18012d7a0:	8b 85 f0 03 00 00    	mov    eax,DWORD PTR [rbp+0x3f0]
   18012d7a6:	89 05 04 9e 16 01    	mov    DWORD PTR [rip+0x1169e04],eax        # 0x1812975b0
   18012d7ac:	bf 02 00 00 00       	mov    edi,0x2
   18012d7b1:	8b d7                	mov    edx,edi
   18012d7b3:	8d 4f ff             	lea    ecx,[rdi-0x1]
   18012d7b6:	ff 15 e4 1c 4b 00    	call   QWORD PTR [rip+0x4b1ce4]        # 0x1805df4a0 ; ?beginPipelineSetup@DeviceUtilRaytracing@d3d@@SAXHH@Z
   18012d7bc:	48 8b 0d f5 00 16 01 	mov    rcx,QWORD PTR [rip+0x11600f5]        # 0x18128d8b8
   18012d7c3:	48 85 c9             	test   rcx,rcx
   18012d7c6:	75 3f                	jne    0x18012d807
   18012d7c8:	48 8d 0d a1 2d 50 00 	lea    rcx,[rip+0x502da1]        # 0x180630570 ; 'rt_ao.rfx'
   18012d7cf:	e8 4c 57 0a 00       	call   0x1801d2f20
   18012d7d4:	48 8b c8             	mov    rcx,rax
   18012d7d7:	48 8d 15 8a 2d 50 00 	lea    rdx,[rip+0x502d8a]        # 0x180630568 ; 'rt_AO'
   18012d7de:	e8 fd 54 0a 00       	call   0x1801d2ce0
   18012d7e3:	48 8b c8             	mov    rcx,rax
   18012d7e6:	33 d2                	xor    edx,edx
   18012d7e8:	e8 03 fc 0a 00       	call   0x1801dd3f0
   18012d7ed:	48 89 05 c4 00 16 01 	mov    QWORD PTR [rip+0x11600c4],rax        # 0x18128d8b8
   18012d7f4:	48 8d 0d bd 00 16 01 	lea    rcx,[rip+0x11600bd]        # 0x18128d8b8
   18012d7fb:	e8 00 af 0a 00       	call   0x1801d8700
   18012d800:	48 8b 0d b1 00 16 01 	mov    rcx,QWORD PTR [rip+0x11600b1]        # 0x18128d8b8
   18012d807:	49 8b d5             	mov    rdx,r13
   18012d80a:	e8 01 e0 0a 00       	call   0x1801db810
   18012d80f:	48 8d 0d 22 2b 50 00 	lea    rcx,[rip+0x502b22]        # 0x180630338 ; 'rayGenMain'
   18012d816:	ff 15 8c 1c 4b 00    	call   QWORD PTR [rip+0x4b1c8c]        # 0x1805df4a8 ; ?setRayGeneration@DeviceUtilRaytracing@d3d@@SAXPEBD@Z
   18012d81c:	48 8d 15 05 2b 50 00 	lea    rdx,[rip+0x502b05]        # 0x180630328 ; 'missMain'
   18012d823:	33 c9                	xor    ecx,ecx
   18012d825:	ff 15 85 1c 4b 00    	call   QWORD PTR [rip+0x4b1c85]        # 0x1805df4b0 ; ?setMiss@DeviceUtilRaytracing@d3d@@SAXHPEBD@Z
   18012d82b:	48 8d 05 26 2b 50 00 	lea    rax,[rip+0x502b26]        # 0x180630358 ; 'closestHitMain'
   18012d832:	48 89 44 24 20       	mov    QWORD PTR [rsp+0x20],rax
   18012d837:	45 33 c9             	xor    r9d,r9d
   18012d83a:	45 33 c0             	xor    r8d,r8d
   18012d83d:	33 d2                	xor    edx,edx
   18012d83f:	33 c9                	xor    ecx,ecx
   18012d841:	ff 15 71 1c 4b 00    	call   QWORD PTR [rip+0x4b1c71]        # 0x1805df4b8 ; ?setHitGroup@DeviceUtilRaytracing@d3d@@SAXHHPEBD00@Z
   18012d847:	48 c7 44 24 20 00 00 	mov    QWORD PTR [rsp+0x20],0x0
   18012d84e:	00 00 
   18012d850:	4c 8d 0d f1 2a 50 00 	lea    r9,[rip+0x502af1]        # 0x180630348 ; 'anyHitAlphaTest'
   18012d857:	45 33 c0             	xor    r8d,r8d
   18012d85a:	41 8d 50 01          	lea    edx,[r8+0x1]
   18012d85e:	33 c9                	xor    ecx,ecx
   18012d860:	ff 15 52 1c 4b 00    	call   QWORD PTR [rip+0x4b1c52]        # 0x1805df4b8 ; ?setHitGroup@DeviceUtilRaytracing@d3d@@SAXHHPEBD00@Z
   18012d866:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012d869:	48 81 c1 20 02 00 00 	add    rcx,0x220
   18012d870:	e8 2b 5b ee ff       	call   0x1800133a0
   18012d875:	48 8b c8             	mov    rcx,rax
   18012d878:	e8 23 a9 f7 ff       	call   0x1800a81a0
   18012d87d:	8b d8                	mov    ebx,eax
   18012d87f:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012d882:	48 81 c1 20 02 00 00 	add    rcx,0x220
   18012d889:	e8 12 5b ee ff       	call   0x1800133a0
   18012d88e:	48 8b c8             	mov    rcx,rax
   18012d891:	e8 fa a8 f7 ff       	call   0x1800a8190
   18012d896:	4c 8b c8             	mov    r9,rax
   18012d899:	89 5c 24 20          	mov    DWORD PTR [rsp+0x20],ebx
   18012d89d:	44 8b c7             	mov    r8d,edi
   18012d8a0:	ba 04 00 00 00       	mov    edx,0x4
   18012d8a5:	8d 4a fd             	lea    ecx,[rdx-0x3]
   18012d8a8:	ff 15 12 1c 4b 00    	call   QWORD PTR [rip+0x4b1c12]        # 0x1805df4c0 ; ?endPipelineSetup@DeviceUtilRaytracing@d3d@@SAXHHHPEBURaytracingGeometryBinding@2@H@Z
   18012d8ae:	8b 15 d8 0b 7e 00    	mov    edx,DWORD PTR [rip+0x7e0bd8]        # 0x18090e48c
   18012d8b4:	8b 0d ce 0b 7e 00    	mov    ecx,DWORD PTR [rip+0x7e0bce]        # 0x18090e488
   18012d8ba:	ff 15 08 1c 4b 00    	call   QWORD PTR [rip+0x4b1c08]        # 0x1805df4c8 ; ?raytrace@DeviceUtilRaytracing@d3d@@SAXHH@Z
   18012d8c0:	45 84 e4             	test   r12b,r12b
   18012d8c3:	0f 85 b4 00 00 00    	jne    0x18012d97d
   18012d8c9:	49 8b 47 08          	mov    rax,QWORD PTR [r15+0x8]
   18012d8cd:	48 8b 08             	mov    rcx,QWORD PTR [rax]
   18012d8d0:	48 85 c9             	test   rcx,rcx
   18012d8d3:	0f 84 a4 00 00 00    	je     0x18012d97d
   18012d8d9:	48 8d 95 e8 02 00 00 	lea    rdx,[rbp+0x2e8]
   18012d8e0:	e8 6b 08 06 00       	call   0x18018e150
   18012d8e5:	f3 0f 10 10          	movss  xmm2,DWORD PTR [rax]
   18012d8e9:	f3 0f 10 40 04       	movss  xmm0,DWORD PTR [rax+0x4]
   18012d8ee:	f3 0f 5f d0          	maxss  xmm2,xmm0
   18012d8f2:	0f 28 c2             	movaps xmm0,xmm2
   18012d8f5:	0f 28 d0             	movaps xmm2,xmm0
   18012d8f8:	f3 0f 5f 50 08       	maxss  xmm2,DWORD PTR [rax+0x8]
   18012d8fd:	0f 2f 15 f4 13 55 00 	comiss xmm2,DWORD PTR [rip+0x5513f4]        # 0x18067ecf8
   18012d904:	76 77                	jbe    0x18012d97d
   18012d906:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012d909:	48 81 c1 88 03 00 00 	add    rcx,0x388
   18012d910:	e8 8b 5a ee ff       	call   0x1800133a0
   18012d915:	48 8b d0             	mov    rdx,rax
   18012d918:	45 33 c0             	xor    r8d,r8d
   18012d91b:	48 8d 4d e0          	lea    rcx,[rbp-0x20]
   18012d91f:	ff 15 73 26 4b 00    	call   QWORD PTR [rip+0x4b2673]        # 0x1805dff98 ; ??0RenderToTexture@d3d@@QEAA@AEAVTexture2D@1@PEBVNativeTexture@1@@Z
   18012d925:	90                   	nop
   18012d926:	45 33 c0             	xor    r8d,r8d
   18012d929:	33 d2                	xor    edx,edx
   18012d92b:	48 8d 4d e0          	lea    rcx,[rbp-0x20]
   18012d92f:	ff 15 3b 24 4b 00    	call   QWORD PTR [rip+0x4b243b]        # 0x1805dfd70 ; ?begin@RenderToTexture@d3d@@QEAAXHH@Z
   18012d935:	49 8b 7f 08          	mov    rdi,QWORD PTR [r15+0x8]
   18012d939:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012d93c:	48 81 c1 88 03 00 00 	add    rcx,0x388
   18012d943:	e8 58 5a ee ff       	call   0x1800133a0
   18012d948:	48 8b d8             	mov    rbx,rax
   18012d94b:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012d94e:	48 81 c1 b0 00 00 00 	add    rcx,0xb0
   18012d955:	e8 46 5a ee ff       	call   0x1800133a0
   18012d95a:	48 8b c8             	mov    rcx,rax
   18012d95d:	4c 8b 07             	mov    r8,QWORD PTR [rdi]
   18012d960:	48 8b d3             	mov    rdx,rbx
   18012d963:	e8 48 4c 11 00       	call   0x1802425b0
   18012d968:	48 8d 4d e0          	lea    rcx,[rbp-0x20]
   18012d96c:	ff 15 06 24 4b 00    	call   QWORD PTR [rip+0x4b2406]        # 0x1805dfd78 ; ?end@RenderToTexture@d3d@@QEAAXXZ
   18012d972:	90                   	nop
   18012d973:	48 8d 4d e0          	lea    rcx,[rbp-0x20]
   18012d977:	ff 15 eb 23 4b 00    	call   QWORD PTR [rip+0x4b23eb]        # 0x1805dfd68 ; ??1RenderToTexture@d3d@@QEAA@XZ
   18012d97d:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012d980:	48 81 c1 88 03 00 00 	add    rcx,0x388
   18012d987:	e8 14 5a ee ff       	call   0x1800133a0
   18012d98c:	48 8b f8             	mov    rdi,rax
   18012d98f:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012d992:	48 81 c1 90 03 00 00 	add    rcx,0x390
   18012d999:	e8 02 5a ee ff       	call   0x1800133a0
   18012d99e:	48 8b d8             	mov    rbx,rax
   18012d9a1:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012d9a4:	48 81 c1 d8 00 00 00 	add    rcx,0xd8
   18012d9ab:	e8 f0 59 ee ff       	call   0x1800133a0
   18012d9b0:	48 8b c8             	mov    rcx,rax
   18012d9b3:	4c 8b c7             	mov    r8,rdi
   18012d9b6:	48 8b d3             	mov    rdx,rbx
   18012d9b9:	e8 82 af 11 00       	call   0x180248940
   18012d9be:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012d9c1:	48 8d 91 90 03 00 00 	lea    rdx,[rcx+0x390]
   18012d9c8:	48 81 c1 88 03 00 00 	add    rcx,0x388
   18012d9cf:	e8 ac a6 ee ff       	call   0x180018080
   18012d9d4:	49 8b 0f             	mov    rcx,QWORD PTR [r15]
   18012d9d7:	48 81 c1 90 03 00 00 	add    rcx,0x390
   18012d9de:	33 d2                	xor    edx,edx
   18012d9e0:	e8 ab a6 ee ff       	call   0x180018090
   18012d9e5:	4c 8d 9c 24 b0 04 00 	lea    r11,[rsp+0x4b0]
   18012d9ec:	00 
   18012d9ed:	49 8b 5b 48          	mov    rbx,QWORD PTR [r11+0x48]
   18012d9f1:	41 0f 28 73 f0       	movaps xmm6,XMMWORD PTR [r11-0x10]
   18012d9f6:	41 0f 28 7b e0       	movaps xmm7,XMMWORD PTR [r11-0x20]
   18012d9fb:	45 0f 28 43 d0       	movaps xmm8,XMMWORD PTR [r11-0x30]
   18012da00:	45 0f 28 4b c0       	movaps xmm9,XMMWORD PTR [r11-0x40]
   18012da05:	45 0f 28 53 b0       	movaps xmm10,XMMWORD PTR [r11-0x50]
   18012da0a:	45 0f 28 5b a0       	movaps xmm11,XMMWORD PTR [r11-0x60]
   18012da0f:	45 0f 28 63 90       	movaps xmm12,XMMWORD PTR [r11-0x70]
   18012da14:	45 0f 28 6b 80       	movaps xmm13,XMMWORD PTR [r11-0x80]
   18012da19:	45 0f 28 b3 70 ff ff 	movaps xmm14,XMMWORD PTR [r11-0x90]
   18012da20:	ff 
   18012da21:	45 0f 28 bb 60 ff ff 	movaps xmm15,XMMWORD PTR [r11-0xa0]
   18012da28:	ff 
   18012da29:	49 8b e3             	mov    rsp,r11
   18012da2c:	41 5f                	pop    r15
   18012da2e:	41 5e                	pop    r14
   18012da30:	41 5d                	pop    r13
   18012da32:	41 5c                	pop    r12
   18012da34:	5f                   	pop    rdi
   18012da35:	5e                   	pop    rsi
   18012da36:	5d                   	pop    rbp
   18012da37:	c3                   	ret
