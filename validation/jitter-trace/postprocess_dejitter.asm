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
   18011e264:	41 8b 04 36          	mov    eax,DWORD PTR [r14+rsi*1]
   18011e268:	39 05 5a 62 17 01    	cmp    DWORD PTR [rip+0x117625a],eax        # 0x1812944c8
   18011e26e:	7e 6b                	jle    0x18011e2db
   18011e270:	48 8d 0d 51 62 17 01 	lea    rcx,[rip+0x1176251]        # 0x1812944c8
   18011e277:	e8 e8 9b 46 00       	call   0x180587e64
   18011e27c:	83 3d 45 62 17 01 ff 	cmp    DWORD PTR [rip+0x1176245],0xffffffff        # 0x1812944c8
   18011e283:	75 56                	jne    0x18011e2db
   18011e285:	48 89 1d 44 62 17 01 	mov    QWORD PTR [rip+0x1176244],rbx        # 0x1812944d0
   18011e28c:	48 8d 15 3d 0a 51 00 	lea    rdx,[rip+0x510a3d]        # 0x18062ecd0
   18011e293:	48 8d 0d 3e 62 17 01 	lea    rcx,[rip+0x117623e]        # 0x1812944d8
   18011e29a:	e8 d1 1f 0f 00       	call   0x180210270
   18011e29f:	48 c7 05 32 62 17 01 	mov    QWORD PTR [rip+0x1176232],0x1        # 0x1812944dc
   18011e2a6:	01 00 00 00 
   18011e2aa:	48 8d 05 2f 62 17 01 	lea    rax,[rip+0x117622f]        # 0x1812944e0
   18011e2b1:	48 89 05 18 62 17 01 	mov    QWORD PTR [rip+0x1176218],rax        # 0x1812944d0
   18011e2b8:	c7 05 22 62 17 01 00 	mov    DWORD PTR [rip+0x1176222],0x0        # 0x1812944e4
   18011e2bf:	00 00 00 
   18011e2c2:	48 8d 0d 27 eb 4a 00 	lea    rcx,[rip+0x4aeb27]        # 0x1805ccdf0
   18011e2c9:	e8 f6 98 46 00       	call   0x180587bc4
   18011e2ce:	90                   	nop
   18011e2cf:	48 8d 0d f2 61 17 01 	lea    rcx,[rip+0x11761f2]        # 0x1812944c8
   18011e2d6:	e8 29 9b 46 00       	call   0x180587e04
   18011e2db:	45 33 c0             	xor    r8d,r8d
   18011e2de:	48 8d 95 d0 09 00 00 	lea    rdx,[rbp+0x9d0]
   18011e2e5:	8b 0d ed 61 17 01    	mov    ecx,DWORD PTR [rip+0x11761ed]        # 0x1812944d8
   18011e2eb:	ff 15 9f 1c 4c 00    	call   QWORD PTR [rip+0x4c1c9f]        # 0x1805dff90
   18011e2f1:	48 8b 85 d0 09 00 00 	mov    rax,QWORD PTR [rbp+0x9d0]
   18011e2f8:	48 89 05 e1 61 17 01 	mov    QWORD PTR [rip+0x11761e1],rax        # 0x1812944e0
   18011e2ff:	48 8b 85 f0 09 00 00 	mov    rax,QWORD PTR [rbp+0x9f0]
   18011e306:	48 89 44 24 28       	mov    QWORD PTR [rsp+0x28],rax
   18011e30b:	48 8b 85 e8 09 00 00 	mov    rax,QWORD PTR [rbp+0x9e8]
   18011e312:	48 89 44 24 20       	mov    QWORD PTR [rsp+0x20],rax
   18011e317:	4d 8b cd             	mov    r9,r13
   18011e31a:	4c 8d 45 20          	lea    r8,[rbp+0x20]
   18011e31e:	49 8b d7             	mov    rdx,r15
   18011e321:	48 8b cf             	mov    rcx,rdi
   18011e324:	e8 f7 0a 00 00       	call   0x18011ee20
   18011e329:	90                   	nop
   18011e32a:	48 8d 4d 20          	lea    rcx,[rbp+0x20]
   18011e32e:	e8 8d 05 06 00       	call   0x18017e8c0
   18011e333:	e9 cf 01 00 00       	jmp    0x18011e507
   18011e338:	c7 05 4e 46 6e 00 01 	mov    DWORD PTR [rip+0x6e464e],0x1        # 0x180802990
   18011e33f:	00 00 00 
   18011e342:	0f 28 05 67 12 56 00 	movaps xmm0,XMMWORD PTR [rip+0x561267]        # 0x18067f5b0
   18011e349:	0f 29 05 d0 df 6c 00 	movaps XMMWORD PTR [rip+0x6cdfd0],xmm0        # 0x1807ec320
   18011e350:	0f 57 c9             	xorps  xmm1,xmm1
   18011e353:	0f 29 0d d6 df 6c 00 	movaps XMMWORD PTR [rip+0x6cdfd6],xmm1        # 0x1807ec330
   18011e35a:	0f 28 05 9f 21 56 00 	movaps xmm0,XMMWORD PTR [rip+0x56219f]        # 0x180680500
   18011e361:	0f 29 05 d8 df 6c 00 	movaps XMMWORD PTR [rip+0x6cdfd8],xmm0        # 0x1807ec340
   18011e368:	0f 29 0d e1 df 6c 00 	movaps XMMWORD PTR [rip+0x6cdfe1],xmm1        # 0x1807ec350
   18011e36f:	0f 57 c0             	xorps  xmm0,xmm0
   18011e372:	0f 29 05 e7 df 6c 00 	movaps XMMWORD PTR [rip+0x6cdfe7],xmm0        # 0x1807ec360
   18011e379:	0f 28 0d 30 12 56 00 	movaps xmm1,XMMWORD PTR [rip+0x561230]        # 0x18067f5b0
   18011e380:	0f 29 0d e9 df 6c 00 	movaps XMMWORD PTR [rip+0x6cdfe9],xmm1        # 0x1807ec370
   18011e387:	0f 29 05 f2 df 6c 00 	movaps XMMWORD PTR [rip+0x6cdff2],xmm0        # 0x1807ec380
   18011e38e:	0f 28 0d 6b 21 56 00 	movaps xmm1,XMMWORD PTR [rip+0x56216b]        # 0x180680500
   18011e395:	0f 29 0d f4 df 6c 00 	movaps XMMWORD PTR [rip+0x6cdff4],xmm1        # 0x1807ec390
   18011e39c:	8b 0d 9a 44 6e 00    	mov    ecx,DWORD PTR [rip+0x6e449a]        # 0x18080283c
   18011e3a2:	65 48 8b 04 25 58 00 	mov    rax,QWORD PTR gs:0x58
   18011e3a9:	00 00 
   18011e3ab:	4c 8b 34 c8          	mov    r14,QWORD PTR [rax+rcx*8]
   18011e3af:	33 db                	xor    ebx,ebx
   18011e3b1:	41 8b 04 16          	mov    eax,DWORD PTR [r14+rdx*1]
   18011e3b5:	39 05 f5 78 17 01    	cmp    DWORD PTR [rip+0x11778f5],eax        # 0x181295cb0
   18011e3bb:	7e 60                	jle    0x18011e41d
   18011e3bd:	48 8d 0d ec 78 17 01 	lea    rcx,[rip+0x11778ec]        # 0x181295cb0
   18011e3c4:	e8 9b 9a 46 00       	call   0x180587e64
   18011e3c9:	83 3d e0 78 17 01 ff 	cmp    DWORD PTR [rip+0x11778e0],0xffffffff        # 0x181295cb0
   18011e3d0:	75 4b                	jne    0x18011e41d
   18011e3d2:	48 89 1d df 78 17 01 	mov    QWORD PTR [rip+0x11778df],rbx        # 0x181295cb8
   18011e3d9:	48 8d 15 38 14 51 00 	lea    rdx,[rip+0x511438]        # 0x18062f818
   18011e3e0:	48 8d 0d d9 78 17 01 	lea    rcx,[rip+0x11778d9]        # 0x181295cc0
   18011e3e7:	e8 84 1e 0f 00       	call   0x180210270
   18011e3ec:	c7 05 ce 78 17 01 14 	mov    DWORD PTR [rip+0x11778ce],0x14        # 0x181295cc4
   18011e3f3:	00 00 00 
   18011e3f6:	48 8d 05 cb 78 17 01 	lea    rax,[rip+0x11778cb]        # 0x181295cc8
   18011e3fd:	48 89 05 b4 78 17 01 	mov    QWORD PTR [rip+0x11778b4],rax        # 0x181295cb8
   18011e404:	48 8d 0d 55 f8 4a 00 	lea    rcx,[rip+0x4af855]        # 0x1805cdc60
   18011e40b:	e8 b4 97 46 00       	call   0x180587bc4
   18011e410:	90                   	nop
   18011e411:	48 8d 0d 98 78 17 01 	lea    rcx,[rip+0x1177898]        # 0x181295cb0
   18011e418:	e8 e7 99 46 00       	call   0x180587e04
   18011e41d:	8b 05 6d 45 6e 00    	mov    eax,DWORD PTR [rip+0x6e456d]        # 0x180802990
   18011e423:	89 85 d0 09 00 00    	mov    DWORD PTR [rbp+0x9d0],eax
   18011e429:	45 33 c0             	xor    r8d,r8d
   18011e42c:	48 8d 95 d0 09 00 00 	lea    rdx,[rbp+0x9d0]
   18011e433:	8b 0d 87 78 17 01    	mov    ecx,DWORD PTR [rip+0x1177887]        # 0x181295cc0
   18011e439:	ff 15 51 1b 4c 00    	call   QWORD PTR [rip+0x4c1b51]        # 0x1805dff90
   18011e43f:	8b 85 d0 09 00 00    	mov    eax,DWORD PTR [rbp+0x9d0]
   18011e445:	89 05 7d 78 17 01    	mov    DWORD PTR [rip+0x117787d],eax        # 0x181295cc8
   18011e44b:	41 8b 04 36          	mov    eax,DWORD PTR [r14+rsi*1]
   18011e44f:	39 05 7b 78 17 01    	cmp    DWORD PTR [rip+0x117787b],eax        # 0x181295cd0
   18011e455:	7e 60                	jle    0x18011e4b7
   18011e457:	48 8d 0d 72 78 17 01 	lea    rcx,[rip+0x1177872]        # 0x181295cd0
   18011e45e:	e8 01 9a 46 00       	call   0x180587e64
   18011e463:	83 3d 66 78 17 01 ff 	cmp    DWORD PTR [rip+0x1177866],0xffffffff        # 0x181295cd0
   18011e46a:	75 4b                	jne    0x18011e4b7
   18011e46c:	48 89 1d 65 78 17 01 	mov    QWORD PTR [rip+0x1177865],rbx        # 0x181295cd8
