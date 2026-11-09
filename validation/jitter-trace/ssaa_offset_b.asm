   18014786c:	ff 15 96 84 49 00    	call   QWORD PTR [rip+0x498496]        # 0x1805dfd08
   180147872:	48 8b 15 6f 85 49 00 	mov    rdx,QWORD PTR [rip+0x49856f]        # 0x1805dfde8
   180147879:	48 8b 12             	mov    rdx,QWORD PTR [rdx]
   18014787c:	48 8b cb             	mov    rcx,rbx
   18014787f:	ff 15 bb 85 49 00    	call   QWORD PTR [rip+0x4985bb]        # 0x1805dfe40
   180147885:	48 8b 15 74 85 49 00 	mov    rdx,QWORD PTR [rip+0x498574]        # 0x1805dfe00
   18014788c:	48 8b 12             	mov    rdx,QWORD PTR [rdx]
   18014788f:	48 8b cb             	mov    rcx,rbx
   180147892:	ff 15 b0 85 49 00    	call   QWORD PTR [rip+0x4985b0]        # 0x1805dfe48
   180147898:	49 8b cd             	mov    rcx,r13
   18014789b:	ff 15 47 84 49 00    	call   QWORD PTR [rip+0x498447]        # 0x1805dfce8
   1801478a1:	89 45 c0             	mov    DWORD PTR [rbp-0x40],eax
   1801478a4:	49 8b cd             	mov    rcx,r13
   1801478a7:	ff 15 33 84 49 00    	call   QWORD PTR [rip+0x498433]        # 0x1805dfce0
   1801478ad:	89 45 a0             	mov    DWORD PTR [rbp-0x60],eax
   1801478b0:	48 8b cf             	mov    rcx,rdi
   1801478b3:	ff 15 2f 84 49 00    	call   QWORD PTR [rip+0x49842f]        # 0x1805dfce8
   1801478b9:	48 8b cf             	mov    rcx,rdi
   1801478bc:	ff 15 1e 84 49 00    	call   QWORD PTR [rip+0x49841e]        # 0x1805dfce0
   1801478c2:	8b 15 74 af 6b 00    	mov    edx,DWORD PTR [rip+0x6baf74]        # 0x18080283c
   1801478c8:	65 48 8b 0c 25 58 00 	mov    rcx,QWORD PTR gs:0x58
   1801478cf:	00 00 
   1801478d1:	b8 40 00 00 00       	mov    eax,0x40
   1801478d6:	4c 8b 3c d1          	mov    r15,QWORD PTR [rcx+rdx*8]
   1801478da:	4c 89 7d 80          	mov    QWORD PTR [rbp-0x80],r15
   1801478de:	33 db                	xor    ebx,ebx
   1801478e0:	48 8d 3d 59 08 15 01 	lea    rdi,[rip+0x1150859]        # 0x181298140
   1801478e7:	41 8b 0c 07          	mov    ecx,DWORD PTR [r15+rax*1]
   1801478eb:	39 0d 2f 08 15 01    	cmp    DWORD PTR [rip+0x115082f],ecx        # 0x181298120
   1801478f1:	7e 7b                	jle    0x18014796e
   1801478f3:	48 8d 0d 26 08 15 01 	lea    rcx,[rip+0x1150826]        # 0x181298120
   1801478fa:	e8 65 05 44 00       	call   0x180587e64
   1801478ff:	83 3d 1a 08 15 01 ff 	cmp    DWORD PTR [rip+0x115081a],0xffffffff        # 0x181298120
   180147906:	75 66                	jne    0x18014796e
   180147908:	48 89 1d 21 08 15 01 	mov    QWORD PTR [rip+0x1150821],rbx        # 0x181298130
   18014790f:	48 8d 15 e2 b3 4e 00 	lea    rdx,[rip+0x4eb3e2]        # 0x180632cf8
   180147916:	48 8d 0d 1b 08 15 01 	lea    rcx,[rip+0x115081b]        # 0x181298138
   18014791d:	e8 4e 89 0c 00       	call   0x180210270
   180147922:	c7 05 10 08 15 01 01 	mov    DWORD PTR [rip+0x1150810],0x1        # 0x18129813c
   180147929:	00 00 00 
   18014792c:	48 89 3d fd 07 15 01 	mov    QWORD PTR [rip+0x11507fd],rdi        # 0x181298130
   180147933:	0f 57 c0             	xorps  xmm0,xmm0
   180147936:	0f 29 05 03 08 15 01 	movaps XMMWORD PTR [rip+0x1150803],xmm0        # 0x181298140
   18014793d:	0f 57 c9             	xorps  xmm1,xmm1
   180147940:	0f 29 0d 09 08 15 01 	movaps XMMWORD PTR [rip+0x1150809],xmm1        # 0x181298150
   180147947:	0f 29 05 12 08 15 01 	movaps XMMWORD PTR [rip+0x1150812],xmm0        # 0x181298160
   18014794e:	0f 29 0d 1b 08 15 01 	movaps XMMWORD PTR [rip+0x115081b],xmm1        # 0x181298170
   180147955:	48 8d 0d f4 7c 48 00 	lea    rcx,[rip+0x487cf4]        # 0x1805cf650
   18014795c:	e8 63 02 44 00       	call   0x180587bc4
   180147961:	90                   	nop
   180147962:	48 8d 0d b7 07 15 01 	lea    rcx,[rip+0x11507b7]        # 0x181298120
   180147969:	e8 96 04 44 00       	call   0x180587e04
   18014796e:	4c 8d b6 d0 00 00 00 	lea    r14,[rsi+0xd0]
   180147975:	4c 89 75 b0          	mov    QWORD PTR [rbp-0x50],r14
   180147979:	49 8b d6             	mov    rdx,r14
   18014797c:	48 8d 4d 30          	lea    rcx,[rbp+0x30]
   180147980:	e8 cb 10 ed ff       	call   0x180018a50
   180147985:	49 8b d6             	mov    rdx,r14
   180147988:	48 8d 4d 70          	lea    rcx,[rbp+0x70]
   18014798c:	e8 bf 10 ed ff       	call   0x180018a50
   180147991:	f3 0f 10 4d 54       	movss  xmm1,DWORD PTR [rbp+0x54]
   180147996:	f3 0f 10 35 12 7b 53 	movss  xmm6,DWORD PTR [rip+0x537b12]        # 0x18067f4b0
   18014799d:	00 
   18014799e:	f3 0f 59 ce          	mulss  xmm1,xmm6
   1801479a2:	f3 0f 10 85 90 00 00 	movss  xmm0,DWORD PTR [rbp+0x90]
   1801479a9:	00 
   1801479aa:	f3 44 0f 10 0d 05 75 	movss  xmm9,DWORD PTR [rip+0x537505]        # 0x18067eeb8
   1801479b1:	53 00 
   1801479b3:	f3 41 0f 59 c1       	mulss  xmm0,xmm9
   1801479b8:	f3 0f 11 45 88       	movss  DWORD PTR [rbp-0x78],xmm0
   1801479bd:	f3 0f 11 4d 8c       	movss  DWORD PTR [rbp-0x74],xmm1
   1801479c2:	48 89 5c 24 20       	mov    QWORD PTR [rsp+0x20],rbx
   1801479c7:	4c 8d 4d 88          	lea    r9,[rbp-0x78]
   1801479cb:	33 d2                	xor    edx,edx
   1801479cd:	44 8d 42 08          	lea    r8d,[rdx+0x8]
   1801479d1:	8b 0d 61 07 15 01    	mov    ecx,DWORD PTR [rip+0x1150761]        # 0x181298138
   1801479d7:	ff 15 ab 83 49 00    	call   QWORD PTR [rip+0x4983ab]        # 0x1805dfd88
   1801479dd:	48 8b 45 88          	mov    rax,QWORD PTR [rbp-0x78]
   1801479e1:	48 89 05 58 07 15 01 	mov    QWORD PTR [rip+0x1150758],rax        # 0x181298140
   1801479e8:	8b fb                	mov    edi,ebx
   1801479ea:	48 8b f3             	mov    rsi,rbx
   1801479ed:	49 be ab aa aa aa aa 	movabs r14,0xaaaaaaaaaaaaaaab
   1801479f4:	aa aa aa 
   1801479f7:	4c 8b 7d 08          	mov    r15,QWORD PTR [rbp+0x8]
   1801479fb:	4c 8d 2d 3e 07 15 01 	lea    r13,[rip+0x115073e]        # 0x181298140
   180147a02:	48 63 cf             	movsxd rcx,edi
   180147a05:	49 03 cc             	add    rcx,r12
   180147a08:	49 8b c6             	mov    rax,r14
   180147a0b:	48 f7 e1             	mul    rcx
   180147a0e:	48 d1 ea             	shr    rdx,1
   180147a11:	48 8d 04 52          	lea    rax,[rdx+rdx*2]
   180147a15:	48 2b c8             	sub    rcx,rax
   180147a18:	48 69 c1 f0 02 00 00 	imul   rax,rcx,0x2f0
   180147a1f:	4a 8d 9c 38 10 01 00 	lea    rbx,[rax+r15*1+0x110]
   180147a26:	00 
   180147a27:	48 8b d3             	mov    rdx,rbx
   180147a2a:	48 8d 4d 70          	lea    rcx,[rbp+0x70]
   180147a2e:	e8 1d 10 ed ff       	call   0x180018a50
   180147a33:	48 8b d3             	mov    rdx,rbx
   180147a36:	48 8d 4d 30          	lea    rcx,[rbp+0x30]
   180147a3a:	e8 11 10 ed ff       	call   0x180018a50
   180147a3f:	f3 0f 10 8d 94 00 00 	movss  xmm1,DWORD PTR [rbp+0x94]
   180147a46:	00 
   180147a47:	f3 0f 59 ce          	mulss  xmm1,xmm6
   180147a4b:	f3 0f 10 45 50       	movss  xmm0,DWORD PTR [rbp+0x50]
   180147a50:	f3 41 0f 59 c1       	mulss  xmm0,xmm9
   180147a55:	f3 0f 11 45 88       	movss  DWORD PTR [rbp-0x78],xmm0
   180147a5a:	f3 0f 11 4d 8c       	movss  DWORD PTR [rbp-0x74],xmm1
   180147a5f:	bb 03 00 00 00       	mov    ebx,0x3
   180147a64:	48 2b de             	sub    rbx,rsi
   180147a67:	48 c1 e3 04          	shl    rbx,0x4
   180147a6b:	48 c7 44 24 20 00 00 	mov    QWORD PTR [rsp+0x20],0x0
   180147a72:	00 00 
   180147a74:	4c 8d 4d 88          	lea    r9,[rbp-0x78]
   180147a78:	41 b8 08 00 00 00    	mov    r8d,0x8
   180147a7e:	48 8b d3             	mov    rdx,rbx
   180147a81:	8b 0d b1 06 15 01    	mov    ecx,DWORD PTR [rip+0x11506b1]        # 0x181298138
   180147a87:	ff 15 fb 82 49 00    	call   QWORD PTR [rip+0x4982fb]        # 0x1805dfd88
   180147a8d:	48 8b 45 88          	mov    rax,QWORD PTR [rbp-0x78]
   180147a91:	4a 89 04 2b          	mov    QWORD PTR [rbx+r13*1],rax
   180147a95:	ff c7                	inc    edi
   180147a97:	48 ff c6             	inc    rsi
   180147a9a:	83 ff 03             	cmp    edi,0x3
   180147a9d:	0f 82 5f ff ff ff    	jb     0x180147a02
   180147aa3:	66 0f 6e 7d c0       	movd   xmm7,DWORD PTR [rbp-0x40]
