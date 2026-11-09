   1801303b7:	4c 8b a4 24 10 02 00 	mov    r12,QWORD PTR [rsp+0x210]
   1801303be:	00 
   1801303bf:	49 8b 54 24 08       	mov    rdx,QWORD PTR [r12+0x8]
   1801303c4:	48 81 c2 e0 00 00 00 	add    rdx,0xe0
   1801303cb:	48 8d 8c 24 a0 00 00 	lea    rcx,[rsp+0xa0]
   1801303d2:	00 
   1801303d3:	e8 78 86 ee ff       	call   0x180018a50
   1801303d8:	48 8d 8c 24 e0 00 00 	lea    rcx,[rsp+0xe0]
   1801303df:	00 
   1801303e0:	e8 6b 86 ee ff       	call   0x180018a50
   1801303e5:	f3 0f 10 94 24 c4 00 	movss  xmm2,DWORD PTR [rsp+0xc4]
   1801303ec:	00 00 
   1801303ee:	f3 0f 59 15 ba f0 54 	mulss  xmm2,DWORD PTR [rip+0x54f0ba]        # 0x18067f4b0
   1801303f5:	00 
   1801303f6:	f3 0f 10 84 24 00 01 	movss  xmm0,DWORD PTR [rsp+0x100]
   1801303fd:	00 00 
   1801303ff:	f3 0f 59 05 b1 ea 54 	mulss  xmm0,DWORD PTR [rip+0x54eab1]        # 0x18067eeb8
   180130406:	00 
   180130407:	f3 0f 11 84 24 20 02 	movss  DWORD PTR [rsp+0x220],xmm0
   18013040e:	00 00 
   180130410:	f3 0f 11 94 24 24 02 	movss  DWORD PTR [rsp+0x224],xmm2
   180130417:	00 00 
   180130419:	8b 0d 1d 24 6d 00    	mov    ecx,DWORD PTR [rip+0x6d241d]        # 0x18080283c
   18013041f:	65 48 8b 04 25 58 00 	mov    rax,QWORD PTR gs:0x58
   180130426:	00 00 
   180130428:	48 8b 34 c8          	mov    rsi,QWORD PTR [rax+rcx*8]
   18013042c:	b8 40 00 00 00       	mov    eax,0x40
   180130431:	8b 04 30             	mov    eax,DWORD PTR [rax+rsi*1]
   180130434:	39 05 26 73 16 01    	cmp    DWORD PTR [rip+0x1167326],eax        # 0x181297760
   18013043a:	7e 6b                	jle    0x1801304a7
   18013043c:	48 8d 0d 1d 73 16 01 	lea    rcx,[rip+0x116731d]        # 0x181297760
   180130443:	e8 1c 7a 45 00       	call   0x180587e64
   180130448:	83 3d 11 73 16 01 ff 	cmp    DWORD PTR [rip+0x1167311],0xffffffff        # 0x181297760
   18013044f:	75 56                	jne    0x1801304a7
   180130451:	48 89 2d 10 73 16 01 	mov    QWORD PTR [rip+0x1167310],rbp        # 0x181297768
   180130458:	48 8d 15 51 02 50 00 	lea    rdx,[rip+0x500251]        # 0x1806306b0
   18013045f:	48 8d 0d 0a 73 16 01 	lea    rcx,[rip+0x116730a]        # 0x181297770
   180130466:	e8 05 fe 0d 00       	call   0x180210270
   18013046b:	48 c7 05 fe 72 16 01 	mov    QWORD PTR [rip+0x11672fe],0x1        # 0x181297774
   180130472:	01 00 00 00 
   180130476:	48 8d 05 fb 72 16 01 	lea    rax,[rip+0x11672fb]        # 0x181297778
   18013047d:	48 89 05 e4 72 16 01 	mov    QWORD PTR [rip+0x11672e4],rax        # 0x181297768
   180130484:	c7 05 ee 72 16 01 00 	mov    DWORD PTR [rip+0x11672ee],0x0        # 0x18129777c
   18013048b:	00 00 00 
   18013048e:	48 8d 0d 6b da 49 00 	lea    rcx,[rip+0x49da6b]        # 0x1805cdf00
   180130495:	e8 2a 77 45 00       	call   0x180587bc4
   18013049a:	90                   	nop
   18013049b:	48 8d 0d be 72 16 01 	lea    rcx,[rip+0x11672be]        # 0x181297760
   1801304a2:	e8 5d 79 45 00       	call   0x180587e04
   1801304a7:	45 33 c0             	xor    r8d,r8d
   1801304aa:	48 8d 94 24 20 02 00 	lea    rdx,[rsp+0x220]
   1801304b1:	00 
   1801304b2:	8b 0d b8 72 16 01    	mov    ecx,DWORD PTR [rip+0x11672b8]        # 0x181297770
   1801304b8:	ff 15 d2 fa 4a 00    	call   QWORD PTR [rip+0x4afad2]        # 0x1805dff90
   1801304be:	48 8b 84 24 20 02 00 	mov    rax,QWORD PTR [rsp+0x220]
   1801304c5:	00 
   1801304c6:	48 89 05 ab 72 16 01 	mov    QWORD PTR [rip+0x11672ab],rax        # 0x181297778
   1801304cd:	48 8d 0d f4 de 7d 00 	lea    rcx,[rip+0x7ddef4]        # 0x18090e3c8
   1801304d4:	e8 c7 a6 fe ff       	call   0x18011aba0
   1801304d9:	8b 15 ed de 7d 00    	mov    edx,DWORD PTR [rip+0x7ddeed]        # 0x18090e3cc
   1801304df:	8b 0d e3 de 7d 00    	mov    ecx,DWORD PTR [rip+0x7ddee3]        # 0x18090e3c8
   1801304e5:	e8 36 e7 0d 00       	call   0x18020ec20
   1801304ea:	ff 15 c0 f7 4a 00    	call   QWORD PTR [rip+0x4af7c0]        # 0x1805dfcb0
   1801304f0:	48 8b f8             	mov    rdi,rax
   1801304f3:	48 8b 15 de f8 4a 00 	mov    rdx,QWORD PTR [rip+0x4af8de]        # 0x1805dfdd8
   1801304fa:	48 8b 12             	mov    rdx,QWORD PTR [rdx]
   1801304fd:	48 8b c8             	mov    rcx,rax
   180130500:	ff 15 02 f8 4a 00    	call   QWORD PTR [rip+0x4af802]        # 0x1805dfd08
   180130506:	48 8b 15 13 f1 4a 00 	mov    rdx,QWORD PTR [rip+0x4af113]        # 0x1805df620
   18013050d:	48 8b 12             	mov    rdx,QWORD PTR [rdx]
   180130510:	48 8b cf             	mov    rcx,rdi
   180130513:	ff 15 27 f9 4a 00    	call   QWORD PTR [rip+0x4af927]        # 0x1805dfe40
   180130519:	48 8b 15 e0 f8 4a 00 	mov    rdx,QWORD PTR [rip+0x4af8e0]        # 0x1805dfe00
   180130520:	48 8b 12             	mov    rdx,QWORD PTR [rdx]
   180130523:	48 8b cf             	mov    rcx,rdi
   180130526:	ff 15 1c f9 4a 00    	call   QWORD PTR [rip+0x4af91c]        # 0x1805dfe48
   18013052c:	48 8d 0d 5d 01 50 00 	lea    rcx,[rip+0x50015d]        # 0x180630690
   180130533:	e8 58 bb 00 00       	call   0x18013c090
   180130538:	8b cd                	mov    ecx,ebp
   18013053a:	49 8b d6             	mov    rdx,r14
   18013053d:	0f 1f 00             	nop    DWORD PTR [rax]
   180130540:	4c 8b 02             	mov    r8,QWORD PTR [rdx]
   180130543:	4d 85 c0             	test   r8,r8
   180130546:	74 09                	je     0x180130551
   180130548:	4c 3b c0             	cmp    r8,rax
   18013054b:	0f 84 67 01 00 00    	je     0x1801306b8
   180130551:	ff c1                	inc    ecx
   180130553:	48 83 c2 08          	add    rdx,0x8
   180130557:	83 f9 10             	cmp    ecx,0x10
   18013055a:	7c e4                	jl     0x180130540
   18013055c:	48 8b cd             	mov    rcx,rbp
   18013055f:	48 89 8c 24 80 00 00 	mov    QWORD PTR [rsp+0x80],rcx
   180130566:	00 
   180130567:	48 8d 0d 62 01 50 00 	lea    rcx,[rip+0x500162]        # 0x1806306d0
   18013056e:	e8 dd bb 00 00       	call   0x18013c150
   180130573:	8b cd                	mov    ecx,ebp
   180130575:	49 8b d6             	mov    rdx,r14
   180130578:	0f 1f 84 00 00 00 00 	nop    DWORD PTR [rax+rax*1+0x0]
   18013057f:	00 
   180130580:	4c 8b 02             	mov    r8,QWORD PTR [rdx]
   180130583:	4d 85 c0             	test   r8,r8
   180130586:	74 09                	je     0x180130591
   180130588:	4c 3b c0             	cmp    r8,rax
   18013058b:	0f 84 34 01 00 00    	je     0x1801306c5
   180130591:	ff c1                	inc    ecx
   180130593:	48 83 c2 08          	add    rdx,0x8
   180130597:	83 f9 10             	cmp    ecx,0x10
   18013059a:	7c e4                	jl     0x180130580
   18013059c:	4c 8b cd             	mov    r9,rbp
   18013059f:	4d 8b 09             	mov    r9,QWORD PTR [r9]
   1801305a2:	41 b8 01 00 00 00    	mov    r8d,0x1
   1801305a8:	48 8d 94 24 80 00 00 	lea    rdx,[rsp+0x80]
   1801305af:	00 
   1801305b0:	48 8d 8c 24 20 01 00 	lea    rcx,[rsp+0x120]
   1801305b7:	00 
   1801305b8:	ff 15 a2 f7 4a 00    	call   QWORD PTR [rip+0x4af7a2]        # 0x1805dfd60
   1801305be:	90                   	nop
   1801305bf:	45 33 c0             	xor    r8d,r8d
   1801305c2:	33 d2                	xor    edx,edx
   1801305c4:	48 8d 8c 24 20 01 00 	lea    rcx,[rsp+0x120]
   1801305cb:	00 
   1801305cc:	ff 15 9e f7 4a 00    	call   QWORD PTR [rip+0x4af79e]        # 0x1805dfd70
   1801305d2:	48 8b 05 bf d2 15 01 	mov    rax,QWORD PTR [rip+0x115d2bf]        # 0x18128d898
   1801305d9:	48 85 c0             	test   rax,rax
   1801305dc:	75 48                	jne    0x180130626
