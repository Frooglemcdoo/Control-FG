   180147313:	f3 0f 11 4d cc       	movss  DWORD PTR [rbp-0x34],xmm1
   180147318:	bb 02 00 00 00       	mov    ebx,0x2
   18014731d:	49 2b de             	sub    rbx,r14
   180147320:	48 c1 e3 06          	shl    rbx,0x6
   180147324:	48 c7 44 24 20 00 00 	mov    QWORD PTR [rsp+0x20],0x0
   18014732b:	00 00 
   18014732d:	4c 8d 4d 90          	lea    r9,[rbp-0x70]
   180147331:	41 b8 40 00 00 00    	mov    r8d,0x40
   180147337:	48 8b d3             	mov    rdx,rbx
   18014733a:	8b 0d b8 0c 15 01    	mov    ecx,DWORD PTR [rip+0x1150cb8]        # 0x181297ff8
   180147340:	ff 15 42 8a 49 00    	call   QWORD PTR [rip+0x498a42]        # 0x1805dfd88
   180147346:	0f 28 45 90          	movaps xmm0,XMMWORD PTR [rbp-0x70]
   18014734a:	48 8d 05 af 0c 15 01 	lea    rax,[rip+0x1150caf]        # 0x181298000
   180147351:	0f 11 04 03          	movups XMMWORD PTR [rbx+rax*1],xmm0
   180147355:	0f 28 45 a0          	movaps xmm0,XMMWORD PTR [rbp-0x60]
   180147359:	0f 11 44 03 10       	movups XMMWORD PTR [rbx+rax*1+0x10],xmm0
   18014735e:	0f 28 45 b0          	movaps xmm0,XMMWORD PTR [rbp-0x50]
   180147362:	0f 11 44 03 20       	movups XMMWORD PTR [rbx+rax*1+0x20],xmm0
   180147367:	0f 28 45 c0          	movaps xmm0,XMMWORD PTR [rbp-0x40]
   18014736b:	0f 11 44 03 30       	movups XMMWORD PTR [rbx+rax*1+0x30],xmm0
   180147370:	ff c7                	inc    edi
   180147372:	49 ff c6             	inc    r14
   180147375:	83 ff 03             	cmp    edi,0x3
   180147378:	0f 82 f7 fa ff ff    	jb     0x180146e75
   18014737e:	48 8b 7d 80          	mov    rdi,QWORD PTR [rbp-0x80]
   180147382:	b8 40 00 00 00       	mov    eax,0x40
   180147387:	8b 04 07             	mov    eax,DWORD PTR [rdi+rax*1]
   18014738a:	39 05 30 0d 15 01    	cmp    DWORD PTR [rip+0x1150d30],eax        # 0x1812980c0
   180147390:	0f 8e 82 00 00 00    	jle    0x180147418
   180147396:	48 8d 0d 23 0d 15 01 	lea    rcx,[rip+0x1150d23]        # 0x1812980c0
   18014739d:	e8 c2 0a 44 00       	call   0x180587e64
   1801473a2:	83 3d 17 0d 15 01 ff 	cmp    DWORD PTR [rip+0x1150d17],0xffffffff        # 0x1812980c0
   1801473a9:	75 6d                	jne    0x180147418
   1801473ab:	33 db                	xor    ebx,ebx
   1801473ad:	48 89 1d 1c 0d 15 01 	mov    QWORD PTR [rip+0x1150d1c],rbx        # 0x1812980d0
   1801473b4:	48 8d 15 3d b9 4e 00 	lea    rdx,[rip+0x4eb93d]        # 0x180632cf8
   1801473bb:	48 8d 0d 16 0d 15 01 	lea    rcx,[rip+0x1150d16]        # 0x1812980d8
   1801473c2:	e8 a9 8e 0c 00       	call   0x180210270
   1801473c7:	c7 05 0b 0d 15 01 01 	mov    DWORD PTR [rip+0x1150d0b],0x1        # 0x1812980dc
   1801473ce:	00 00 00 
   1801473d1:	48 8d 05 08 0d 15 01 	lea    rax,[rip+0x1150d08]        # 0x1812980e0
   1801473d8:	48 89 05 f1 0c 15 01 	mov    QWORD PTR [rip+0x1150cf1],rax        # 0x1812980d0
   1801473df:	44 8d 43 04          	lea    r8d,[rbx+0x4]
   1801473e3:	48 8d 05 fe 0c 15 01 	lea    rax,[rip+0x1150cfe]        # 0x1812980e8
   1801473ea:	48 89 58 f8          	mov    QWORD PTR [rax-0x8],rbx
   1801473ee:	33 d2                	xor    edx,edx
   1801473f0:	48 89 10             	mov    QWORD PTR [rax],rdx
   1801473f3:	48 8d 40 10          	lea    rax,[rax+0x10]
   1801473f7:	49 83 e8 01          	sub    r8,0x1
   1801473fb:	75 ed                	jne    0x1801473ea
   1801473fd:	48 8d 0d 8c 7f 48 00 	lea    rcx,[rip+0x487f8c]        # 0x1805cf390
   180147404:	e8 bb 07 44 00       	call   0x180587bc4
   180147409:	90                   	nop
   18014740a:	48 8d 0d af 0c 15 01 	lea    rcx,[rip+0x1150caf]        # 0x1812980c0
   180147411:	e8 ee 09 44 00       	call   0x180587e04
   180147416:	eb 02                	jmp    0x18014741a
   180147418:	33 db                	xor    ebx,ebx
   18014741a:	49 8d 94 24 d0 00 00 	lea    rdx,[r12+0xd0]
   180147421:	00 
   180147422:	48 8d 4d 90          	lea    rcx,[rbp-0x70]
   180147426:	e8 25 16 ed ff       	call   0x180018a50
   18014742b:	49 8d 94 24 d0 00 00 	lea    rdx,[r12+0xd0]
   180147432:	00 
   180147433:	48 8d 4d e0          	lea    rcx,[rbp-0x20]
   180147437:	e8 14 16 ed ff       	call   0x180018a50
   18014743c:	f3 0f 10 55 b4       	movss  xmm2,DWORD PTR [rbp-0x4c]
   180147441:	f3 0f 10 3d 67 80 53 	movss  xmm7,DWORD PTR [rip+0x538067]        # 0x18067f4b0
   180147448:	00 
   180147449:	f3 0f 59 d7          	mulss  xmm2,xmm7
   18014744d:	f3 0f 10 45 00       	movss  xmm0,DWORD PTR [rbp+0x0]
   180147452:	f3 0f 10 35 5e 7a 53 	movss  xmm6,DWORD PTR [rip+0x537a5e]        # 0x18067eeb8
   180147459:	00 
   18014745a:	f3 0f 59 c6          	mulss  xmm0,xmm6
   18014745e:	f3 0f 11 45 80       	movss  DWORD PTR [rbp-0x80],xmm0
   180147463:	f3 0f 11 55 84       	movss  DWORD PTR [rbp-0x7c],xmm2
   180147468:	48 89 5c 24 20       	mov    QWORD PTR [rsp+0x20],rbx
   18014746d:	4c 8d 4d 80          	lea    r9,[rbp-0x80]
   180147471:	33 d2                	xor    edx,edx
   180147473:	44 8d 42 08          	lea    r8d,[rdx+0x8]
   180147477:	8b 0d 5b 0c 15 01    	mov    ecx,DWORD PTR [rip+0x1150c5b]        # 0x1812980d8
   18014747d:	ff 15 05 89 49 00    	call   QWORD PTR [rip+0x498905]        # 0x1805dfd88
   180147483:	48 8b 45 80          	mov    rax,QWORD PTR [rbp-0x80]
   180147487:	48 89 05 52 0c 15 01 	mov    QWORD PTR [rip+0x1150c52],rax        # 0x1812980e0
   18014748e:	8b fb                	mov    edi,ebx
   180147490:	48 8b f3             	mov    rsi,rbx
   180147493:	49 bc ab aa aa aa aa 	movabs r12,0xaaaaaaaaaaaaaaab
   18014749a:	aa aa aa 
   18014749d:	4c 8d 35 3c 0c 15 01 	lea    r14,[rip+0x1150c3c]        # 0x1812980e0
   1801474a4:	48 63 cf             	movsxd rcx,edi
   1801474a7:	49 03 cd             	add    rcx,r13
   1801474aa:	49 8b c4             	mov    rax,r12
   1801474ad:	48 f7 e1             	mul    rcx
   1801474b0:	48 d1 ea             	shr    rdx,1
   1801474b3:	48 8d 04 52          	lea    rax,[rdx+rdx*2]
   1801474b7:	48 2b c8             	sub    rcx,rax
   1801474ba:	48 69 c1 f0 02 00 00 	imul   rax,rcx,0x2f0
   1801474c1:	4a 8d 94 38 10 01 00 	lea    rdx,[rax+r15*1+0x110]
   1801474c8:	00 
   1801474c9:	48 8d 4d e0          	lea    rcx,[rbp-0x20]
   1801474cd:	e8 7e 15 ed ff       	call   0x180018a50
   1801474d2:	48 8d 4d 90          	lea    rcx,[rbp-0x70]
   1801474d6:	e8 75 15 ed ff       	call   0x180018a50
   1801474db:	f3 0f 10 55 04       	movss  xmm2,DWORD PTR [rbp+0x4]
   1801474e0:	f3 0f 59 d7          	mulss  xmm2,xmm7
   1801474e4:	f3 0f 10 45 b0       	movss  xmm0,DWORD PTR [rbp-0x50]
   1801474e9:	f3 0f 59 c6          	mulss  xmm0,xmm6
   1801474ed:	f3 0f 11 45 80       	movss  DWORD PTR [rbp-0x80],xmm0
   1801474f2:	f3 0f 11 55 84       	movss  DWORD PTR [rbp-0x7c],xmm2
   1801474f7:	bb 03 00 00 00       	mov    ebx,0x3
   1801474fc:	48 2b de             	sub    rbx,rsi
   1801474ff:	48 c1 e3 04          	shl    rbx,0x4
   180147503:	48 c7 44 24 20 00 00 	mov    QWORD PTR [rsp+0x20],0x0
   18014750a:	00 00 
   18014750c:	4c 8d 4d 80          	lea    r9,[rbp-0x80]
   180147510:	41 b8 08 00 00 00    	mov    r8d,0x8
   180147516:	48 8b d3             	mov    rdx,rbx
   180147519:	8b 0d b9 0b 15 01    	mov    ecx,DWORD PTR [rip+0x1150bb9]        # 0x1812980d8
   18014751f:	ff 15 63 88 49 00    	call   QWORD PTR [rip+0x498863]        # 0x1805dfd88
   180147525:	48 8b 45 80          	mov    rax,QWORD PTR [rbp-0x80]
   180147529:	4a 89 04 33          	mov    QWORD PTR [rbx+r14*1],rax
   18014752d:	ff c7                	inc    edi
   18014752f:	48 ff c6             	inc    rsi
   180147532:	83 ff 03             	cmp    edi,0x3
   180147535:	0f 82 69 ff ff ff    	jb     0x1801474a4
   18014753b:	ff 15 6f 87 49 00    	call   QWORD PTR [rip+0x49876f]        # 0x1805dfcb0
