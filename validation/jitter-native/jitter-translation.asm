
upload/renderer_rmdwin10_f(4).dll:     file format pei-x86-64


Disassembly of section .text:

000000018011dc50 <.text+0x11cc50>:
   18011dc50:	48 8b c4             	mov    rax,rsp
   18011dc53:	4c 89 48 20          	mov    QWORD PTR [rax+0x20],r9
   18011dc57:	55                   	push   rbp
   18011dc58:	56                   	push   rsi
   18011dc59:	57                   	push   rdi
   18011dc5a:	41 54                	push   r12
   18011dc5c:	41 55                	push   r13
   18011dc5e:	41 56                	push   r14
   18011dc60:	41 57                	push   r15
   18011dc62:	48 8d a8 38 f6 ff ff 	lea    rbp,[rax-0x9c8]
   18011dc69:	48 81 ec 90 0a 00 00 	sub    rsp,0xa90
   18011dc70:	48 c7 44 24 50 fe ff 	mov    QWORD PTR [rsp+0x50],0xfffffffffffffffe
   18011dc77:	ff ff 
   18011dc79:	48 89 58 10          	mov    QWORD PTR [rax+0x10],rbx
   18011dc7d:	0f 29 70 b8          	movaps XMMWORD PTR [rax-0x48],xmm6
   18011dc81:	0f 29 78 a8          	movaps XMMWORD PTR [rax-0x58],xmm7
   18011dc85:	44 0f 29 40 98       	movaps XMMWORD PTR [rax-0x68],xmm8
   18011dc8a:	4d 8b e1             	mov    r12,r9
   18011dc8d:	4d 8b e8             	mov    r13,r8
   18011dc90:	4c 8b fa             	mov    r15,rdx
   18011dc93:	48 8b f9             	mov    rdi,rcx
   18011dc96:	8b 81 d8 06 00 00    	mov    eax,DWORD PTR [rcx+0x6d8]
   18011dc9c:	4c 8b 11             	mov    r10,QWORD PTR [rcx]
   18011dc9f:	c6 44 24 40 00       	mov    BYTE PTR [rsp+0x40],0x0
   18011dca4:	89 44 24 38          	mov    DWORD PTR [rsp+0x38],eax
   18011dca8:	8b 05 5e 06 7f 00    	mov    eax,DWORD PTR [rip+0x7f065e]        # 0x18090e30c
   18011dcae:	89 44 24 30          	mov    DWORD PTR [rsp+0x30],eax
   18011dcb2:	8b 05 50 06 7f 00    	mov    eax,DWORD PTR [rip+0x7f0650]        # 0x18090e308
   18011dcb8:	89 44 24 28          	mov    DWORD PTR [rsp+0x28],eax
   18011dcbc:	8b 05 8a 05 7f 00    	mov    eax,DWORD PTR [rip+0x7f058a]        # 0x18090e24c
   18011dcc2:	89 44 24 20          	mov    DWORD PTR [rsp+0x20],eax
   18011dcc6:	44 8b 0d 7b 05 7f 00 	mov    r9d,DWORD PTR [rip+0x7f057b]        # 0x18090e248
   18011dccd:	44 8b 05 b8 04 7f 00 	mov    r8d,DWORD PTR [rip+0x7f04b8]        # 0x18090e18c
   18011dcd4:	8b 15 ae 04 7f 00    	mov    edx,DWORD PTR [rip+0x7f04ae]        # 0x18090e188
   18011dcda:	41 ff 52 10          	call   QWORD PTR [r10+0x10]
   18011dcde:	0f b6 0d 74 e4 6c 00 	movzx  ecx,BYTE PTR [rip+0x6ce474]        # 0x1807ec159
   18011dce5:	88 0d 07 e6 6c 00    	mov    BYTE PTR [rip+0x6ce607],cl        # 0x1807ec2f2
   18011dceb:	41 80 bf b0 04 00 00 	cmp    BYTE PTR [r15+0x4b0],0x0
   18011dcf2:	00 
   18011dcf3:	0f 94 c0             	sete   al
   18011dcf6:	08 05 27 4c 6e 00    	or     BYTE PTR [rip+0x6e4c27],al        # 0x180802923
   18011dcfc:	ba 40 00 00 00       	mov    edx,0x40
   18011dd01:	8b f2                	mov    esi,edx
   18011dd03:	84 c9                	test   cl,cl
   18011dd05:	0f 84 2d 06 00 00    	je     0x18011e338
   18011dd0b:	8b 0d bf 4c 6e 00    	mov    ecx,DWORD PTR [rip+0x6e4cbf]        # 0x1808029d0
   18011dd11:	8b c1                	mov    eax,ecx
   18011dd13:	83 e0 03             	and    eax,0x3
   18011dd16:	89 05 74 4c 6e 00    	mov    DWORD PTR [rip+0x6e4c74],eax        # 0x180802990
   18011dd1c:	44 8d 61 ff          	lea    r12d,[rcx-0x1]
   18011dd20:	41 83 e4 03          	and    r12d,0x3
   18011dd24:	8b 0d 12 4b 6e 00    	mov    ecx,DWORD PTR [rip+0x6e4b12]        # 0x18080283c
   18011dd2a:	65 48 8b 04 25 58 00 	mov    rax,QWORD PTR gs:0x58
   18011dd31:	00 00 
   18011dd33:	4c 8b 34 c8          	mov    r14,QWORD PTR [rax+rcx*8]
   18011dd37:	41 8b 04 16          	mov    eax,DWORD PTR [r14+rdx*1]
   18011dd3b:	39 05 e7 7e 17 01    	cmp    DWORD PTR [rip+0x1177ee7],eax        # 0x181295c28
   18011dd41:	7e 3d                	jle    0x18011dd80
   18011dd43:	48 8d 0d de 7e 17 01 	lea    rcx,[rip+0x1177ede]        # 0x181295c28
   18011dd4a:	e8 15 a1 46 00       	call   0x180587e64
   18011dd4f:	83 3d d2 7e 17 01 ff 	cmp    DWORD PTR [rip+0x1177ed2],0xffffffff        # 0x181295c28
   18011dd56:	75 28                	jne    0x18011dd80
   18011dd58:	0f 28 05 71 24 56 00 	movaps xmm0,XMMWORD PTR [rip+0x562471]        # 0x1806801d0
   18011dd5f:	0f 11 05 ca 7e 17 01 	movups XMMWORD PTR [rip+0x1177eca],xmm0        # 0x181295c30
   18011dd66:	0f 28 0d f3 2d 56 00 	movaps xmm1,XMMWORD PTR [rip+0x562df3]        # 0x180680b60
   18011dd6d:	0f 11 0d cc 7e 17 01 	movups XMMWORD PTR [rip+0x1177ecc],xmm1        # 0x181295c40
   18011dd74:	48 8d 0d ad 7e 17 01 	lea    rcx,[rip+0x1177ead]        # 0x181295c28
   18011dd7b:	e8 84 a0 46 00       	call   0x180587e04
   18011dd80:	0f 28 05 29 18 56 00 	movaps xmm0,XMMWORD PTR [rip+0x561829]        # 0x18067f5b0
   18011dd87:	0f 29 44 24 60       	movaps XMMWORD PTR [rsp+0x60],xmm0
   18011dd8c:	0f 57 c9             	xorps  xmm1,xmm1
   18011dd8f:	0f 29 4c 24 70       	movaps XMMWORD PTR [rsp+0x70],xmm1
   18011dd94:	0f 28 05 65 27 56 00 	movaps xmm0,XMMWORD PTR [rip+0x562765]        # 0x180680500
   18011dd9b:	0f 29 45 80          	movaps XMMWORD PTR [rbp-0x80],xmm0
   18011dd9f:	0f 29 4d 90          	movaps XMMWORD PTR [rbp-0x70],xmm1
   18011dda3:	0f 57 c0             	xorps  xmm0,xmm0
   18011dda6:	0f 29 45 a0          	movaps XMMWORD PTR [rbp-0x60],xmm0
   18011ddaa:	0f 28 0d ff 17 56 00 	movaps xmm1,XMMWORD PTR [rip+0x5617ff]        # 0x18067f5b0
   18011ddb1:	0f 29 4d b0          	movaps XMMWORD PTR [rbp-0x50],xmm1
   18011ddb5:	0f 28 05 44 27 56 00 	movaps xmm0,XMMWORD PTR [rip+0x562744]        # 0x180680500
   18011ddbc:	0f 29 45 d0          	movaps XMMWORD PTR [rbp-0x30],xmm0
   18011ddc0:	33 db                	xor    ebx,ebx
   18011ddc2:	38 1d 2b e5 6c 00    	cmp    BYTE PTR [rip+0x6ce52b],bl        # 0x1807ec2f3
   18011ddc8:	0f 84 a1 01 00 00    	je     0x18011df6f
   18011ddce:	f3 0f 10 3d fa 36 7f 	movss  xmm7,DWORD PTR [rip+0x7f36fa]        # 0x1809114d0
   18011ddd5:	00 
   18011ddd6:	38 1d 63 4c 6e 00    	cmp    BYTE PTR [rip+0x6e4c63],bl        # 0x180802a3f
   18011dddc:	0f 84 db 00 00 00    	je     0x18011debd
   18011dde2:	8b 05 24 05 7f 00    	mov    eax,DWORD PTR [rip+0x7f0524]        # 0x18090e30c
   18011dde8:	8b 0d 9e 06 7f 00    	mov    ecx,DWORD PTR [rip+0x7f069e]        # 0x18090e48c
   18011ddee:	38 1d f0 f3 16 01    	cmp    BYTE PTR [rip+0x116f3f0],bl        # 0x18128d1e4
   18011ddf4:	0f 45 c1             	cmovne eax,ecx
   18011ddf7:	66 0f 6e c0          	movd   xmm0,eax
   18011ddfb:	0f 5b c0             	cvtdq2ps xmm0,xmm0
   18011ddfe:	66 0f 6e c9          	movd   xmm1,ecx
   18011de02:	0f 5b c9             	cvtdq2ps xmm1,xmm1
   18011de05:	f3 0f 5e c1          	divss  xmm0,xmm1
   18011de09:	f3 0f 10 35 b3 12 56 	movss  xmm6,DWORD PTR [rip+0x5612b3]        # 0x18067f0c4
   18011de10:	00 
   18011de11:	0f 28 ce             	movaps xmm1,xmm6
   18011de14:	e8 eb b0 46 00       	call   0x180588f04
   18011de19:	f3 0f 59 05 07 15 56 	mulss  xmm0,DWORD PTR [rip+0x561507]        # 0x18067f328
   18011de20:	00 
   18011de21:	ff 15 51 14 4c 00    	call   QWORD PTR [rip+0x4c1451]        # 0x1805df278
   18011de27:	8b c8                	mov    ecx,eax
   18011de29:	b8 00 04 00 00       	mov    eax,0x400
   18011de2e:	38 1d ec 67 7f 00    	cmp    BYTE PTR [rip+0x7f67ec],bl        # 0x180914620
   18011de34:	0f 45 c8             	cmovne ecx,eax
   18011de37:	8b 05 93 4b 6e 00    	mov    eax,DWORD PTR [rip+0x6e4b93]        # 0x1808029d0
   18011de3d:	99                   	cdq
   18011de3e:	f7 f9                	idiv   ecx
   18011de40:	8b ca                	mov    ecx,edx
   18011de42:	8d 53 02             	lea    edx,[rbx+0x2]
   18011de45:	e8 b6 fd ff ff       	call   0x18011dc00
   18011de4a:	0f 28 e8             	movaps xmm5,xmm0
   18011de4d:	f3 0f 5c 2d 63 10 56 	subss  xmm5,DWORD PTR [rip+0x561063]        # 0x18067eeb8
   18011de54:	00 
   18011de55:	8d 53 03             	lea    edx,[rbx+0x3]
   18011de58:	e8 a3 fd ff ff       	call   0x18011dc00
   18011de5d:	f3 0f 5c 05 53 10 56 	subss  xmm0,DWORD PTR [rip+0x561053]        # 0x18067eeb8
   18011de64:	00 
   18011de65:	f3 0f 59 ef          	mulss  xmm5,xmm7
   18011de69:	66 0f 6e 0d 17 06 7f 	movd   xmm1,DWORD PTR [rip+0x7f0617]        # 0x18090e488
   18011de70:	00 
   18011de71:	0f 5b c9             	cvtdq2ps xmm1,xmm1
   18011de74:	f3 0f 5e e9          	divss  xmm5,xmm1
   18011de78:	f3 0f 59 ee          	mulss  xmm5,xmm6
   18011de7c:	0f 57 f6             	xorps  xmm6,xmm6
   18011de7f:	f3 0f 5a f5          	cvtss2sd xmm6,xmm5
   18011de83:	f3 0f 59 c7          	mulss  xmm0,xmm7
   18011de87:	66 0f 6e 0d fd 05 7f 	movd   xmm1,DWORD PTR [rip+0x7f05fd]        # 0x18090e48c
   18011de8e:	00 
   18011de8f:	0f 5b c9             	cvtdq2ps xmm1,xmm1
   18011de92:	f3 0f 5e c1          	divss  xmm0,xmm1
   18011de96:	f3 0f 59 05 4a 16 56 	mulss  xmm0,DWORD PTR [rip+0x56164a]        # 0x18067f4e8
   18011de9d:	00 
   18011de9e:	45 0f 57 c0          	xorps  xmm8,xmm8
   18011dea2:	f3 44 0f 5a c0       	cvtss2sd xmm8,xmm0
   18011dea7:	f2 0f 11 b7 80 04 00 	movsd  QWORD PTR [rdi+0x480],xmm6
   18011deae:	00 
   18011deaf:	f2 44 0f 11 87 88 04 	movsd  QWORD PTR [rdi+0x488],xmm8
   18011deb6:	00 00 
   18011deb8:	e9 e7 00 00 00       	jmp    0x18011dfa4
   18011debd:	39 1d cd 4a 6e 00    	cmp    DWORD PTR [rip+0x6e4acd],ebx        # 0x180802990
   18011dec3:	7d 04                	jge    0x18011dec9
   18011dec5:	8b c3                	mov    eax,ebx
   18011dec7:	eb 10                	jmp    0x18011ded9
   18011dec9:	8b 05 c1 4a 6e 00    	mov    eax,DWORD PTR [rip+0x6e4ac1]        # 0x180802990
   18011decf:	ba 03 00 00 00       	mov    edx,0x3
   18011ded4:	3b c2                	cmp    eax,edx
   18011ded6:	0f 4f c2             	cmovg  eax,edx
   18011ded9:	48 98                	cdqe
   18011dedb:	48 8d 0d 4e 7d 17 01 	lea    rcx,[rip+0x1177d4e]        # 0x181295c30
   18011dee2:	0f 28 cf             	movaps xmm1,xmm7
   18011dee5:	f3 0f 59 0c c1       	mulss  xmm1,DWORD PTR [rcx+rax*8]
   18011deea:	0f 57 c0             	xorps  xmm0,xmm0
   18011deed:	f3 0f 2a 05 93 05 7f 	cvtsi2ss xmm0,DWORD PTR [rip+0x7f0593]        # 0x18090e488
   18011def4:	00 
   18011def5:	f3 0f 5e c8          	divss  xmm1,xmm0
   18011def9:	0f 5a f1             	cvtps2pd xmm6,xmm1
   18011defc:	0f 28 cf             	movaps xmm1,xmm7
   18011deff:	f3 0f 59 4c c1 04    	mulss  xmm1,DWORD PTR [rcx+rax*8+0x4]
   18011df05:	0f 57 c0             	xorps  xmm0,xmm0
   18011df08:	f3 0f 2a 05 7c 05 7f 	cvtsi2ss xmm0,DWORD PTR [rip+0x7f057c]        # 0x18090e48c
   18011df0f:	00 
   18011df10:	f3 0f 5e c8          	divss  xmm1,xmm0
   18011df14:	44 0f 5a c1          	cvtps2pd xmm8,xmm1
   18011df18:	f2 0f 11 b7 80 04 00 	movsd  QWORD PTR [rdi+0x480],xmm6
   18011df1f:	00 
   18011df20:	f2 44 0f 11 87 88 04 	movsd  QWORD PTR [rdi+0x488],xmm8
   18011df27:	00 00 
   18011df29:	0f 28 cf             	movaps xmm1,xmm7
   18011df2c:	f3 42 0f 59 0c e1    	mulss  xmm1,DWORD PTR [rcx+r12*8]
   18011df32:	0f 57 c0             	xorps  xmm0,xmm0
   18011df35:	f3 0f 2a 05 4b 05 7f 	cvtsi2ss xmm0,DWORD PTR [rip+0x7f054b]        # 0x18090e488
   18011df3c:	00 
   18011df3d:	f3 0f 5e c8          	divss  xmm1,xmm0
   18011df41:	0f 5a c1             	cvtps2pd xmm0,xmm1
   18011df44:	f2 0f 11 05 34 e4 6c 	movsd  QWORD PTR [rip+0x6ce434],xmm0        # 0x1807ec380
   18011df4b:	00 
   18011df4c:	f3 42 0f 59 7c e1 04 	mulss  xmm7,DWORD PTR [rcx+r12*8+0x4]
   18011df53:	0f 57 c0             	xorps  xmm0,xmm0
   18011df56:	f3 0f 2a 05 2e 05 7f 	cvtsi2ss xmm0,DWORD PTR [rip+0x7f052e]        # 0x18090e48c
   18011df5d:	00 
   18011df5e:	f3 0f 5e f8          	divss  xmm7,xmm0
   18011df62:	0f 5a c7             	cvtps2pd xmm0,xmm7
   18011df65:	f2 0f 11 05 1b e4 6c 	movsd  QWORD PTR [rip+0x6ce41b],xmm0        # 0x1807ec388
   18011df6c:	00 
   18011df6d:	eb 35                	jmp    0x18011dfa4
   18011df6f:	66 0f 6e 05 11 05 7f 	movd   xmm0,DWORD PTR [rip+0x7f0511]        # 0x18090e488
   18011df76:	00 
   18011df77:	0f 5b c0             	cvtdq2ps xmm0,xmm0
   18011df7a:	f3 0f 10 0d 66 f2 16 	movss  xmm1,DWORD PTR [rip+0x116f266]        # 0x18128d1e8
   18011df81:	01 
   18011df82:	f3 0f 5e c8          	divss  xmm1,xmm0
   18011df86:	0f 5a f1             	cvtps2pd xmm6,xmm1
   18011df89:	66 0f 6e 05 fb 04 7f 	movd   xmm0,DWORD PTR [rip+0x7f04fb]        # 0x18090e48c
   18011df90:	00 
   18011df91:	0f 5b c0             	cvtdq2ps xmm0,xmm0
   18011df94:	f3 0f 10 0d 50 f2 16 	movss  xmm1,DWORD PTR [rip+0x116f250]        # 0x18128d1ec
   18011df9b:	01 
   18011df9c:	f3 0f 5e c8          	divss  xmm1,xmm0
   18011dfa0:	44 0f 5a c1          	cvtps2pd xmm8,xmm1
   18011dfa4:	f2 0f 11 75 c0       	movsd  QWORD PTR [rbp-0x40],xmm6
   18011dfa9:	f2 44 0f 11 45 c8    	movsd  QWORD PTR [rbp-0x38],xmm8
   18011dfaf:	41 8b 04 36          	mov    eax,DWORD PTR [r14+rsi*1]
   18011dfb3:	39 05 97 7c 17 01    	cmp    DWORD PTR [rip+0x1177c97],eax        # 0x181295c50
   18011dfb9:	7e 60                	jle    0x18011e01b
   18011dfbb:	48 8d 0d 8e 7c 17 01 	lea    rcx,[rip+0x1177c8e]        # 0x181295c50
   18011dfc2:	e8 9d 9e 46 00       	call   0x180587e64
   18011dfc7:	83 3d 82 7c 17 01 ff 	cmp    DWORD PTR [rip+0x1177c82],0xffffffff        # 0x181295c50
   18011dfce:	75 4b                	jne    0x18011e01b
   18011dfd0:	48 89 1d 81 7c 17 01 	mov    QWORD PTR [rip+0x1177c81],rbx        # 0x181295c58
   18011dfd7:	48 8d 15 3a 18 51 00 	lea    rdx,[rip+0x51183a]        # 0x18062f818
   18011dfde:	48 8d 0d 7b 7c 17 01 	lea    rcx,[rip+0x1177c7b]        # 0x181295c60
   18011dfe5:	e8 86 22 0f 00       	call   0x180210270
   18011dfea:	c7 05 70 7c 17 01 14 	mov    DWORD PTR [rip+0x1177c70],0x14        # 0x181295c64
   18011dff1:	00 00 00 
   18011dff4:	48 8d 05 6d 7c 17 01 	lea    rax,[rip+0x1177c6d]        # 0x181295c68
   18011dffb:	48 89 05 56 7c 17 01 	mov    QWORD PTR [rip+0x1177c56],rax        # 0x181295c58
   18011e002:	48 8d 0d e7 fc 4a 00 	lea    rcx,[rip+0x4afce7]        # 0x1805cdcf0
   18011e009:	e8 b6 9b 46 00       	call   0x180587bc4
   18011e00e:	90                   	nop
   18011e00f:	48 8d 0d 3a 7c 17 01 	lea    rcx,[rip+0x1177c3a]        # 0x181295c50
   18011e016:	e8 e9 9d 46 00       	call   0x180587e04
   18011e01b:	8b 05 6f 49 6e 00    	mov    eax,DWORD PTR [rip+0x6e496f]        # 0x180802990
   18011e021:	89 85 d0 09 00 00    	mov    DWORD PTR [rbp+0x9d0],eax
   18011e027:	45 33 c0             	xor    r8d,r8d
   18011e02a:	48 8d 95 d0 09 00 00 	lea    rdx,[rbp+0x9d0]
   18011e031:	8b 0d 29 7c 17 01    	mov    ecx,DWORD PTR [rip+0x1177c29]        # 0x181295c60
   18011e037:	ff 15 53 1f 4c 00    	call   QWORD PTR [rip+0x4c1f53]        # 0x1805dff90
   18011e03d:	8b 85 d0 09 00 00    	mov    eax,DWORD PTR [rbp+0x9d0]
   18011e043:	89 05 1f 7c 17 01    	mov    DWORD PTR [rip+0x1177c1f],eax        # 0x181295c68
   18011e049:	41 8b 04 36          	mov    eax,DWORD PTR [r14+rsi*1]
   18011e04d:	39 05 1d 7c 17 01    	cmp    DWORD PTR [rip+0x1177c1d],eax        # 0x181295c70
   18011e053:	7e 60                	jle    0x18011e0b5
   18011e055:	48 8d 0d 14 7c 17 01 	lea    rcx,[rip+0x1177c14]        # 0x181295c70
   18011e05c:	e8 03 9e 46 00       	call   0x180587e64
   18011e061:	83 3d 08 7c 17 01 ff 	cmp    DWORD PTR [rip+0x1177c08],0xffffffff        # 0x181295c70
   18011e068:	75 4b                	jne    0x18011e0b5
   18011e06a:	48 89 1d 07 7c 17 01 	mov    QWORD PTR [rip+0x1177c07],rbx        # 0x181295c78
   18011e071:	48 8d 15 e8 17 51 00 	lea    rdx,[rip+0x5117e8]        # 0x18062f860
   18011e078:	48 8d 0d 01 7c 17 01 	lea    rcx,[rip+0x1177c01]        # 0x181295c80
   18011e07f:	e8 ec 21 0f 00       	call   0x180210270
   18011e084:	c7 05 f6 7b 17 01 14 	mov    DWORD PTR [rip+0x1177bf6],0x14        # 0x181295c84
   18011e08b:	00 00 00 
   18011e08e:	48 8d 05 f3 7b 17 01 	lea    rax,[rip+0x1177bf3]        # 0x181295c88
   18011e095:	48 89 05 dc 7b 17 01 	mov    QWORD PTR [rip+0x1177bdc],rax        # 0x181295c78
   18011e09c:	48 8d 0d 1d fc 4a 00 	lea    rcx,[rip+0x4afc1d]        # 0x1805cdcc0
   18011e0a3:	e8 1c 9b 46 00       	call   0x180587bc4
   18011e0a8:	90                   	nop
   18011e0a9:	48 8d 0d c0 7b 17 01 	lea    rcx,[rip+0x1177bc0]        # 0x181295c70
   18011e0b0:	e8 4f 9d 46 00       	call   0x180587e04
   18011e0b5:	8b 05 15 49 6e 00    	mov    eax,DWORD PTR [rip+0x6e4915]        # 0x1808029d0
   18011e0bb:	89 85 d0 09 00 00    	mov    DWORD PTR [rbp+0x9d0],eax
   18011e0c1:	45 33 c0             	xor    r8d,r8d
   18011e0c4:	48 8d 95 d0 09 00 00 	lea    rdx,[rbp+0x9d0]
   18011e0cb:	8b 0d af 7b 17 01    	mov    ecx,DWORD PTR [rip+0x1177baf]        # 0x181295c80
   18011e0d1:	ff 15 b9 1e 4c 00    	call   QWORD PTR [rip+0x4c1eb9]        # 0x1805dff90
   18011e0d7:	8b 85 d0 09 00 00    	mov    eax,DWORD PTR [rbp+0x9d0]
   18011e0dd:	89 05 a5 7b 17 01    	mov    DWORD PTR [rip+0x1177ba5],eax        # 0x181295c88
   18011e0e3:	41 8b 04 36          	mov    eax,DWORD PTR [r14+rsi*1]
   18011e0e7:	39 05 a3 7b 17 01    	cmp    DWORD PTR [rip+0x1177ba3],eax        # 0x181295c90
   18011e0ed:	7e 60                	jle    0x18011e14f
   18011e0ef:	48 8d 0d 9a 7b 17 01 	lea    rcx,[rip+0x1177b9a]        # 0x181295c90
   18011e0f6:	e8 69 9d 46 00       	call   0x180587e64
   18011e0fb:	83 3d 8e 7b 17 01 ff 	cmp    DWORD PTR [rip+0x1177b8e],0xffffffff        # 0x181295c90
   18011e102:	75 4b                	jne    0x18011e14f
   18011e104:	48 89 1d 8d 7b 17 01 	mov    QWORD PTR [rip+0x1177b8d],rbx        # 0x181295c98
   18011e10b:	48 8d 15 3e 17 51 00 	lea    rdx,[rip+0x51173e]        # 0x18062f850
   18011e112:	48 8d 0d 87 7b 17 01 	lea    rcx,[rip+0x1177b87]        # 0x181295ca0
   18011e119:	e8 52 21 0f 00       	call   0x180210270
   18011e11e:	c7 05 7c 7b 17 01 14 	mov    DWORD PTR [rip+0x1177b7c],0x14        # 0x181295ca4
   18011e125:	00 00 00 
   18011e128:	48 8d 05 79 7b 17 01 	lea    rax,[rip+0x1177b79]        # 0x181295ca8
   18011e12f:	48 89 05 62 7b 17 01 	mov    QWORD PTR [rip+0x1177b62],rax        # 0x181295c98
   18011e136:	48 8d 0d 53 fb 4a 00 	lea    rcx,[rip+0x4afb53]        # 0x1805cdc90
   18011e13d:	e8 82 9a 46 00       	call   0x180587bc4
   18011e142:	90                   	nop
   18011e143:	48 8d 0d 46 7b 17 01 	lea    rcx,[rip+0x1177b46]        # 0x181295c90
   18011e14a:	e8 b5 9c 46 00       	call   0x180587e04
   18011e14f:	8b c3                	mov    eax,ebx
   18011e151:	38 1d cc 47 6e 00    	cmp    BYTE PTR [rip+0x6e47cc],bl        # 0x180802923
   18011e157:	0f 95 c0             	setne  al
   18011e15a:	89 85 d0 09 00 00    	mov    DWORD PTR [rbp+0x9d0],eax
   18011e160:	45 33 c0             	xor    r8d,r8d
   18011e163:	48 8d 95 d0 09 00 00 	lea    rdx,[rbp+0x9d0]
   18011e16a:	8b 0d 30 7b 17 01    	mov    ecx,DWORD PTR [rip+0x1177b30]        # 0x181295ca0
   18011e170:	ff 15 1a 1e 4c 00    	call   QWORD PTR [rip+0x4c1e1a]        # 0x1805dff90
   18011e176:	8b 85 d0 09 00 00    	mov    eax,DWORD PTR [rbp+0x9d0]
   18011e17c:	89 05 26 7b 17 01    	mov    DWORD PTR [rip+0x1177b26],eax        # 0x181295ca8
   18011e182:	48 8d 4d 20          	lea    rcx,[rbp+0x20]
   18011e186:	e8 55 fe 05 00       	call   0x18017dfe0
   18011e18b:	90                   	nop
   18011e18c:	4c 8d 0d 8d e1 6c 00 	lea    r9,[rip+0x6ce18d]        # 0x1807ec320
   18011e193:	4c 8d 44 24 60       	lea    r8,[rsp+0x60]
   18011e198:	49 8b d7             	mov    rdx,r15
   18011e19b:	48 8d 4d 20          	lea    rcx,[rbp+0x20]
   18011e19f:	e8 9c 27 06 00       	call   0x180180940
   18011e1a4:	48 8d 4d 20          	lea    rcx,[rbp+0x20]
   18011e1a8:	e8 b3 09 06 00       	call   0x18017eb60
   18011e1ad:	0f 28 05 fc 13 56 00 	movaps xmm0,XMMWORD PTR [rip+0x5613fc]        # 0x18067f5b0
   18011e1b4:	0f 29 05 65 e1 6c 00 	movaps XMMWORD PTR [rip+0x6ce165],xmm0        # 0x1807ec320
   18011e1bb:	0f 57 c9             	xorps  xmm1,xmm1
   18011e1be:	0f 29 0d 6b e1 6c 00 	movaps XMMWORD PTR [rip+0x6ce16b],xmm1        # 0x1807ec330
   18011e1c5:	0f 28 05 34 23 56 00 	movaps xmm0,XMMWORD PTR [rip+0x562334]        # 0x180680500
   18011e1cc:	0f 29 05 6d e1 6c 00 	movaps XMMWORD PTR [rip+0x6ce16d],xmm0        # 0x1807ec340
   18011e1d3:	0f 29 0d 76 e1 6c 00 	movaps XMMWORD PTR [rip+0x6ce176],xmm1        # 0x1807ec350
   18011e1da:	0f 57 c0             	xorps  xmm0,xmm0
   18011e1dd:	0f 29 05 7c e1 6c 00 	movaps XMMWORD PTR [rip+0x6ce17c],xmm0        # 0x1807ec360
   18011e1e4:	0f 28 0d c5 13 56 00 	movaps xmm1,XMMWORD PTR [rip+0x5613c5]        # 0x18067f5b0
   18011e1eb:	0f 29 0d 7e e1 6c 00 	movaps XMMWORD PTR [rip+0x6ce17e],xmm1        # 0x1807ec370
   18011e1f2:	f2 0f 11 35 86 e1 6c 	movsd  QWORD PTR [rip+0x6ce186],xmm6        # 0x1807ec380
   18011e1f9:	00 
   18011e1fa:	f2 44 0f 11 05 85 e1 	movsd  QWORD PTR [rip+0x6ce185],xmm8        # 0x1807ec388
   18011e201:	6c 00 
   18011e203:	0f 28 05 f6 22 56 00 	movaps xmm0,XMMWORD PTR [rip+0x5622f6]        # 0x180680500
   18011e20a:	0f 29 05 7f e1 6c 00 	movaps XMMWORD PTR [rip+0x6ce17f],xmm0        # 0x1807ec390
   18011e211:	48 8d 55 20          	lea    rdx,[rbp+0x20]
   18011e215:	80 3d 06 47 6e 00 00 	cmp    BYTE PTR [rip+0x6e4706],0x0        # 0x180802922
   18011e21c:	49 0f 44 d7          	cmove  rdx,r15
   18011e220:	48 81 c2 e0 00 00 00 	add    rdx,0xe0
   18011e227:	48 8d 4d e0          	lea    rcx,[rbp-0x20]
   18011e22b:	e8 20 a8 ef ff       	call   0x180018a50
   18011e230:	48 8d 4c 24 60       	lea    rcx,[rsp+0x60]
   18011e235:	e8 16 a8 ef ff       	call   0x180018a50
   18011e23a:	f3 0f 10 55 04       	movss  xmm2,DWORD PTR [rbp+0x4]
   18011e23f:	f3 0f 59 15 69 12 56 	mulss  xmm2,DWORD PTR [rip+0x561269]        # 0x18067f4b0
   18011e246:	00 
   18011e247:	f3 0f 10 45 80       	movss  xmm0,DWORD PTR [rbp-0x80]
   18011e24c:	f3 0f 59 05 64 0c 56 	mulss  xmm0,DWORD PTR [rip+0x560c64]        # 0x18067eeb8
   18011e253:	00 
   18011e254:	f3 0f 11 85 d0 09 00 	movss  DWORD PTR [rbp+0x9d0],xmm0
   18011e25b:	00 
   18011e25c:	f3 0f 11 95 d4 09 00 	movss  DWORD PTR [rbp+0x9d4],xmm2
   18011e263:	00 
