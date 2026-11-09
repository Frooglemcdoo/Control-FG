
/workspace/scratch/65fb49337344/upload/d3d_rmdwin10_f(4).dll:     file format pei-x86-64


Disassembly of section .text:

000000018003a3e0 <.text+0x393e0>:
   18003a3e0:	40 57                	rex push rdi
   18003a3e2:	48 83 ec 30          	sub    rsp,0x30
   18003a3e6:	48 c7 44 24 20 fe ff 	mov    QWORD PTR [rsp+0x20],0xfffffffffffffffe
   18003a3ed:	ff ff 
   18003a3ef:	48 89 5c 24 40       	mov    QWORD PTR [rsp+0x40],rbx
   18003a3f4:	48 8b d9             	mov    rbx,rcx
   18003a3f7:	48 83 b9 98 00 00 00 	cmp    QWORD PTR [rcx+0x98],0x0
   18003a3fe:	00 
   18003a3ff:	74 0d                	je     0x18003a40e
   18003a401:	48 8b 91 98 00 00 00 	mov    rdx,QWORD PTR [rcx+0x98]
   18003a408:	e8 93 10 fe ff       	call   0x18001b4a0
   18003a40d:	90                   	nop
   18003a40e:	48 83 bb 90 00 00 00 	cmp    QWORD PTR [rbx+0x90],0x0
   18003a415:	00 
   18003a416:	74 0d                	je     0x18003a425
   18003a418:	48 8b 93 90 00 00 00 	mov    rdx,QWORD PTR [rbx+0x90]
   18003a41f:	e8 ac 0f fe ff       	call   0x18001b3d0
   18003a424:	90                   	nop
   18003a425:	48 83 bb a8 00 00 00 	cmp    QWORD PTR [rbx+0xa8],0x0
   18003a42c:	00 
   18003a42d:	74 0d                	je     0x18003a43c
   18003a42f:	48 8b 93 a8 00 00 00 	mov    rdx,QWORD PTR [rbx+0xa8]
   18003a436:	e8 35 11 fe ff       	call   0x18001b570
   18003a43b:	90                   	nop
   18003a43c:	48 83 bb a0 00 00 00 	cmp    QWORD PTR [rbx+0xa0],0x0
   18003a443:	00 
   18003a444:	74 0d                	je     0x18003a453
   18003a446:	48 8b 93 a0 00 00 00 	mov    rdx,QWORD PTR [rbx+0xa0]
   18003a44d:	e8 ee 11 fe ff       	call   0x18001b640
   18003a452:	90                   	nop
   18003a453:	33 ff                	xor    edi,edi
   18003a455:	48 89 bb 98 00 00 00 	mov    QWORD PTR [rbx+0x98],rdi
   18003a45c:	48 89 bb 90 00 00 00 	mov    QWORD PTR [rbx+0x90],rdi
   18003a463:	48 89 bb a8 00 00 00 	mov    QWORD PTR [rbx+0xa8],rdi
   18003a46a:	48 89 bb a0 00 00 00 	mov    QWORD PTR [rbx+0xa0],rdi
   18003a471:	48 8b 8b b0 00 00 00 	mov    rcx,QWORD PTR [rbx+0xb0]
   18003a478:	48 85 c9             	test   rcx,rcx
   18003a47b:	74 0d                	je     0x18003a48a
   18003a47d:	48 8d 15 8c fe ff ff 	lea    rdx,[rip+0xfffffffffffffe8c]        # 0x18003a310
   18003a484:	e8 77 a0 fe ff       	call   0x180024500
   18003a489:	90                   	nop
   18003a48a:	48 89 bb b0 00 00 00 	mov    QWORD PTR [rbx+0xb0],rdi
   18003a491:	48 8b 8b b8 00 00 00 	mov    rcx,QWORD PTR [rbx+0xb8]
   18003a498:	48 85 c9             	test   rcx,rcx
   18003a49b:	74 0d                	je     0x18003a4aa
   18003a49d:	48 8d 15 7c fe ff ff 	lea    rdx,[rip+0xfffffffffffffe7c]        # 0x18003a320
   18003a4a4:	e8 57 a0 fe ff       	call   0x180024500
   18003a4a9:	90                   	nop
   18003a4aa:	48 89 bb b8 00 00 00 	mov    QWORD PTR [rbx+0xb8],rdi
   18003a4b1:	48 8b 8b c0 00 00 00 	mov    rcx,QWORD PTR [rbx+0xc0]
   18003a4b8:	48 85 c9             	test   rcx,rcx
   18003a4bb:	74 0d                	je     0x18003a4ca
   18003a4bd:	48 8d 15 6c fe ff ff 	lea    rdx,[rip+0xfffffffffffffe6c]        # 0x18003a330
   18003a4c4:	e8 37 a0 fe ff       	call   0x180024500
   18003a4c9:	90                   	nop
   18003a4ca:	48 89 bb c0 00 00 00 	mov    QWORD PTR [rbx+0xc0],rdi
   18003a4d1:	48 8b 8b 88 00 00 00 	mov    rcx,QWORD PTR [rbx+0x88]
   18003a4d8:	e8 53 9f fe ff       	call   0x180024430
   18003a4dd:	90                   	nop
   18003a4de:	48 8b 8b 88 00 00 00 	mov    rcx,QWORD PTR [rbx+0x88]
   18003a4e5:	48 85 c9             	test   rcx,rcx
   18003a4e8:	74 07                	je     0x18003a4f1
   18003a4ea:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   18003a4ed:	ff 50 10             	call   QWORD PTR [rax+0x10]
   18003a4f0:	90                   	nop
   18003a4f1:	89 7b 78             	mov    DWORD PTR [rbx+0x78],edi
   18003a4f4:	f7 43 7c 00 00 00 40 	test   DWORD PTR [rbx+0x7c],0x40000000
   18003a4fb:	75 16                	jne    0x18003a513
   18003a4fd:	48 8b 4b 70          	mov    rcx,QWORD PTR [rbx+0x70]
   18003a501:	ff 15 79 33 02 00    	call   QWORD PTR [rip+0x23379]        # 0x18005d880
   18003a507:	90                   	nop
   18003a508:	48 89 7b 70          	mov    QWORD PTR [rbx+0x70],rdi
   18003a50c:	81 63 7c 00 00 00 c0 	and    DWORD PTR [rbx+0x7c],0xc0000000
   18003a513:	48 8b 5c 24 40       	mov    rbx,QWORD PTR [rsp+0x40]
   18003a518:	48 83 c4 30          	add    rsp,0x30
   18003a51c:	5f                   	pop    rdi
   18003a51d:	c3                   	ret
