
upload/d3d_rmdwin10_f(4).dll:     file format pei-x86-64


Disassembly of section .text:

000000018001f3f0 <.text+0x1e3f0>:
   18001f3f0:	40 55                	rex push rbp
   18001f3f2:	41 54                	push   r12
   18001f3f4:	41 55                	push   r13
   18001f3f6:	41 56                	push   r14
   18001f3f8:	41 57                	push   r15
   18001f3fa:	48 8d 6c 24 f1       	lea    rbp,[rsp-0xf]
   18001f3ff:	48 81 ec b0 00 00 00 	sub    rsp,0xb0
   18001f406:	4c 8b 15 d3 27 0f 00 	mov    r10,QWORD PTR [rip+0xf27d3]        # 0x180111be0
   18001f40d:	45 8b f1             	mov    r14d,r9d
   18001f410:	45 8b e8             	mov    r13d,r8d
   18001f413:	44 8b e2             	mov    r12d,edx
   18001f416:	44 8b f9             	mov    r15d,ecx
   18001f419:	4d 85 d2             	test   r10,r10
   18001f41c:	0f 84 2b 04 00 00    	je     0x18001f84d
   18001f422:	41 80 7a 08 00       	cmp    BYTE PTR [r10+0x8],0x0
   18001f427:	0f 84 20 04 00 00    	je     0x18001f84d
   18001f42d:	0f b6 4d 6f          	movzx  ecx,BYTE PTR [rbp+0x6f]
   18001f431:	41 22 4a 09          	and    cl,BYTE PTR [r10+0x9]
   18001f435:	49 83 7a 28 00       	cmp    QWORD PTR [r10+0x28],0x0
   18001f43a:	88 4d 6f             	mov    BYTE PTR [rbp+0x6f],cl
   18001f43d:	74 0b                	je     0x18001f44a
   18001f43f:	49 83 7a 30 00       	cmp    QWORD PTR [r10+0x30],0x0
   18001f444:	74 04                	je     0x18001f44a
   18001f446:	b0 01                	mov    al,0x1
   18001f448:	eb 02                	jmp    0x18001f44c
   18001f44a:	32 c0                	xor    al,al
   18001f44c:	45 3b 7a 14          	cmp    r15d,DWORD PTR [r10+0x14]
   18001f450:	75 4d                	jne    0x18001f49f
   18001f452:	45 3b 62 18          	cmp    r12d,DWORD PTR [r10+0x18]
   18001f456:	75 47                	jne    0x18001f49f
   18001f458:	45 3b 6a 0c          	cmp    r13d,DWORD PTR [r10+0xc]
   18001f45c:	75 41                	jne    0x18001f49f
   18001f45e:	45 3b 72 10          	cmp    r14d,DWORD PTR [r10+0x10]
   18001f462:	75 3b                	jne    0x18001f49f
   18001f464:	0f b6 55 5f          	movzx  edx,BYTE PTR [rbp+0x5f]
   18001f468:	41 3a 52 1c          	cmp    dl,BYTE PTR [r10+0x1c]
   18001f46c:	75 31                	jne    0x18001f49f
   18001f46e:	0f b6 55 67          	movzx  edx,BYTE PTR [rbp+0x67]
   18001f472:	41 3a 52 1d          	cmp    dl,BYTE PTR [r10+0x1d]
   18001f476:	75 27                	jne    0x18001f49f
   18001f478:	41 3a 4a 1e          	cmp    cl,BYTE PTR [r10+0x1e]
   18001f47c:	75 21                	jne    0x18001f49f
   18001f47e:	0f b6 55 77          	movzx  edx,BYTE PTR [rbp+0x77]
   18001f482:	41 3a 52 1f          	cmp    dl,BYTE PTR [r10+0x1f]
   18001f486:	75 17                	jne    0x18001f49f
   18001f488:	84 c0                	test   al,al
   18001f48a:	74 13                	je     0x18001f49f
   18001f48c:	b0 01                	mov    al,0x1
   18001f48e:	48 81 c4 b0 00 00 00 	add    rsp,0xb0
   18001f495:	41 5f                	pop    r15
   18001f497:	41 5e                	pop    r14
   18001f499:	41 5d                	pop    r13
   18001f49b:	41 5c                	pop    r12
   18001f49d:	5d                   	pop    rbp
   18001f49e:	c3                   	ret
   18001f49f:	48 8b 45 7f          	mov    rax,QWORD PTR [rbp+0x7f]
   18001f4a3:	ba ff ff ff ff       	mov    edx,0xffffffff
   18001f4a8:	48 89 9c 24 e0 00 00 	mov    QWORD PTR [rsp+0xe0],rbx
   18001f4af:	00 
   18001f4b0:	48 89 b4 24 e8 00 00 	mov    QWORD PTR [rsp+0xe8],rsi
   18001f4b7:	00 
   18001f4b8:	c7 45 cf 01 00 00 00 	mov    DWORD PTR [rbp-0x31],0x1
   18001f4bf:	c6 00 01             	mov    BYTE PTR [rax],0x1
   18001f4c2:	33 c0                	xor    eax,eax
   18001f4c4:	8b f0                	mov    esi,eax
   18001f4c6:	41 c7 42 20 00 00 00 	mov    DWORD PTR [r10+0x20],0x3f000000
   18001f4cd:	3f 
   18001f4ce:	89 55 df             	mov    DWORD PTR [rbp-0x21],edx
   18001f4d1:	48 89 bc 24 f0 00 00 	mov    QWORD PTR [rsp+0xf0],rdi
   18001f4d8:	00 
   18001f4d9:	89 45 c7             	mov    DWORD PTR [rbp-0x39],eax
   18001f4dc:	84 c9                	test   cl,cl
   18001f4de:	89 45 cb             	mov    DWORD PTR [rbp-0x35],eax
   18001f4e1:	89 45 d3             	mov    DWORD PTR [rbp-0x2d],eax
   18001f4e4:	89 45 bf             	mov    DWORD PTR [rbp-0x41],eax
   18001f4e7:	89 45 d7             	mov    DWORD PTR [rbp-0x29],eax
   18001f4ea:	89 45 c3             	mov    DWORD PTR [rbp-0x3d],eax
   18001f4ed:	f3 41 0f 10 42 20    	movss  xmm0,DWORD PTR [r10+0x20]
   18001f4f3:	f3 0f 11 45 db       	movss  DWORD PTR [rbp-0x25],xmm0
   18001f4f8:	49 8b 7a 38          	mov    rdi,QWORD PTR [r10+0x38]
   18001f4fc:	48 8b cf             	mov    rcx,rdi
   18001f4ff:	0f 84 85 00 00 00    	je     0x18001f58a
   18001f505:	4c 8d 45 e7          	lea    r8,[rbp-0x19]
   18001f509:	48 89 45 e7          	mov    QWORD PTR [rbp-0x19],rax
   18001f50d:	48 8d 15 64 4e 04 00 	lea    rdx,[rip+0x44e64]        # 0x180064378
   18001f514:	e8 17 28 03 00       	call   0x180051d30
   18001f519:	48 83 7d e7 00       	cmp    QWORD PTR [rbp-0x19],0x0
   18001f51e:	75 0a                	jne    0x18001f52a
   18001f520:	bb 0c 00 d0 ba       	mov    ebx,0xbad0000c
   18001f525:	e9 7c 01 00 00       	jmp    0x18001f6a6
   18001f52a:	45 8b c7             	mov    r8d,r15d
   18001f52d:	48 8d 15 70 47 04 00 	lea    rdx,[rip+0x44770]        # 0x180063ca4
   18001f534:	48 8b cf             	mov    rcx,rdi
   18001f537:	e8 14 2a 03 00       	call   0x180051f50
   18001f53c:	45 8b c4             	mov    r8d,r12d
   18001f53f:	48 8d 15 66 47 04 00 	lea    rdx,[rip+0x44766]        # 0x180063cac
   18001f546:	48 8b cf             	mov    rcx,rdi
   18001f549:	e8 02 2a 03 00       	call   0x180051f50
   18001f54e:	44 8b c6             	mov    r8d,esi
   18001f551:	48 8d 15 60 47 04 00 	lea    rdx,[rip+0x44760]        # 0x180063cb8
   18001f558:	48 8b cf             	mov    rcx,rdi
   18001f55b:	e8 90 29 03 00       	call   0x180051ef0
   18001f560:	45 33 c0             	xor    r8d,r8d
   18001f563:	48 8d 15 66 47 04 00 	lea    rdx,[rip+0x44766]        # 0x180063cd0
   18001f56a:	48 8b cf             	mov    rcx,rdi
   18001f56d:	e8 7e 29 03 00       	call   0x180051ef0
   18001f572:	48 8b cf             	mov    rcx,rdi
   18001f575:	ff 55 e7             	call   QWORD PTR [rbp-0x19]
   18001f578:	8b c8                	mov    ecx,eax
   18001f57a:	8b d8                	mov    ebx,eax
   18001f57c:	81 e1 00 00 f0 ff    	and    ecx,0xfff00000
   18001f582:	81 f9 00 00 d0 ba    	cmp    ecx,0xbad00000
   18001f588:	eb 7f                	jmp    0x18001f609
   18001f58a:	4c 8d 45 ef          	lea    r8,[rbp-0x11]
   18001f58e:	48 89 45 ef          	mov    QWORD PTR [rbp-0x11],rax
   18001f592:	48 8d 15 ef 46 04 00 	lea    rdx,[rip+0x446ef]        # 0x180063c88
   18001f599:	e8 92 27 03 00       	call   0x180051d30
   18001f59e:	48 83 7d ef 00       	cmp    QWORD PTR [rbp-0x11],0x0
   18001f5a3:	75 0a                	jne    0x18001f5af
   18001f5a5:	bb 0c 00 d0 ba       	mov    ebx,0xbad0000c
   18001f5aa:	e9 f7 00 00 00       	jmp    0x18001f6a6
   18001f5af:	45 8b c7             	mov    r8d,r15d
   18001f5b2:	48 8d 15 eb 46 04 00 	lea    rdx,[rip+0x446eb]        # 0x180063ca4
   18001f5b9:	48 8b cf             	mov    rcx,rdi
   18001f5bc:	e8 8f 29 03 00       	call   0x180051f50
   18001f5c1:	45 8b c4             	mov    r8d,r12d
   18001f5c4:	48 8d 15 e1 46 04 00 	lea    rdx,[rip+0x446e1]        # 0x180063cac
   18001f5cb:	48 8b cf             	mov    rcx,rdi
   18001f5ce:	e8 7d 29 03 00       	call   0x180051f50
   18001f5d3:	44 8b c6             	mov    r8d,esi
   18001f5d6:	48 8d 15 db 46 04 00 	lea    rdx,[rip+0x446db]        # 0x180063cb8
   18001f5dd:	48 8b cf             	mov    rcx,rdi
   18001f5e0:	e8 0b 29 03 00       	call   0x180051ef0
   18001f5e5:	45 33 c0             	xor    r8d,r8d
   18001f5e8:	48 8d 15 e1 46 04 00 	lea    rdx,[rip+0x446e1]        # 0x180063cd0
   18001f5ef:	48 8b cf             	mov    rcx,rdi
   18001f5f2:	e8 f9 28 03 00       	call   0x180051ef0
   18001f5f7:	48 8b cf             	mov    rcx,rdi
   18001f5fa:	ff 55 ef             	call   QWORD PTR [rbp-0x11]
   18001f5fd:	8b d8                	mov    ebx,eax
   18001f5ff:	25 00 00 f0 ff       	and    eax,0xfff00000
   18001f604:	3d 00 00 d0 ba       	cmp    eax,0xbad00000
   18001f609:	0f 84 97 00 00 00    	je     0x18001f6a6
   18001f60f:	4c 8d 45 c7          	lea    r8,[rbp-0x39]
   18001f613:	48 8b cf             	mov    rcx,rdi
   18001f616:	48 8d 15 c3 46 04 00 	lea    rdx,[rip+0x446c3]        # 0x180063ce0
   18001f61d:	e8 4e 26 03 00       	call   0x180051c70
   18001f622:	4c 8d 45 cb          	lea    r8,[rbp-0x35]
   18001f626:	48 8b cf             	mov    rcx,rdi
   18001f629:	48 8d 15 c0 46 04 00 	lea    rdx,[rip+0x446c0]        # 0x180063cf0
   18001f630:	e8 3b 26 03 00       	call   0x180051c70
   18001f635:	8b 4d c7             	mov    ecx,DWORD PTR [rbp-0x39]
   18001f638:	4c 8d 45 bf          	lea    r8,[rbp-0x41]
   18001f63c:	8b 45 cb             	mov    eax,DWORD PTR [rbp-0x35]
   18001f63f:	48 8d 15 ba 46 04 00 	lea    rdx,[rip+0x446ba]        # 0x180063d00
   18001f646:	89 4d bf             	mov    DWORD PTR [rbp-0x41],ecx
   18001f649:	89 4d d3             	mov    DWORD PTR [rbp-0x2d],ecx
   18001f64c:	48 8b cf             	mov    rcx,rdi
   18001f64f:	89 45 c3             	mov    DWORD PTR [rbp-0x3d],eax
   18001f652:	89 45 d7             	mov    DWORD PTR [rbp-0x29],eax
   18001f655:	e8 16 26 03 00       	call   0x180051c70
   18001f65a:	4c 8d 45 c3          	lea    r8,[rbp-0x3d]
   18001f65e:	48 8b cf             	mov    rcx,rdi
   18001f661:	48 8d 15 c0 46 04 00 	lea    rdx,[rip+0x446c0]        # 0x180063d28
   18001f668:	e8 03 26 03 00       	call   0x180051c70
   18001f66d:	4c 8d 45 d3          	lea    r8,[rbp-0x2d]
   18001f671:	48 8b cf             	mov    rcx,rdi
   18001f674:	48 8d 15 d5 46 04 00 	lea    rdx,[rip+0x446d5]        # 0x180063d50
   18001f67b:	e8 f0 25 03 00       	call   0x180051c70
   18001f680:	4c 8d 45 d7          	lea    r8,[rbp-0x29]
   18001f684:	48 8b cf             	mov    rcx,rdi
   18001f687:	48 8d 15 ea 46 04 00 	lea    rdx,[rip+0x446ea]        # 0x180063d78
   18001f68e:	e8 dd 25 03 00       	call   0x180051c70
   18001f693:	4c 8d 45 db          	lea    r8,[rbp-0x25]
   18001f697:	48 8b cf             	mov    rcx,rdi
   18001f69a:	48 8d 15 ff 46 04 00 	lea    rdx,[rip+0x446ff]        # 0x180063da0
   18001f6a1:	e8 0a 25 03 00       	call   0x180051bb0
   18001f6a6:	4c 8b 15 33 25 0f 00 	mov    r10,QWORD PTR [rip+0xf2533]        # 0x180111be0
   18001f6ad:	81 e3 00 00 f0 ff    	and    ebx,0xfff00000
   18001f6b3:	81 fb 00 00 d0 ba    	cmp    ebx,0xbad00000
   18001f6b9:	0f 84 bc 00 00 00    	je     0x18001f77b
   18001f6bf:	8b 5d c7             	mov    ebx,DWORD PTR [rbp-0x39]
   18001f6c2:	85 db                	test   ebx,ebx
   18001f6c4:	0f 84 b1 00 00 00    	je     0x18001f77b
   18001f6ca:	8b 7d cb             	mov    edi,DWORD PTR [rbp-0x35]
   18001f6cd:	85 ff                	test   edi,edi
   18001f6cf:	0f 84 a6 00 00 00    	je     0x18001f77b
   18001f6d5:	83 fe 05             	cmp    esi,0x5
   18001f6d8:	75 1d                	jne    0x18001f6f7
   18001f6da:	41 8b df             	mov    ebx,r15d
   18001f6dd:	44 89 65 cb          	mov    DWORD PTR [rbp-0x35],r12d
   18001f6e1:	89 5d c7             	mov    DWORD PTR [rbp-0x39],ebx
   18001f6e4:	41 8b fc             	mov    edi,r12d
   18001f6e7:	45 8b cf             	mov    r9d,r15d
   18001f6ea:	44 89 7d bf          	mov    DWORD PTR [rbp-0x41],r15d
   18001f6ee:	45 8b dc             	mov    r11d,r12d
   18001f6f1:	44 89 65 c3          	mov    DWORD PTR [rbp-0x3d],r12d
   18001f6f5:	eb 1e                	jmp    0x18001f715
   18001f6f7:	44 8b 4d bf          	mov    r9d,DWORD PTR [rbp-0x41]
   18001f6fb:	45 85 c9             	test   r9d,r9d
   18001f6fe:	74 09                	je     0x18001f709
   18001f700:	44 8b 5d c3          	mov    r11d,DWORD PTR [rbp-0x3d]
   18001f704:	45 85 db             	test   r11d,r11d
   18001f707:	75 0c                	jne    0x18001f715
   18001f709:	44 8b cb             	mov    r9d,ebx
   18001f70c:	89 5d bf             	mov    DWORD PTR [rbp-0x41],ebx
   18001f70f:	44 8b df             	mov    r11d,edi
   18001f712:	89 7d c3             	mov    DWORD PTR [rbp-0x3d],edi
   18001f715:	45 8b c5             	mov    r8d,r13d
   18001f718:	41 8b ce             	mov    ecx,r14d
   18001f71b:	2b cf                	sub    ecx,edi
   18001f71d:	44 2b c3             	sub    r8d,ebx
   18001f720:	8b c1                	mov    eax,ecx
   18001f722:	41 8b d0             	mov    edx,r8d
   18001f725:	c1 f8 1f             	sar    eax,0x1f
   18001f728:	03 c8                	add    ecx,eax
   18001f72a:	c1 fa 1f             	sar    edx,0x1f
   18001f72d:	33 c8                	xor    ecx,eax
   18001f72f:	42 8d 04 02          	lea    eax,[rdx+r8*1]
   18001f733:	33 c2                	xor    eax,edx
   18001f735:	03 c8                	add    ecx,eax
   18001f737:	44 3b 6d d3          	cmp    r13d,DWORD PTR [rbp-0x2d]
   18001f73b:	72 14                	jb     0x18001f751
   18001f73d:	45 3b e9             	cmp    r13d,r9d
   18001f740:	77 0f                	ja     0x18001f751
   18001f742:	44 3b 75 d7          	cmp    r14d,DWORD PTR [rbp-0x29]
   18001f746:	72 09                	jb     0x18001f751
   18001f748:	45 3b f3             	cmp    r14d,r11d
   18001f74b:	77 04                	ja     0x18001f751
   18001f74d:	b0 01                	mov    al,0x1
   18001f74f:	eb 02                	jmp    0x18001f753
   18001f751:	32 c0                	xor    al,al
   18001f753:	41 3b dd             	cmp    ebx,r13d
   18001f756:	75 05                	jne    0x18001f75d
   18001f758:	41 3b fe             	cmp    edi,r14d
   18001f75b:	74 33                	je     0x18001f790
   18001f75d:	84 c0                	test   al,al
   18001f75f:	74 1a                	je     0x18001f77b
   18001f761:	3b 4d df             	cmp    ecx,DWORD PTR [rbp-0x21]
   18001f764:	73 15                	jae    0x18001f77b
   18001f766:	f3 0f 10 45 db       	movss  xmm0,DWORD PTR [rbp-0x25]
   18001f76b:	8b c6                	mov    eax,esi
   18001f76d:	f3 41 0f 11 42 20    	movss  DWORD PTR [r10+0x20],xmm0
   18001f773:	89 45 cf             	mov    DWORD PTR [rbp-0x31],eax
   18001f776:	89 4d df             	mov    DWORD PTR [rbp-0x21],ecx
   18001f779:	eb 03                	jmp    0x18001f77e
   18001f77b:	8b 45 cf             	mov    eax,DWORD PTR [rbp-0x31]
   18001f77e:	ff c6                	inc    esi
   18001f780:	83 fe 06             	cmp    esi,0x6
   18001f783:	7d 1f                	jge    0x18001f7a4
   18001f785:	0f b6 4d 6f          	movzx  ecx,BYTE PTR [rbp+0x6f]
   18001f789:	33 c0                	xor    eax,eax
   18001f78b:	e9 49 fd ff ff       	jmp    0x18001f4d9
   18001f790:	4c 8b 15 49 24 0f 00 	mov    r10,QWORD PTR [rip+0xf2449]        # 0x180111be0
   18001f797:	8b c6                	mov    eax,esi
   18001f799:	f3 0f 10 45 db       	movss  xmm0,DWORD PTR [rbp-0x25]
   18001f79e:	f3 41 0f 11 42 20    	movss  DWORD PTR [r10+0x20],xmm0
   18001f7a4:	48 8b bc 24 f0 00 00 	mov    rdi,QWORD PTR [rsp+0xf0]
   18001f7ab:	00 
   18001f7ac:	48 8b b4 24 e8 00 00 	mov    rsi,QWORD PTR [rsp+0xe8]
   18001f7b3:	00 
   18001f7b4:	48 8b 9c 24 e0 00 00 	mov    rbx,QWORD PTR [rsp+0xe0]
   18001f7bb:	00 
   18001f7bc:	45 3b ef             	cmp    r13d,r15d
   18001f7bf:	72 0b                	jb     0x18001f7cc
   18001f7c1:	45 3b f4             	cmp    r14d,r12d
   18001f7c4:	b9 05 00 00 00       	mov    ecx,0x5
   18001f7c9:	0f 43 c1             	cmovae eax,ecx
   18001f7cc:	44 0f b6 45 67       	movzx  r8d,BYTE PTR [rbp+0x67]
   18001f7d1:	44 0f b6 4d 5f       	movzx  r9d,BYTE PTR [rbp+0x5f]
   18001f7d6:	0f b6 55 77          	movzx  edx,BYTE PTR [rbp+0x77]
   18001f7da:	0f b6 4d 6f          	movzx  ecx,BYTE PTR [rbp+0x6f]
   18001f7de:	88 54 24 50          	mov    BYTE PTR [rsp+0x50],dl
   18001f7e2:	88 4c 24 48          	mov    BYTE PTR [rsp+0x48],cl
   18001f7e6:	44 88 44 24 40       	mov    BYTE PTR [rsp+0x40],r8b
   18001f7eb:	44 88 4c 24 38       	mov    BYTE PTR [rsp+0x38],r9b
   18001f7f0:	89 44 24 30          	mov    DWORD PTR [rsp+0x30],eax
   18001f7f4:	45 88 4a 1c          	mov    BYTE PTR [r10+0x1c],r9b
   18001f7f8:	45 8b cc             	mov    r9d,r12d
   18001f7fb:	45 88 42 1d          	mov    BYTE PTR [r10+0x1d],r8b
   18001f7ff:	45 8b c7             	mov    r8d,r15d
   18001f802:	41 88 52 1f          	mov    BYTE PTR [r10+0x1f],dl
   18001f806:	48 8d 55 f7          	lea    rdx,[rbp-0x9]
   18001f80a:	44 89 74 24 28       	mov    DWORD PTR [rsp+0x28],r14d
   18001f80f:	44 89 6c 24 20       	mov    DWORD PTR [rsp+0x20],r13d
   18001f814:	41 88 4a 1e          	mov    BYTE PTR [r10+0x1e],cl
   18001f818:	e8 b3 e9 ff ff       	call   0x18001e1d0
   18001f81d:	48 8b 0d bc 23 0f 00 	mov    rcx,QWORD PTR [rip+0xf23bc]        # 0x180111be0
   18001f824:	0f 10 00             	movups xmm0,XMMWORD PTR [rax]
   18001f827:	44 89 79 14          	mov    DWORD PTR [rcx+0x14],r15d
   18001f82b:	44 89 61 18          	mov    DWORD PTR [rcx+0x18],r12d
   18001f82f:	0f 11 41 28          	movups XMMWORD PTR [rcx+0x28],xmm0
   18001f833:	48 83 79 28 00       	cmp    QWORD PTR [rcx+0x28],0x0
   18001f838:	44 89 69 0c          	mov    DWORD PTR [rcx+0xc],r13d
   18001f83c:	44 89 71 10          	mov    DWORD PTR [rcx+0x10],r14d
   18001f840:	74 0b                	je     0x18001f84d
   18001f842:	48 83 79 30 00       	cmp    QWORD PTR [rcx+0x30],0x0
   18001f847:	0f 85 3f fc ff ff    	jne    0x18001f48c
   18001f84d:	32 c0                	xor    al,al
   18001f84f:	48 81 c4 b0 00 00 00 	add    rsp,0xb0
   18001f856:	41 5f                	pop    r15
   18001f858:	41 5e                	pop    r14
   18001f85a:	41 5d                	pop    r13
   18001f85c:	41 5c                	pop    r12
   18001f85e:	5d                   	pop    rbp
   18001f85f:	c3                   	ret
