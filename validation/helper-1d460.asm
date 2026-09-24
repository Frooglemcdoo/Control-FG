
/workspace/scratch/65fb49337344/upload/d3d_rmdwin10_f(4).dll:     file format pei-x86-64


Disassembly of section .text:

000000018001d460 <.text+0x1c460>:
   18001d460:	48 89 5c 24 08       	mov    QWORD PTR [rsp+0x8],rbx
   18001d465:	48 89 6c 24 10       	mov    QWORD PTR [rsp+0x10],rbp
   18001d46a:	48 89 74 24 18       	mov    QWORD PTR [rsp+0x18],rsi
   18001d46f:	57                   	push   rdi
   18001d470:	48 83 ec 40          	sub    rsp,0x40
   18001d474:	49 8b f8             	mov    rdi,r8
   18001d477:	0f 29 74 24 30       	movaps XMMWORD PTR [rsp+0x30],xmm6
   18001d47c:	4d 8b 41 20          	mov    r8,QWORD PTR [r9+0x20]
   18001d480:	48 8b f2             	mov    rsi,rdx
   18001d483:	48 8b e9             	mov    rbp,rcx
   18001d486:	0f 29 7c 24 20       	movaps XMMWORD PTR [rsp+0x20],xmm7
   18001d48b:	48 8b cf             	mov    rcx,rdi
   18001d48e:	48 8d 15 57 69 04 00 	lea    rdx,[rip+0x46957]        # 0x180063dec
   18001d495:	49 8b d9             	mov    rbx,r9
   18001d498:	e8 a3 49 03 00       	call   0x180051e40
   18001d49d:	4c 8b 43 28          	mov    r8,QWORD PTR [rbx+0x28]
   18001d4a1:	48 8d 15 4c 69 04 00 	lea    rdx,[rip+0x4694c]        # 0x180063df4
   18001d4a8:	48 8b cf             	mov    rcx,rdi
   18001d4ab:	e8 90 49 03 00       	call   0x180051e40
   18001d4b0:	4c 8b 43 30          	mov    r8,QWORD PTR [rbx+0x30]
   18001d4b4:	48 8d 15 41 69 04 00 	lea    rdx,[rip+0x46941]        # 0x180063dfc
   18001d4bb:	48 8b cf             	mov    rcx,rdi
   18001d4be:	e8 7d 49 03 00       	call   0x180051e40
   18001d4c3:	4c 8b 43 38          	mov    r8,QWORD PTR [rbx+0x38]
   18001d4c7:	48 8d 15 3a 69 04 00 	lea    rdx,[rip+0x4693a]        # 0x180063e08
   18001d4ce:	48 8b cf             	mov    rcx,rdi
   18001d4d1:	e8 6a 49 03 00       	call   0x180051e40
   18001d4d6:	f3 0f 10 53 40       	movss  xmm2,DWORD PTR [rbx+0x40]
   18001d4db:	48 8d 15 36 69 04 00 	lea    rdx,[rip+0x46936]        # 0x180063e18
   18001d4e2:	48 8b cf             	mov    rcx,rdi
   18001d4e5:	e8 b6 49 03 00       	call   0x180051ea0
   18001d4ea:	f3 0f 10 53 44       	movss  xmm2,DWORD PTR [rbx+0x44]
   18001d4ef:	48 8d 15 32 69 04 00 	lea    rdx,[rip+0x46932]        # 0x180063e28
   18001d4f6:	48 8b cf             	mov    rcx,rdi
   18001d4f9:	e8 a2 49 03 00       	call   0x180051ea0
   18001d4fe:	44 8b 43 50          	mov    r8d,DWORD PTR [rbx+0x50]
   18001d502:	48 8d 15 2f 69 04 00 	lea    rdx,[rip+0x4692f]        # 0x180063e38
   18001d509:	48 8b cf             	mov    rcx,rdi
   18001d50c:	e8 df 49 03 00       	call   0x180051ef0
   18001d511:	f3 0f 10 53 54       	movss  xmm2,DWORD PTR [rbx+0x54]
   18001d516:	0f 57 ff             	xorps  xmm7,xmm7
   18001d519:	0f 2e d7             	ucomiss xmm2,xmm7
   18001d51c:	f3 0f 10 35 94 99 04 	movss  xmm6,DWORD PTR [rip+0x49994]        # 0x180066eb8
   18001d523:	00 
   18001d524:	7a 05                	jp     0x18001d52b
   18001d526:	75 03                	jne    0x18001d52b
   18001d528:	0f 28 d6             	movaps xmm2,xmm6
   18001d52b:	48 8d 15 0e 69 04 00 	lea    rdx,[rip+0x4690e]        # 0x180063e40
   18001d532:	48 8b cf             	mov    rcx,rdi
   18001d535:	e8 66 49 03 00       	call   0x180051ea0
   18001d53a:	f3 0f 10 53 58       	movss  xmm2,DWORD PTR [rbx+0x58]
   18001d53f:	0f 2e d7             	ucomiss xmm2,xmm7
   18001d542:	7a 05                	jp     0x18001d549
   18001d544:	75 03                	jne    0x18001d549
   18001d546:	0f 28 d6             	movaps xmm2,xmm6
   18001d549:	48 8d 15 00 69 04 00 	lea    rdx,[rip+0x46900]        # 0x180063e50
   18001d550:	48 8b cf             	mov    rcx,rdi
   18001d553:	e8 48 49 03 00       	call   0x180051ea0
   18001d558:	4c 8b 43 60          	mov    r8,QWORD PTR [rbx+0x60]
   18001d55c:	48 8d 15 fd 68 04 00 	lea    rdx,[rip+0x468fd]        # 0x180063e60
   18001d563:	48 8b cf             	mov    rcx,rdi
   18001d566:	e8 d5 48 03 00       	call   0x180051e40
   18001d56b:	4c 8b 43 68          	mov    r8,QWORD PTR [rbx+0x68]
   18001d56f:	48 8d 15 02 69 04 00 	lea    rdx,[rip+0x46902]        # 0x180063e78
   18001d576:	48 8b cf             	mov    rcx,rdi
   18001d579:	e8 c2 48 03 00       	call   0x180051e40
   18001d57e:	4c 8b 43 70          	mov    r8,QWORD PTR [rbx+0x70]
   18001d582:	48 8d 15 ff 68 04 00 	lea    rdx,[rip+0x468ff]        # 0x180063e88
   18001d589:	48 8b cf             	mov    rcx,rdi
   18001d58c:	e8 af 48 03 00       	call   0x180051e40
   18001d591:	4c 8b 83 48 02 00 00 	mov    r8,QWORD PTR [rbx+0x248]
   18001d598:	48 8d 15 11 69 04 00 	lea    rdx,[rip+0x46911]        # 0x180063eb0
   18001d59f:	48 8b cf             	mov    rcx,rdi
   18001d5a2:	e8 99 48 03 00       	call   0x180051e40
   18001d5a7:	4c 8b 83 50 02 00 00 	mov    r8,QWORD PTR [rbx+0x250]
   18001d5ae:	48 8d 15 0b 69 04 00 	lea    rdx,[rip+0x4690b]        # 0x180063ec0
   18001d5b5:	48 8b cf             	mov    rcx,rdi
   18001d5b8:	e8 83 48 03 00       	call   0x180051e40
   18001d5bd:	4c 8b 83 58 02 00 00 	mov    r8,QWORD PTR [rbx+0x258]
   18001d5c4:	48 8d 15 0d 69 04 00 	lea    rdx,[rip+0x4690d]        # 0x180063ed8
   18001d5cb:	48 8b cf             	mov    rcx,rdi
   18001d5ce:	e8 6d 48 03 00       	call   0x180051e40
   18001d5d3:	4c 8b 83 60 02 00 00 	mov    r8,QWORD PTR [rbx+0x260]
   18001d5da:	48 8d 15 0f 69 04 00 	lea    rdx,[rip+0x4690f]        # 0x180063ef0
   18001d5e1:	48 8b cf             	mov    rcx,rdi
   18001d5e4:	e8 57 48 03 00       	call   0x180051e40
   18001d5e9:	4c 8b 83 68 02 00 00 	mov    r8,QWORD PTR [rbx+0x268]
   18001d5f0:	48 8d 15 11 69 04 00 	lea    rdx,[rip+0x46911]        # 0x180063f08
   18001d5f7:	48 8b cf             	mov    rcx,rdi
   18001d5fa:	e8 41 48 03 00       	call   0x180051e40
   18001d5ff:	4c 8b 83 70 02 00 00 	mov    r8,QWORD PTR [rbx+0x270]
   18001d606:	48 8d 15 13 69 04 00 	lea    rdx,[rip+0x46913]        # 0x180063f20
   18001d60d:	48 8b cf             	mov    rcx,rdi
   18001d610:	e8 2b 48 03 00       	call   0x180051e40
   18001d615:	4c 8b 83 78 02 00 00 	mov    r8,QWORD PTR [rbx+0x278]
   18001d61c:	48 8d 15 0d 69 04 00 	lea    rdx,[rip+0x4690d]        # 0x180063f30
   18001d623:	48 8b cf             	mov    rcx,rdi
   18001d626:	e8 15 48 03 00       	call   0x180051e40
   18001d62b:	4c 8b 83 80 02 00 00 	mov    r8,QWORD PTR [rbx+0x280]
   18001d632:	48 8d 15 0f 69 04 00 	lea    rdx,[rip+0x4690f]        # 0x180063f48
   18001d639:	48 8b cf             	mov    rcx,rdi
   18001d63c:	e8 ff 47 03 00       	call   0x180051e40
   18001d641:	4c 8b 83 88 02 00 00 	mov    r8,QWORD PTR [rbx+0x288]
   18001d648:	48 8d 15 11 69 04 00 	lea    rdx,[rip+0x46911]        # 0x180063f60
   18001d64f:	48 8b cf             	mov    rcx,rdi
   18001d652:	e8 e9 47 03 00       	call   0x180051e40
   18001d657:	4c 8b 83 90 02 00 00 	mov    r8,QWORD PTR [rbx+0x290]
   18001d65e:	48 8d 15 13 69 04 00 	lea    rdx,[rip+0x46913]        # 0x180063f78
   18001d665:	48 8b cf             	mov    rcx,rdi
   18001d668:	e8 d3 47 03 00       	call   0x180051e40
   18001d66d:	4c 8b 83 a0 02 00 00 	mov    r8,QWORD PTR [rbx+0x2a0]
   18001d674:	48 8d 15 2d 69 04 00 	lea    rdx,[rip+0x4692d]        # 0x180063fa8
   18001d67b:	48 8b cf             	mov    rcx,rdi
   18001d67e:	e8 bd 47 03 00       	call   0x180051e40
   18001d683:	4c 8b 83 a8 02 00 00 	mov    r8,QWORD PTR [rbx+0x2a8]
   18001d68a:	48 8d 15 2f 69 04 00 	lea    rdx,[rip+0x4692f]        # 0x180063fc0
   18001d691:	48 8b cf             	mov    rcx,rdi
   18001d694:	e8 a7 47 03 00       	call   0x180051e40
   18001d699:	4c 8b 83 b0 02 00 00 	mov    r8,QWORD PTR [rbx+0x2b0]
   18001d6a0:	48 8d 15 31 69 04 00 	lea    rdx,[rip+0x46931]        # 0x180063fd8
   18001d6a7:	48 8b cf             	mov    rcx,rdi
   18001d6aa:	e8 91 47 03 00       	call   0x180051e40
   18001d6af:	4c 8b 83 b8 02 00 00 	mov    r8,QWORD PTR [rbx+0x2b8]
   18001d6b6:	48 8d 15 33 69 04 00 	lea    rdx,[rip+0x46933]        # 0x180063ff0
   18001d6bd:	48 8b cf             	mov    rcx,rdi
   18001d6c0:	e8 7b 47 03 00       	call   0x180051e40
   18001d6c5:	4c 8b 83 c0 02 00 00 	mov    r8,QWORD PTR [rbx+0x2c0]
   18001d6cc:	48 8d 15 35 69 04 00 	lea    rdx,[rip+0x46935]        # 0x180064008
   18001d6d3:	48 8b cf             	mov    rcx,rdi
   18001d6d6:	e8 65 47 03 00       	call   0x180051e40
   18001d6db:	44 8b 83 c8 02 00 00 	mov    r8d,DWORD PTR [rbx+0x2c8]
   18001d6e2:	48 8d 15 37 69 04 00 	lea    rdx,[rip+0x46937]        # 0x180064020
   18001d6e9:	48 8b cf             	mov    rcx,rdi
   18001d6ec:	e8 5f 48 03 00       	call   0x180051f50
   18001d6f1:	4c 8b 83 d0 02 00 00 	mov    r8,QWORD PTR [rbx+0x2d0]
   18001d6f8:	48 8d 15 31 69 04 00 	lea    rdx,[rip+0x46931]        # 0x180064030
   18001d6ff:	48 8b cf             	mov    rcx,rdi
   18001d702:	e8 39 47 03 00       	call   0x180051e40
   18001d707:	4c 8b 83 d8 02 00 00 	mov    r8,QWORD PTR [rbx+0x2d8]
   18001d70e:	48 8d 15 2b 69 04 00 	lea    rdx,[rip+0x4692b]        # 0x180064040
   18001d715:	48 8b cf             	mov    rcx,rdi
   18001d718:	e8 23 47 03 00       	call   0x180051e40
   18001d71d:	4c 8b 83 e0 02 00 00 	mov    r8,QWORD PTR [rbx+0x2e0]
   18001d724:	48 8d 15 25 69 04 00 	lea    rdx,[rip+0x46925]        # 0x180064050
   18001d72b:	48 8b cf             	mov    rcx,rdi
   18001d72e:	e8 0d 47 03 00       	call   0x180051e40
   18001d733:	4c 8b 83 e8 02 00 00 	mov    r8,QWORD PTR [rbx+0x2e8]
   18001d73a:	48 8d 15 27 69 04 00 	lea    rdx,[rip+0x46927]        # 0x180064068
   18001d741:	48 8b cf             	mov    rcx,rdi
   18001d744:	e8 f7 46 03 00       	call   0x180051e40
   18001d749:	4c 8b 83 f0 02 00 00 	mov    r8,QWORD PTR [rbx+0x2f0]
   18001d750:	48 8d 15 21 69 04 00 	lea    rdx,[rip+0x46921]        # 0x180064078
   18001d757:	48 8b cf             	mov    rcx,rdi
   18001d75a:	e8 e1 46 03 00       	call   0x180051e40
   18001d75f:	f3 0f 10 93 f8 02 00 	movss  xmm2,DWORD PTR [rbx+0x2f8]
   18001d766:	00 
   18001d767:	48 8d 15 22 69 04 00 	lea    rdx,[rip+0x46922]        # 0x180064090
   18001d76e:	48 8b cf             	mov    rcx,rdi
   18001d771:	e8 2a 47 03 00       	call   0x180051ea0
   18001d776:	4c 8b 83 00 03 00 00 	mov    r8,QWORD PTR [rbx+0x300]
   18001d77d:	48 8d 15 24 69 04 00 	lea    rdx,[rip+0x46924]        # 0x1800640a8
   18001d784:	48 8b cf             	mov    rcx,rdi
   18001d787:	e8 b4 46 03 00       	call   0x180051e40
   18001d78c:	4c 8b 83 08 03 00 00 	mov    r8,QWORD PTR [rbx+0x308]
   18001d793:	48 8d 15 46 6c 04 00 	lea    rdx,[rip+0x46c46]        # 0x1800643e0
   18001d79a:	48 8b cf             	mov    rcx,rdi
   18001d79d:	e8 9e 46 03 00       	call   0x180051e40
   18001d7a2:	44 8b 83 98 00 00 00 	mov    r8d,DWORD PTR [rbx+0x98]
   18001d7a9:	48 8d 15 28 69 04 00 	lea    rdx,[rip+0x46928]        # 0x1800640d8
   18001d7b0:	48 8b cf             	mov    rcx,rdi
   18001d7b3:	e8 98 47 03 00       	call   0x180051f50
   18001d7b8:	44 8b 83 9c 00 00 00 	mov    r8d,DWORD PTR [rbx+0x9c]
   18001d7bf:	48 8d 15 32 69 04 00 	lea    rdx,[rip+0x46932]        # 0x1800640f8
   18001d7c6:	48 8b cf             	mov    rcx,rdi
   18001d7c9:	e8 82 47 03 00       	call   0x180051f50
   18001d7ce:	44 8b 83 a0 00 00 00 	mov    r8d,DWORD PTR [rbx+0xa0]
   18001d7d5:	48 8d 15 3c 69 04 00 	lea    rdx,[rip+0x4693c]        # 0x180064118
   18001d7dc:	48 8b cf             	mov    rcx,rdi
   18001d7df:	e8 6c 47 03 00       	call   0x180051f50
   18001d7e4:	44 8b 83 a4 00 00 00 	mov    r8d,DWORD PTR [rbx+0xa4]
   18001d7eb:	48 8d 15 46 69 04 00 	lea    rdx,[rip+0x46946]        # 0x180064138
   18001d7f2:	48 8b cf             	mov    rcx,rdi
   18001d7f5:	e8 56 47 03 00       	call   0x180051f50
   18001d7fa:	44 8b 83 a8 00 00 00 	mov    r8d,DWORD PTR [rbx+0xa8]
   18001d801:	48 8d 15 50 69 04 00 	lea    rdx,[rip+0x46950]        # 0x180064158
   18001d808:	48 8b cf             	mov    rcx,rdi
   18001d80b:	e8 40 47 03 00       	call   0x180051f50
   18001d810:	44 8b 83 ac 00 00 00 	mov    r8d,DWORD PTR [rbx+0xac]
   18001d817:	48 8d 15 5a 69 04 00 	lea    rdx,[rip+0x4695a]        # 0x180064178
   18001d81e:	48 8b cf             	mov    rcx,rdi
   18001d821:	e8 2a 47 03 00       	call   0x180051f50
   18001d826:	44 8b 83 b0 00 00 00 	mov    r8d,DWORD PTR [rbx+0xb0]
   18001d82d:	48 8d 15 64 69 04 00 	lea    rdx,[rip+0x46964]        # 0x180064198
   18001d834:	48 8b cf             	mov    rcx,rdi
   18001d837:	e8 14 47 03 00       	call   0x180051f50
   18001d83c:	44 8b 83 b4 00 00 00 	mov    r8d,DWORD PTR [rbx+0xb4]
   18001d843:	48 8d 15 76 69 04 00 	lea    rdx,[rip+0x46976]        # 0x1800641c0
   18001d84a:	48 8b cf             	mov    rcx,rdi
   18001d84d:	e8 fe 46 03 00       	call   0x180051f50
   18001d852:	44 8b 83 b8 00 00 00 	mov    r8d,DWORD PTR [rbx+0xb8]
   18001d859:	48 8d 15 88 69 04 00 	lea    rdx,[rip+0x46988]        # 0x1800641e8
   18001d860:	48 8b cf             	mov    rcx,rdi
   18001d863:	e8 e8 46 03 00       	call   0x180051f50
   18001d868:	44 8b 83 bc 00 00 00 	mov    r8d,DWORD PTR [rbx+0xbc]
   18001d86f:	48 8d 15 a2 69 04 00 	lea    rdx,[rip+0x469a2]        # 0x180064218
   18001d876:	48 8b cf             	mov    rcx,rdi
   18001d879:	e8 d2 46 03 00       	call   0x180051f50
   18001d87e:	44 8b 83 c0 00 00 00 	mov    r8d,DWORD PTR [rbx+0xc0]
   18001d885:	48 8d 15 bc 69 04 00 	lea    rdx,[rip+0x469bc]        # 0x180064248
   18001d88c:	48 8b cf             	mov    rcx,rdi
   18001d88f:	e8 bc 46 03 00       	call   0x180051f50
   18001d894:	44 8b 83 c4 00 00 00 	mov    r8d,DWORD PTR [rbx+0xc4]
   18001d89b:	48 8d 15 c6 69 04 00 	lea    rdx,[rip+0x469c6]        # 0x180064268
   18001d8a2:	48 8b cf             	mov    rcx,rdi
   18001d8a5:	e8 a6 46 03 00       	call   0x180051f50
   18001d8aa:	44 8b 43 48          	mov    r8d,DWORD PTR [rbx+0x48]
   18001d8ae:	48 8d 15 d3 69 04 00 	lea    rdx,[rip+0x469d3]        # 0x180064288
   18001d8b5:	48 8b cf             	mov    rcx,rdi
   18001d8b8:	e8 93 46 03 00       	call   0x180051f50
   18001d8bd:	44 8b 43 4c          	mov    r8d,DWORD PTR [rbx+0x4c]
   18001d8c1:	48 8d 15 e8 69 04 00 	lea    rdx,[rip+0x469e8]        # 0x1800642b0
   18001d8c8:	48 8b cf             	mov    rcx,rdi
   18001d8cb:	e8 80 46 03 00       	call   0x180051f50
   18001d8d0:	f3 0f 10 93 38 02 00 	movss  xmm2,DWORD PTR [rbx+0x238]
   18001d8d7:	00 
   18001d8d8:	0f 2e d7             	ucomiss xmm2,xmm7
   18001d8db:	7a 05                	jp     0x18001d8e2
   18001d8dd:	75 03                	jne    0x18001d8e2
   18001d8df:	0f 28 d6             	movaps xmm2,xmm6
   18001d8e2:	48 8d 15 ef 69 04 00 	lea    rdx,[rip+0x469ef]        # 0x1800642d8
   18001d8e9:	48 8b cf             	mov    rcx,rdi
   18001d8ec:	e8 af 45 03 00       	call   0x180051ea0
   18001d8f1:	f3 0f 10 83 3c 02 00 	movss  xmm0,DWORD PTR [rbx+0x23c]
   18001d8f8:	00 
   18001d8f9:	0f 2e c7             	ucomiss xmm0,xmm7
   18001d8fc:	7a 02                	jp     0x18001d900
   18001d8fe:	74 03                	je     0x18001d903
   18001d900:	0f 28 f0             	movaps xmm6,xmm0
   18001d903:	0f 28 d6             	movaps xmm2,xmm6
   18001d906:	48 8d 15 e3 69 04 00 	lea    rdx,[rip+0x469e3]        # 0x1800642f0
   18001d90d:	48 8b cf             	mov    rcx,rdi
   18001d910:	e8 8b 45 03 00       	call   0x180051ea0
   18001d915:	44 8b 83 40 02 00 00 	mov    r8d,DWORD PTR [rbx+0x240]
   18001d91c:	48 8d 15 e5 69 04 00 	lea    rdx,[rip+0x469e5]        # 0x180064308
   18001d923:	48 8b cf             	mov    rcx,rdi
   18001d926:	e8 c5 45 03 00       	call   0x180051ef0
   18001d92b:	44 8b 83 44 02 00 00 	mov    r8d,DWORD PTR [rbx+0x244]
   18001d932:	48 8d 15 ef 69 04 00 	lea    rdx,[rip+0x469ef]        # 0x180064328
   18001d939:	48 8b cf             	mov    rcx,rdi
   18001d93c:	e8 af 45 03 00       	call   0x180051ef0
   18001d941:	4c 8b 83 a8 02 00 00 	mov    r8,QWORD PTR [rbx+0x2a8]
   18001d948:	48 8d 15 39 6c 04 00 	lea    rdx,[rip+0x46c39]        # 0x180064588
   18001d94f:	48 8b cf             	mov    rcx,rdi
   18001d952:	e8 e9 44 03 00       	call   0x180051e40
   18001d957:	4c 8b 03             	mov    r8,QWORD PTR [rbx]
   18001d95a:	48 8d 15 97 6a 04 00 	lea    rdx,[rip+0x46a97]        # 0x1800643f8
   18001d961:	48 8b cf             	mov    rcx,rdi
   18001d964:	e8 d7 44 03 00       	call   0x180051e40
   18001d969:	4c 8b 43 08          	mov    r8,QWORD PTR [rbx+0x8]
   18001d96d:	48 8d 15 a4 6a 04 00 	lea    rdx,[rip+0x46aa4]        # 0x180064418
   18001d974:	48 8b cf             	mov    rcx,rdi
   18001d977:	e8 c4 44 03 00       	call   0x180051e40
   18001d97c:	44 8b 43 78          	mov    r8d,DWORD PTR [rbx+0x78]
   18001d980:	48 8d 15 b1 6a 04 00 	lea    rdx,[rip+0x46ab1]        # 0x180064438
   18001d987:	48 8b cf             	mov    rcx,rdi
   18001d98a:	e8 c1 45 03 00       	call   0x180051f50
   18001d98f:	44 8b 43 7c          	mov    r8d,DWORD PTR [rbx+0x7c]
   18001d993:	48 8d 15 c6 6a 04 00 	lea    rdx,[rip+0x46ac6]        # 0x180064460
   18001d99a:	48 8b cf             	mov    rcx,rdi
   18001d99d:	e8 ae 45 03 00       	call   0x180051f50
   18001d9a2:	44 8b 83 80 00 00 00 	mov    r8d,DWORD PTR [rbx+0x80]
   18001d9a9:	48 8d 15 d8 6a 04 00 	lea    rdx,[rip+0x46ad8]        # 0x180064488
   18001d9b0:	48 8b cf             	mov    rcx,rdi
   18001d9b3:	e8 98 45 03 00       	call   0x180051f50
   18001d9b8:	44 8b 83 84 00 00 00 	mov    r8d,DWORD PTR [rbx+0x84]
   18001d9bf:	48 8d 15 f2 6a 04 00 	lea    rdx,[rip+0x46af2]        # 0x1800644b8
   18001d9c6:	48 8b cf             	mov    rcx,rdi
   18001d9c9:	e8 82 45 03 00       	call   0x180051f50
   18001d9ce:	44 8b 83 88 00 00 00 	mov    r8d,DWORD PTR [rbx+0x88]
   18001d9d5:	48 8d 15 0c 6b 04 00 	lea    rdx,[rip+0x46b0c]        # 0x1800644e8
   18001d9dc:	48 8b cf             	mov    rcx,rdi
   18001d9df:	e8 6c 45 03 00       	call   0x180051f50
   18001d9e4:	44 8b 83 8c 00 00 00 	mov    r8d,DWORD PTR [rbx+0x8c]
   18001d9eb:	48 8d 15 1e 6b 04 00 	lea    rdx,[rip+0x46b1e]        # 0x180064510
   18001d9f2:	48 8b cf             	mov    rcx,rdi
   18001d9f5:	e8 56 45 03 00       	call   0x180051f50
   18001d9fa:	44 8b 83 90 00 00 00 	mov    r8d,DWORD PTR [rbx+0x90]
   18001da01:	48 8d 15 30 6b 04 00 	lea    rdx,[rip+0x46b30]        # 0x180064538
   18001da08:	48 8b cf             	mov    rcx,rdi
   18001da0b:	e8 40 45 03 00       	call   0x180051f50
   18001da10:	44 8b 83 94 00 00 00 	mov    r8d,DWORD PTR [rbx+0x94]
   18001da17:	48 8d 15 42 6b 04 00 	lea    rdx,[rip+0x46b42]        # 0x180064560
   18001da1e:	48 8b cf             	mov    rcx,rdi
   18001da21:	e8 2a 45 03 00       	call   0x180051f50
   18001da26:	4c 8b 43 10          	mov    r8,QWORD PTR [rbx+0x10]
   18001da2a:	48 8d 15 ef 64 04 00 	lea    rdx,[rip+0x464ef]        # 0x180063f20
   18001da31:	48 8b cf             	mov    rcx,rdi
   18001da34:	e8 07 44 03 00       	call   0x180051e40
   18001da39:	4c 8b 43 18          	mov    r8,QWORD PTR [rbx+0x18]
   18001da3d:	48 8d 15 7c 64 04 00 	lea    rdx,[rip+0x4647c]        # 0x180063ec0
   18001da44:	48 8b cf             	mov    rcx,rdi
   18001da47:	e8 f4 43 03 00       	call   0x180051e40
   18001da4c:	4c 8b 83 c8 00 00 00 	mov    r8,QWORD PTR [rbx+0xc8]
   18001da53:	48 8d 15 46 6b 04 00 	lea    rdx,[rip+0x46b46]        # 0x1800645a0
   18001da5a:	48 8b cf             	mov    rcx,rdi
   18001da5d:	e8 de 43 03 00       	call   0x180051e40
   18001da62:	4c 8b 83 d0 00 00 00 	mov    r8,QWORD PTR [rbx+0xd0]
   18001da69:	48 8d 15 48 6b 04 00 	lea    rdx,[rip+0x46b48]        # 0x1800645b8
   18001da70:	48 8b cf             	mov    rcx,rdi
   18001da73:	e8 c8 43 03 00       	call   0x180051e40
   18001da78:	4c 8b 83 d8 00 00 00 	mov    r8,QWORD PTR [rbx+0xd8]
   18001da7f:	48 8d 15 52 6b 04 00 	lea    rdx,[rip+0x46b52]        # 0x1800645d8
   18001da86:	48 8b cf             	mov    rcx,rdi
   18001da89:	e8 b2 43 03 00       	call   0x180051e40
   18001da8e:	4c 8b 83 e0 00 00 00 	mov    r8,QWORD PTR [rbx+0xe0]
   18001da95:	48 8d 15 5c 6b 04 00 	lea    rdx,[rip+0x46b5c]        # 0x1800645f8
   18001da9c:	48 8b cf             	mov    rcx,rdi
   18001da9f:	e8 9c 43 03 00       	call   0x180051e40
   18001daa4:	4c 8b 83 e8 00 00 00 	mov    r8,QWORD PTR [rbx+0xe8]
   18001daab:	48 8d 15 66 6b 04 00 	lea    rdx,[rip+0x46b66]        # 0x180064618
   18001dab2:	48 8b cf             	mov    rcx,rdi
   18001dab5:	e8 86 43 03 00       	call   0x180051e40
   18001daba:	4c 8b 83 f0 00 00 00 	mov    r8,QWORD PTR [rbx+0xf0]
   18001dac1:	48 8d 15 70 6b 04 00 	lea    rdx,[rip+0x46b70]        # 0x180064638
   18001dac8:	48 8b cf             	mov    rcx,rdi
   18001dacb:	e8 70 43 03 00       	call   0x180051e40
   18001dad0:	4c 8b 83 f8 00 00 00 	mov    r8,QWORD PTR [rbx+0xf8]
   18001dad7:	48 8d 15 72 6b 04 00 	lea    rdx,[rip+0x46b72]        # 0x180064650
   18001dade:	48 8b cf             	mov    rcx,rdi
   18001dae1:	e8 5a 43 03 00       	call   0x180051e40
   18001dae6:	4c 8b 83 00 01 00 00 	mov    r8,QWORD PTR [rbx+0x100]
   18001daed:	48 8d 15 74 6b 04 00 	lea    rdx,[rip+0x46b74]        # 0x180064668
   18001daf4:	48 8b cf             	mov    rcx,rdi
   18001daf7:	e8 44 43 03 00       	call   0x180051e40
   18001dafc:	4c 8b 83 08 01 00 00 	mov    r8,QWORD PTR [rbx+0x108]
   18001db03:	48 8d 15 8e 6b 04 00 	lea    rdx,[rip+0x46b8e]        # 0x180064698
   18001db0a:	48 8b cf             	mov    rcx,rdi
   18001db0d:	e8 2e 43 03 00       	call   0x180051e40
   18001db12:	4c 8b 83 10 01 00 00 	mov    r8,QWORD PTR [rbx+0x110]
   18001db19:	48 8d 15 b0 6b 04 00 	lea    rdx,[rip+0x46bb0]        # 0x1800646d0
   18001db20:	48 8b cf             	mov    rcx,rdi
   18001db23:	e8 18 43 03 00       	call   0x180051e40
   18001db28:	4c 8b 83 18 01 00 00 	mov    r8,QWORD PTR [rbx+0x118]
   18001db2f:	48 8d 15 ca 6b 04 00 	lea    rdx,[rip+0x46bca]        # 0x180064700
   18001db36:	48 8b cf             	mov    rcx,rdi
   18001db39:	e8 02 43 03 00       	call   0x180051e40
   18001db3e:	4c 8b 83 20 01 00 00 	mov    r8,QWORD PTR [rbx+0x120]
   18001db45:	48 8d 15 dc 6b 04 00 	lea    rdx,[rip+0x46bdc]        # 0x180064728
   18001db4c:	48 8b cf             	mov    rcx,rdi
   18001db4f:	e8 ec 42 03 00       	call   0x180051e40
   18001db54:	4c 8b 83 28 01 00 00 	mov    r8,QWORD PTR [rbx+0x128]
   18001db5b:	48 8d 15 ee 6b 04 00 	lea    rdx,[rip+0x46bee]        # 0x180064750
   18001db62:	48 8b cf             	mov    rcx,rdi
   18001db65:	e8 d6 42 03 00       	call   0x180051e40
   18001db6a:	4c 8b 83 30 01 00 00 	mov    r8,QWORD PTR [rbx+0x130]
   18001db71:	48 8d 15 00 6c 04 00 	lea    rdx,[rip+0x46c00]        # 0x180064778
   18001db78:	48 8b cf             	mov    rcx,rdi
   18001db7b:	e8 c0 42 03 00       	call   0x180051e40
   18001db80:	4c 8b 83 38 01 00 00 	mov    r8,QWORD PTR [rbx+0x138]
   18001db87:	48 8d 15 02 6c 04 00 	lea    rdx,[rip+0x46c02]        # 0x180064790
   18001db8e:	48 8b cf             	mov    rcx,rdi
   18001db91:	e8 aa 42 03 00       	call   0x180051e40
   18001db96:	4c 8b 83 40 01 00 00 	mov    r8,QWORD PTR [rbx+0x140]
   18001db9d:	48 8d 15 0c 6c 04 00 	lea    rdx,[rip+0x46c0c]        # 0x1800647b0
   18001dba4:	48 8b cf             	mov    rcx,rdi
   18001dba7:	e8 94 42 03 00       	call   0x180051e40
   18001dbac:	4c 8b 83 48 01 00 00 	mov    r8,QWORD PTR [rbx+0x148]
   18001dbb3:	48 8d 15 16 6c 04 00 	lea    rdx,[rip+0x46c16]        # 0x1800647d0
   18001dbba:	48 8b cf             	mov    rcx,rdi
   18001dbbd:	e8 7e 42 03 00       	call   0x180051e40
   18001dbc2:	4c 8b 83 50 01 00 00 	mov    r8,QWORD PTR [rbx+0x150]
   18001dbc9:	48 8d 15 20 6c 04 00 	lea    rdx,[rip+0x46c20]        # 0x1800647f0
   18001dbd0:	48 8b cf             	mov    rcx,rdi
   18001dbd3:	e8 68 42 03 00       	call   0x180051e40
   18001dbd8:	4c 8b 83 58 01 00 00 	mov    r8,QWORD PTR [rbx+0x158]
   18001dbdf:	48 8d 15 2a 6c 04 00 	lea    rdx,[rip+0x46c2a]        # 0x180064810
   18001dbe6:	48 8b cf             	mov    rcx,rdi
   18001dbe9:	e8 52 42 03 00       	call   0x180051e40
   18001dbee:	4c 8b 83 60 01 00 00 	mov    r8,QWORD PTR [rbx+0x160]
   18001dbf5:	48 8d 15 34 6c 04 00 	lea    rdx,[rip+0x46c34]        # 0x180064830
   18001dbfc:	48 8b cf             	mov    rcx,rdi
   18001dbff:	e8 3c 42 03 00       	call   0x180051e40
   18001dc04:	4c 8b 83 68 01 00 00 	mov    r8,QWORD PTR [rbx+0x168]
   18001dc0b:	48 8d 15 3e 6c 04 00 	lea    rdx,[rip+0x46c3e]        # 0x180064850
   18001dc12:	48 8b cf             	mov    rcx,rdi
   18001dc15:	e8 26 42 03 00       	call   0x180051e40
   18001dc1a:	4c 8b 83 70 01 00 00 	mov    r8,QWORD PTR [rbx+0x170]
   18001dc21:	48 8d 15 50 6c 04 00 	lea    rdx,[rip+0x46c50]        # 0x180064878
   18001dc28:	48 8b cf             	mov    rcx,rdi
   18001dc2b:	e8 10 42 03 00       	call   0x180051e40
   18001dc30:	44 8b 83 78 01 00 00 	mov    r8d,DWORD PTR [rbx+0x178]
   18001dc37:	48 8d 15 62 6c 04 00 	lea    rdx,[rip+0x46c62]        # 0x1800648a0
   18001dc3e:	48 8b cf             	mov    rcx,rdi
   18001dc41:	e8 0a 43 03 00       	call   0x180051f50
   18001dc46:	44 8b 83 7c 01 00 00 	mov    r8d,DWORD PTR [rbx+0x17c]
   18001dc4d:	48 8d 15 74 6c 04 00 	lea    rdx,[rip+0x46c74]        # 0x1800648c8
   18001dc54:	48 8b cf             	mov    rcx,rdi
   18001dc57:	e8 f4 42 03 00       	call   0x180051f50
   18001dc5c:	44 8b 83 88 01 00 00 	mov    r8d,DWORD PTR [rbx+0x188]
   18001dc63:	48 8d 15 86 6c 04 00 	lea    rdx,[rip+0x46c86]        # 0x1800648f0
   18001dc6a:	48 8b cf             	mov    rcx,rdi
   18001dc6d:	e8 de 42 03 00       	call   0x180051f50
   18001dc72:	44 8b 83 8c 01 00 00 	mov    r8d,DWORD PTR [rbx+0x18c]
   18001dc79:	48 8d 15 a0 6c 04 00 	lea    rdx,[rip+0x46ca0]        # 0x180064920
   18001dc80:	48 8b cf             	mov    rcx,rdi
   18001dc83:	e8 c8 42 03 00       	call   0x180051f50
   18001dc88:	44 8b 83 80 01 00 00 	mov    r8d,DWORD PTR [rbx+0x180]
   18001dc8f:	48 8d 15 ba 6c 04 00 	lea    rdx,[rip+0x46cba]        # 0x180064950
   18001dc96:	48 8b cf             	mov    rcx,rdi
   18001dc99:	e8 b2 42 03 00       	call   0x180051f50
   18001dc9e:	44 8b 83 84 01 00 00 	mov    r8d,DWORD PTR [rbx+0x184]
   18001dca5:	48 8d 15 d4 6c 04 00 	lea    rdx,[rip+0x46cd4]        # 0x180064980
   18001dcac:	48 8b cf             	mov    rcx,rdi
   18001dcaf:	e8 9c 42 03 00       	call   0x180051f50
   18001dcb4:	44 8b 83 90 01 00 00 	mov    r8d,DWORD PTR [rbx+0x190]
   18001dcbb:	48 8d 15 ee 6c 04 00 	lea    rdx,[rip+0x46cee]        # 0x1800649b0
   18001dcc2:	48 8b cf             	mov    rcx,rdi
   18001dcc5:	e8 86 42 03 00       	call   0x180051f50
   18001dcca:	44 8b 83 94 01 00 00 	mov    r8d,DWORD PTR [rbx+0x194]
   18001dcd1:	48 8d 15 08 6d 04 00 	lea    rdx,[rip+0x46d08]        # 0x1800649e0
   18001dcd8:	48 8b cf             	mov    rcx,rdi
   18001dcdb:	e8 70 42 03 00       	call   0x180051f50
   18001dce0:	44 8b 83 98 01 00 00 	mov    r8d,DWORD PTR [rbx+0x198]
   18001dce7:	48 8d 15 22 6d 04 00 	lea    rdx,[rip+0x46d22]        # 0x180064a10
   18001dcee:	48 8b cf             	mov    rcx,rdi
   18001dcf1:	e8 5a 42 03 00       	call   0x180051f50
   18001dcf6:	44 8b 83 9c 01 00 00 	mov    r8d,DWORD PTR [rbx+0x19c]
   18001dcfd:	48 8d 15 3c 6d 04 00 	lea    rdx,[rip+0x46d3c]        # 0x180064a40
   18001dd04:	48 8b cf             	mov    rcx,rdi
   18001dd07:	e8 44 42 03 00       	call   0x180051f50
   18001dd0c:	44 8b 83 a8 01 00 00 	mov    r8d,DWORD PTR [rbx+0x1a8]
   18001dd13:	48 8d 15 a6 6d 04 00 	lea    rdx,[rip+0x46da6]        # 0x180064ac0
   18001dd1a:	48 8b cf             	mov    rcx,rdi
   18001dd1d:	e8 2e 42 03 00       	call   0x180051f50
   18001dd22:	44 8b 83 ac 01 00 00 	mov    r8d,DWORD PTR [rbx+0x1ac]
   18001dd29:	48 8d 15 b8 6d 04 00 	lea    rdx,[rip+0x46db8]        # 0x180064ae8
   18001dd30:	48 8b cf             	mov    rcx,rdi
   18001dd33:	e8 18 42 03 00       	call   0x180051f50
   18001dd38:	44 8b 83 a0 01 00 00 	mov    r8d,DWORD PTR [rbx+0x1a0]
   18001dd3f:	48 8d 15 2a 6d 04 00 	lea    rdx,[rip+0x46d2a]        # 0x180064a70
   18001dd46:	48 8b cf             	mov    rcx,rdi
   18001dd49:	e8 02 42 03 00       	call   0x180051f50
   18001dd4e:	44 8b 83 a4 01 00 00 	mov    r8d,DWORD PTR [rbx+0x1a4]
   18001dd55:	48 8d 15 3c 6d 04 00 	lea    rdx,[rip+0x46d3c]        # 0x180064a98
   18001dd5c:	48 8b cf             	mov    rcx,rdi
   18001dd5f:	e8 ec 41 03 00       	call   0x180051f50
   18001dd64:	44 8b 83 b0 01 00 00 	mov    r8d,DWORD PTR [rbx+0x1b0]
   18001dd6b:	48 8d 15 9e 6d 04 00 	lea    rdx,[rip+0x46d9e]        # 0x180064b10
   18001dd72:	48 8b cf             	mov    rcx,rdi
   18001dd75:	e8 d6 41 03 00       	call   0x180051f50
   18001dd7a:	44 8b 83 b4 01 00 00 	mov    r8d,DWORD PTR [rbx+0x1b4]
   18001dd81:	48 8d 15 c8 6d 04 00 	lea    rdx,[rip+0x46dc8]        # 0x180064b50
   18001dd88:	48 8b cf             	mov    rcx,rdi
   18001dd8b:	e8 c0 41 03 00       	call   0x180051f50
   18001dd90:	44 8b 83 f8 01 00 00 	mov    r8d,DWORD PTR [rbx+0x1f8]
   18001dd97:	48 8d 15 f2 6d 04 00 	lea    rdx,[rip+0x46df2]        # 0x180064b90
   18001dd9e:	48 8b cf             	mov    rcx,rdi
   18001dda1:	e8 aa 41 03 00       	call   0x180051f50
   18001dda6:	44 8b 83 fc 01 00 00 	mov    r8d,DWORD PTR [rbx+0x1fc]
   18001ddad:	48 8d 15 1c 6e 04 00 	lea    rdx,[rip+0x46e1c]        # 0x180064bd0
   18001ddb4:	48 8b cf             	mov    rcx,rdi
   18001ddb7:	e8 94 41 03 00       	call   0x180051f50
   18001ddbc:	44 8b 83 00 02 00 00 	mov    r8d,DWORD PTR [rbx+0x200]
   18001ddc3:	48 8d 15 46 6e 04 00 	lea    rdx,[rip+0x46e46]        # 0x180064c10
   18001ddca:	48 8b cf             	mov    rcx,rdi
   18001ddcd:	e8 7e 41 03 00       	call   0x180051f50
   18001ddd2:	44 8b 83 04 02 00 00 	mov    r8d,DWORD PTR [rbx+0x204]
   18001ddd9:	48 8d 15 70 6e 04 00 	lea    rdx,[rip+0x46e70]        # 0x180064c50
   18001dde0:	48 8b cf             	mov    rcx,rdi
   18001dde3:	e8 68 41 03 00       	call   0x180051f50
   18001dde8:	44 8b 83 b8 01 00 00 	mov    r8d,DWORD PTR [rbx+0x1b8]
   18001ddef:	48 8d 15 9a 6e 04 00 	lea    rdx,[rip+0x46e9a]        # 0x180064c90
   18001ddf6:	48 8b cf             	mov    rcx,rdi
   18001ddf9:	e8 52 41 03 00       	call   0x180051f50
   18001ddfe:	44 8b 83 bc 01 00 00 	mov    r8d,DWORD PTR [rbx+0x1bc]
   18001de05:	48 8d 15 b4 6e 04 00 	lea    rdx,[rip+0x46eb4]        # 0x180064cc0
   18001de0c:	48 8b cf             	mov    rcx,rdi
   18001de0f:	e8 3c 41 03 00       	call   0x180051f50
   18001de14:	44 8b 83 08 02 00 00 	mov    r8d,DWORD PTR [rbx+0x208]
   18001de1b:	48 8d 15 ce 6e 04 00 	lea    rdx,[rip+0x46ece]        # 0x180064cf0
   18001de22:	48 8b cf             	mov    rcx,rdi
   18001de25:	e8 26 41 03 00       	call   0x180051f50
   18001de2a:	44 8b 83 0c 02 00 00 	mov    r8d,DWORD PTR [rbx+0x20c]
   18001de31:	48 8d 15 f0 6e 04 00 	lea    rdx,[rip+0x46ef0]        # 0x180064d28
   18001de38:	48 8b cf             	mov    rcx,rdi
   18001de3b:	e8 10 41 03 00       	call   0x180051f50
   18001de40:	44 8b 83 10 02 00 00 	mov    r8d,DWORD PTR [rbx+0x210]
   18001de47:	48 8d 15 12 6f 04 00 	lea    rdx,[rip+0x46f12]        # 0x180064d60
   18001de4e:	48 8b cf             	mov    rcx,rdi
   18001de51:	e8 fa 40 03 00       	call   0x180051f50
   18001de56:	44 8b 83 14 02 00 00 	mov    r8d,DWORD PTR [rbx+0x214]
   18001de5d:	48 8d 15 34 6f 04 00 	lea    rdx,[rip+0x46f34]        # 0x180064d98
   18001de64:	48 8b cf             	mov    rcx,rdi
   18001de67:	e8 e4 40 03 00       	call   0x180051f50
   18001de6c:	44 8b 83 c0 01 00 00 	mov    r8d,DWORD PTR [rbx+0x1c0]
   18001de73:	48 8d 15 56 6f 04 00 	lea    rdx,[rip+0x46f56]        # 0x180064dd0
   18001de7a:	48 8b cf             	mov    rcx,rdi
   18001de7d:	e8 ce 40 03 00       	call   0x180051f50
   18001de82:	44 8b 83 c4 01 00 00 	mov    r8d,DWORD PTR [rbx+0x1c4]
   18001de89:	48 8d 15 68 6f 04 00 	lea    rdx,[rip+0x46f68]        # 0x180064df8
   18001de90:	48 8b cf             	mov    rcx,rdi
   18001de93:	e8 b8 40 03 00       	call   0x180051f50
   18001de98:	44 8b 83 18 02 00 00 	mov    r8d,DWORD PTR [rbx+0x218]
   18001de9f:	48 8d 15 7a 6f 04 00 	lea    rdx,[rip+0x46f7a]        # 0x180064e20
   18001dea6:	48 8b cf             	mov    rcx,rdi
   18001dea9:	e8 a2 40 03 00       	call   0x180051f50
   18001deae:	44 8b 83 1c 02 00 00 	mov    r8d,DWORD PTR [rbx+0x21c]
   18001deb5:	48 8d 15 94 6f 04 00 	lea    rdx,[rip+0x46f94]        # 0x180064e50
   18001debc:	48 8b cf             	mov    rcx,rdi
   18001debf:	e8 8c 40 03 00       	call   0x180051f50
   18001dec4:	44 8b 83 20 02 00 00 	mov    r8d,DWORD PTR [rbx+0x220]
   18001decb:	48 8d 15 ae 6f 04 00 	lea    rdx,[rip+0x46fae]        # 0x180064e80
   18001ded2:	48 8b cf             	mov    rcx,rdi
   18001ded5:	e8 76 40 03 00       	call   0x180051f50
   18001deda:	44 8b 83 24 02 00 00 	mov    r8d,DWORD PTR [rbx+0x224]
   18001dee1:	48 8d 15 c8 6f 04 00 	lea    rdx,[rip+0x46fc8]        # 0x180064eb0
   18001dee8:	48 8b cf             	mov    rcx,rdi
   18001deeb:	e8 60 40 03 00       	call   0x180051f50
   18001def0:	44 8b 83 c8 01 00 00 	mov    r8d,DWORD PTR [rbx+0x1c8]
   18001def7:	48 8d 15 e2 6f 04 00 	lea    rdx,[rip+0x46fe2]        # 0x180064ee0
   18001defe:	48 8b cf             	mov    rcx,rdi
   18001df01:	e8 4a 40 03 00       	call   0x180051f50
   18001df06:	44 8b 83 cc 01 00 00 	mov    r8d,DWORD PTR [rbx+0x1cc]
   18001df0d:	48 8d 15 f4 6f 04 00 	lea    rdx,[rip+0x46ff4]        # 0x180064f08
   18001df14:	48 8b cf             	mov    rcx,rdi
   18001df17:	e8 34 40 03 00       	call   0x180051f50
   18001df1c:	44 8b 83 d0 01 00 00 	mov    r8d,DWORD PTR [rbx+0x1d0]
   18001df23:	48 8d 15 06 70 04 00 	lea    rdx,[rip+0x47006]        # 0x180064f30
   18001df2a:	48 8b cf             	mov    rcx,rdi
   18001df2d:	e8 1e 40 03 00       	call   0x180051f50
   18001df32:	44 8b 83 d4 01 00 00 	mov    r8d,DWORD PTR [rbx+0x1d4]
   18001df39:	48 8d 15 20 70 04 00 	lea    rdx,[rip+0x47020]        # 0x180064f60
   18001df40:	48 8b cf             	mov    rcx,rdi
   18001df43:	e8 08 40 03 00       	call   0x180051f50
   18001df48:	44 8b 83 d8 01 00 00 	mov    r8d,DWORD PTR [rbx+0x1d8]
   18001df4f:	48 8d 15 3a 70 04 00 	lea    rdx,[rip+0x4703a]        # 0x180064f90
   18001df56:	48 8b cf             	mov    rcx,rdi
   18001df59:	e8 f2 3f 03 00       	call   0x180051f50
   18001df5e:	44 8b 83 dc 01 00 00 	mov    r8d,DWORD PTR [rbx+0x1dc]
   18001df65:	48 8d 15 54 70 04 00 	lea    rdx,[rip+0x47054]        # 0x180064fc0
   18001df6c:	48 8b cf             	mov    rcx,rdi
   18001df6f:	e8 dc 3f 03 00       	call   0x180051f50
   18001df74:	44 8b 83 e0 01 00 00 	mov    r8d,DWORD PTR [rbx+0x1e0]
   18001df7b:	48 8d 15 6e 70 04 00 	lea    rdx,[rip+0x4706e]        # 0x180064ff0
   18001df82:	48 8b cf             	mov    rcx,rdi
   18001df85:	e8 c6 3f 03 00       	call   0x180051f50
   18001df8a:	44 8b 83 e4 01 00 00 	mov    r8d,DWORD PTR [rbx+0x1e4]
   18001df91:	48 8d 15 88 70 04 00 	lea    rdx,[rip+0x47088]        # 0x180065020
   18001df98:	48 8b cf             	mov    rcx,rdi
   18001df9b:	e8 b0 3f 03 00       	call   0x180051f50
   18001dfa0:	44 8b 83 e8 01 00 00 	mov    r8d,DWORD PTR [rbx+0x1e8]
   18001dfa7:	48 8d 15 a2 70 04 00 	lea    rdx,[rip+0x470a2]        # 0x180065050
   18001dfae:	48 8b cf             	mov    rcx,rdi
   18001dfb1:	e8 9a 3f 03 00       	call   0x180051f50
   18001dfb6:	44 8b 83 ec 01 00 00 	mov    r8d,DWORD PTR [rbx+0x1ec]
   18001dfbd:	48 8d 15 c4 70 04 00 	lea    rdx,[rip+0x470c4]        # 0x180065088
   18001dfc4:	48 8b cf             	mov    rcx,rdi
   18001dfc7:	e8 84 3f 03 00       	call   0x180051f50
   18001dfcc:	44 8b 83 f0 01 00 00 	mov    r8d,DWORD PTR [rbx+0x1f0]
   18001dfd3:	48 8d 15 e6 70 04 00 	lea    rdx,[rip+0x470e6]        # 0x1800650c0
   18001dfda:	48 8b cf             	mov    rcx,rdi
   18001dfdd:	e8 6e 3f 03 00       	call   0x180051f50
   18001dfe2:	44 8b 83 f4 01 00 00 	mov    r8d,DWORD PTR [rbx+0x1f4]
   18001dfe9:	48 8d 15 08 71 04 00 	lea    rdx,[rip+0x47108]        # 0x1800650f8
   18001dff0:	48 8b cf             	mov    rcx,rdi
   18001dff3:	e8 58 3f 03 00       	call   0x180051f50
   18001dff8:	4c 8b 83 28 02 00 00 	mov    r8,QWORD PTR [rbx+0x228]
   18001dfff:	48 8d 15 2a 71 04 00 	lea    rdx,[rip+0x4712a]        # 0x180065130
   18001e006:	48 8b cf             	mov    rcx,rdi
   18001e009:	e8 02 40 03 00       	call   0x180052010
   18001e00e:	4c 8b 83 30 02 00 00 	mov    r8,QWORD PTR [rbx+0x230]
   18001e015:	48 8d 15 2c 71 04 00 	lea    rdx,[rip+0x4712c]        # 0x180065148
   18001e01c:	48 8b cf             	mov    rcx,rdi
   18001e01f:	e8 ec 3f 03 00       	call   0x180052010
   18001e024:	4c 8b 83 10 03 00 00 	mov    r8,QWORD PTR [rbx+0x310]
   18001e02b:	48 8d 15 2e 71 04 00 	lea    rdx,[rip+0x4712e]        # 0x180065160
   18001e032:	48 8b cf             	mov    rcx,rdi
   18001e035:	e8 06 3e 03 00       	call   0x180051e40
   18001e03a:	4c 8b 83 20 03 00 00 	mov    r8,QWORD PTR [rbx+0x320]
   18001e041:	48 8d 15 30 71 04 00 	lea    rdx,[rip+0x47130]        # 0x180065178
   18001e048:	48 8b cf             	mov    rcx,rdi
   18001e04b:	e8 f0 3d 03 00       	call   0x180051e40
   18001e050:	4c 8b 83 30 03 00 00 	mov    r8,QWORD PTR [rbx+0x330]
   18001e057:	48 8d 15 3a 71 04 00 	lea    rdx,[rip+0x4713a]        # 0x180065198
   18001e05e:	48 8b cf             	mov    rcx,rdi
   18001e061:	e8 da 3d 03 00       	call   0x180051e40
   18001e066:	4c 8b 83 40 03 00 00 	mov    r8,QWORD PTR [rbx+0x340]
   18001e06d:	48 8d 15 44 71 04 00 	lea    rdx,[rip+0x47144]        # 0x1800651b8
   18001e074:	48 8b cf             	mov    rcx,rdi
   18001e077:	e8 c4 3d 03 00       	call   0x180051e40
   18001e07c:	44 8b 83 18 03 00 00 	mov    r8d,DWORD PTR [rbx+0x318]
   18001e083:	48 8d 15 46 71 04 00 	lea    rdx,[rip+0x47146]        # 0x1800651d0
   18001e08a:	48 8b cf             	mov    rcx,rdi
   18001e08d:	e8 be 3e 03 00       	call   0x180051f50
   18001e092:	44 8b 83 1c 03 00 00 	mov    r8d,DWORD PTR [rbx+0x31c]
   18001e099:	48 8d 15 58 71 04 00 	lea    rdx,[rip+0x47158]        # 0x1800651f8
   18001e0a0:	48 8b cf             	mov    rcx,rdi
   18001e0a3:	e8 a8 3e 03 00       	call   0x180051f50
   18001e0a8:	44 8b 83 28 03 00 00 	mov    r8d,DWORD PTR [rbx+0x328]
   18001e0af:	48 8d 15 6a 71 04 00 	lea    rdx,[rip+0x4716a]        # 0x180065220
   18001e0b6:	48 8b cf             	mov    rcx,rdi
   18001e0b9:	e8 92 3e 03 00       	call   0x180051f50
   18001e0be:	44 8b 83 2c 03 00 00 	mov    r8d,DWORD PTR [rbx+0x32c]
   18001e0c5:	48 8d 15 84 71 04 00 	lea    rdx,[rip+0x47184]        # 0x180065250
   18001e0cc:	48 8b cf             	mov    rcx,rdi
   18001e0cf:	e8 7c 3e 03 00       	call   0x180051f50
   18001e0d4:	44 8b 83 38 03 00 00 	mov    r8d,DWORD PTR [rbx+0x338]
   18001e0db:	48 8d 15 9e 71 04 00 	lea    rdx,[rip+0x4719e]        # 0x180065280
   18001e0e2:	48 8b cf             	mov    rcx,rdi
   18001e0e5:	e8 66 3e 03 00       	call   0x180051f50
   18001e0ea:	44 8b 83 3c 03 00 00 	mov    r8d,DWORD PTR [rbx+0x33c]
   18001e0f1:	48 8d 15 b8 71 04 00 	lea    rdx,[rip+0x471b8]        # 0x1800652b0
   18001e0f8:	48 8b cf             	mov    rcx,rdi
   18001e0fb:	e8 50 3e 03 00       	call   0x180051f50
   18001e100:	44 8b 83 48 03 00 00 	mov    r8d,DWORD PTR [rbx+0x348]
   18001e107:	48 8d 15 d2 71 04 00 	lea    rdx,[rip+0x471d2]        # 0x1800652e0
   18001e10e:	48 8b cf             	mov    rcx,rdi
   18001e111:	e8 3a 3e 03 00       	call   0x180051f50
   18001e116:	44 8b 83 4c 03 00 00 	mov    r8d,DWORD PTR [rbx+0x34c]
   18001e11d:	48 8d 15 e4 71 04 00 	lea    rdx,[rip+0x471e4]        # 0x180065308
   18001e124:	48 8b cf             	mov    rcx,rdi
   18001e127:	e8 24 3e 03 00       	call   0x180051f50
   18001e12c:	45 33 c9             	xor    r9d,r9d
   18001e12f:	4c 8b c7             	mov    r8,rdi
   18001e132:	48 8b d6             	mov    rdx,rsi
   18001e135:	48 8b cd             	mov    rcx,rbp
   18001e138:	48 8b 5c 24 50       	mov    rbx,QWORD PTR [rsp+0x50]
   18001e13d:	48 8b 6c 24 58       	mov    rbp,QWORD PTR [rsp+0x58]
   18001e142:	48 8b 74 24 60       	mov    rsi,QWORD PTR [rsp+0x60]
   18001e147:	0f 28 74 24 30       	movaps xmm6,XMMWORD PTR [rsp+0x30]
   18001e14c:	0f 28 7c 24 20       	movaps xmm7,XMMWORD PTR [rsp+0x20]
   18001e151:	48 83 c4 40          	add    rsp,0x40
   18001e155:	5f                   	pop    rdi
   18001e156:	e9 c5 48 03 00       	jmp    0x180052a20
