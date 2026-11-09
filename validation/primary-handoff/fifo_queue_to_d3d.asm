
../../upload/d3d_rmdwin10_f(4).dll:     file format pei-x86-64


Disassembly of section .text:

00000001800434e9 <.text+0x424e9>:
   1800434e9:	41 8b 7d 30          	mov    edi,DWORD PTR [r13+0x30]
   1800434ed:	85 ff                	test   edi,edi
   1800434ef:	0f 84 ea 00 00 00    	je     0x1800435df
   1800434f5:	4d 8b 7d 28          	mov    r15,QWORD PTR [r13+0x28]
   1800434f9:	4d 8b 65 10          	mov    r12,QWORD PTR [r13+0x10]
   1800434fd:	44 8b f7             	mov    r14d,edi
   180043500:	45 33 ed             	xor    r13d,r13d
   180043503:	4c 89 6c 24 28       	mov    QWORD PTR [rsp+0x28],r13
   180043508:	4c 89 6c 24 30       	mov    QWORD PTR [rsp+0x30],r13
   18004350d:	33 db                	xor    ebx,ebx
   18004350f:	85 ff                	test   edi,edi
   180043511:	74 3b                	je     0x18004354e
   180043513:	49 8b 04 df          	mov    rax,QWORD PTR [r15+rbx*8]
   180043517:	48 c7 40 08 00 00 00 	mov    QWORD PTR [rax+0x8],0x0
   18004351e:	00 
   18004351f:	49 8b 04 df          	mov    rax,QWORD PTR [r15+rbx*8]
   180043523:	48 8b 48 18          	mov    rcx,QWORD PTR [rax+0x18]
   180043527:	48 89 8c 24 88 00 00 	mov    QWORD PTR [rsp+0x88],rcx
   18004352e:	00 
   18004352f:	48 8d 94 24 88 00 00 	lea    rdx,[rsp+0x88]
   180043536:	00 
   180043537:	48 8d 4c 24 28       	lea    rcx,[rsp+0x28]
   18004353c:	e8 5f ee fb ff       	call   0x1800023a0
   180043541:	48 ff c3             	inc    rbx
   180043544:	49 3b de             	cmp    rbx,r14
   180043547:	72 ca                	jb     0x180043513
   180043549:	4c 8b 6c 24 28       	mov    r13,QWORD PTR [rsp+0x28]
   18004354e:	49 8b 04 24          	mov    rax,QWORD PTR [r12]
   180043552:	4d 8b c5             	mov    r8,r13
   180043555:	8b d7                	mov    edx,edi
   180043557:	49 8b cc             	mov    rcx,r12
   18004355a:	ff 50 50             	call   QWORD PTR [rax+0x50]
