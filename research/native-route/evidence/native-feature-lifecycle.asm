
upload/d3d_rmdwin10_f(4).dll:     file format pei-x86-64


Disassembly of section .text:

000000018001e1d0 <.text+0x1d1d0>:
   18001e1d0:	40 55                	rex push rbp
   18001e1d2:	56                   	push   rsi
   18001e1d3:	57                   	push   rdi
   18001e1d4:	41 54                	push   r12
   18001e1d6:	41 55                	push   r13
   18001e1d8:	41 56                	push   r14
   18001e1da:	41 57                	push   r15
   18001e1dc:	48 8d 6c 24 f9       	lea    rbp,[rsp-0x7]
   18001e1e1:	48 81 ec d0 00 00 00 	sub    rsp,0xd0
   18001e1e8:	48 c7 45 cf fe ff ff 	mov    QWORD PTR [rbp-0x31],0xfffffffffffffffe
   18001e1ef:	ff 
   18001e1f0:	48 89 9c 24 10 01 00 	mov    QWORD PTR [rsp+0x110],rbx
   18001e1f7:	00 
   18001e1f8:	48 8b 05 f1 89 0d 00 	mov    rax,QWORD PTR [rip+0xd89f1]        # 0x1800f6bf0
   18001e1ff:	48 33 c4             	xor    rax,rsp
   18001e202:	48 89 45 f7          	mov    QWORD PTR [rbp-0x9],rax
   18001e206:	44 89 4c 24 44       	mov    DWORD PTR [rsp+0x44],r9d
   18001e20b:	41 8b c0             	mov    eax,r8d
   18001e20e:	89 44 24 40          	mov    DWORD PTR [rsp+0x40],eax
   18001e212:	48 8b f2             	mov    rsi,rdx
   18001e215:	48 8b 1d c4 39 0f 00 	mov    rbx,QWORD PTR [rip+0xf39c4]        # 0x180111be0
   18001e21c:	89 45 df             	mov    DWORD PTR [rbp-0x21],eax
   18001e21f:	44 89 4d e3          	mov    DWORD PTR [rbp-0x1d],r9d
   18001e223:	8b 45 67             	mov    eax,DWORD PTR [rbp+0x67]
   18001e226:	89 45 e7             	mov    DWORD PTR [rbp-0x19],eax
   18001e229:	8b 45 6f             	mov    eax,DWORD PTR [rbp+0x6f]
   18001e22c:	89 45 eb             	mov    DWORD PTR [rbp-0x15],eax
   18001e22f:	44 0f b6 65 7f       	movzx  r12d,BYTE PTR [rbp+0x7f]
   18001e234:	44 88 65 ef          	mov    BYTE PTR [rbp-0x11],r12b
   18001e238:	0f b6 85 87 00 00 00 	movzx  eax,BYTE PTR [rbp+0x87]
   18001e23f:	88 45 f0             	mov    BYTE PTR [rbp-0x10],al
   18001e242:	0f b6 85 8f 00 00 00 	movzx  eax,BYTE PTR [rbp+0x8f]
   18001e249:	88 45 f1             	mov    BYTE PTR [rbp-0xf],al
   18001e24c:	0f b6 85 97 00 00 00 	movzx  eax,BYTE PTR [rbp+0x97]
   18001e253:	88 45 f2             	mov    BYTE PTR [rbp-0xe],al
   18001e256:	ba 14 00 00 00       	mov    edx,0x14
   18001e25b:	48 8d 4d df          	lea    rcx,[rbp-0x21]
   18001e25f:	ff 15 33 f4 03 00    	call   QWORD PTR [rip+0x3f433]        # 0x18005d698
   18001e265:	48 89 45 9f          	mov    QWORD PTR [rbp-0x61],rax
   18001e269:	4c 8d 73 40          	lea    r14,[rbx+0x40]
   18001e26d:	49 8b 46 38          	mov    rax,QWORD PTR [r14+0x38]
   18001e271:	49 3b 46 30          	cmp    rax,QWORD PTR [r14+0x30]
   18001e275:	75 12                	jne    0x18001e289
   18001e277:	49 8b 46 40          	mov    rax,QWORD PTR [r14+0x40]
   18001e27b:	48 8d 0c 40          	lea    rcx,[rax+rax*2]
   18001e27f:	49 8b 46 60          	mov    rax,QWORD PTR [r14+0x60]
   18001e283:	4c 8d 04 c8          	lea    r8,[rax+rcx*8]
   18001e287:	eb 28                	jmp    0x18001e2b1
   18001e289:	4c 8d 45 9f          	lea    r8,[rbp-0x61]
   18001e28d:	48 8d 54 24 48       	lea    rdx,[rsp+0x48]
   18001e292:	49 8b ce             	mov    rcx,r14
   18001e295:	e8 96 1b 00 00       	call   0x18001fe30
   18001e29a:	48 8b 54 24 48       	mov    rdx,QWORD PTR [rsp+0x48]
   18001e29f:	48 83 fa ff          	cmp    rdx,0xffffffffffffffff
   18001e2a3:	74 d2                	je     0x18001e277
   18001e2a5:	48 8d 14 52          	lea    rdx,[rdx+rdx*2]
   18001e2a9:	49 8b 4e 60          	mov    rcx,QWORD PTR [r14+0x60]
   18001e2ad:	4c 8d 04 d1          	lea    r8,[rcx+rdx*8]
   18001e2b1:	48 8b 8b 80 00 00 00 	mov    rcx,QWORD PTR [rbx+0x80]
   18001e2b8:	48 8d 14 49          	lea    rdx,[rcx+rcx*2]
   18001e2bc:	48 8b 8b a0 00 00 00 	mov    rcx,QWORD PTR [rbx+0xa0]
   18001e2c3:	48 8d 14 d1          	lea    rdx,[rcx+rdx*8]
   18001e2c7:	4c 3b c2             	cmp    r8,rdx
   18001e2ca:	74 0d                	je     0x18001e2d9
   18001e2cc:	41 0f 10 40 08       	movups xmm0,XMMWORD PTR [r8+0x8]
   18001e2d1:	0f 11 06             	movups XMMWORD PTR [rsi],xmm0
   18001e2d4:	e9 53 03 00 00       	jmp    0x18001e62c
   18001e2d9:	45 33 ed             	xor    r13d,r13d
   18001e2dc:	4c 89 6c 24 30       	mov    QWORD PTR [rsp+0x30],r13
   18001e2e1:	48 8d 4c 24 30       	lea    rcx,[rsp+0x30]
   18001e2e6:	e8 85 45 03 00       	call   0x180052870
   18001e2eb:	25 00 00 f0 ff       	and    eax,0xfff00000
   18001e2f0:	3d 00 00 d0 ba       	cmp    eax,0xbad00000
   18001e2f5:	75 0c                	jne    0x18001e303
   18001e2f7:	4c 89 2e             	mov    QWORD PTR [rsi],r13
   18001e2fa:	4c 89 6e 08          	mov    QWORD PTR [rsi+0x8],r13
   18001e2fe:	e9 29 03 00 00       	jmp    0x18001e62c
   18001e303:	8b 0d f3 32 0f 00    	mov    ecx,DWORD PTR [rip+0xf32f3]        # 0x1801115fc
   18001e309:	65 48 8b 04 25 58 00 	mov    rax,QWORD PTR gs:0x58
   18001e310:	00 00 
   18001e312:	48 8b 1c c8          	mov    rbx,QWORD PTR [rax+rcx*8]
   18001e316:	41 bf 08 00 00 00    	mov    r15d,0x8
   18001e31c:	4c 03 fb             	add    r15,rbx
   18001e31f:	49 8b 07             	mov    rax,QWORD PTR [r15]
   18001e322:	48 85 c0             	test   rax,rax
   18001e325:	74 0b                	je     0x18001e332
   18001e327:	b9 10 00 00 00       	mov    ecx,0x10
   18001e32c:	48 8b 1c 19          	mov    rbx,QWORD PTR [rcx+rbx*1]
   18001e330:	eb 07                	jmp    0x18001e339
   18001e332:	48 8b 1d e7 38 0f 00 	mov    rbx,QWORD PTR [rip+0xf38e7]        # 0x180111c20
   18001e339:	48 89 5d d7          	mov    QWORD PTR [rbp-0x29],rbx
   18001e33d:	48 85 db             	test   rbx,rbx
   18001e340:	74 1f                	je     0x18001e361
   18001e342:	ff 15 68 ee 03 00    	call   QWORD PTR [rip+0x3ee68]        # 0x18005d1b0
   18001e348:	8b f8                	mov    edi,eax
   18001e34a:	39 43 08             	cmp    DWORD PTR [rbx+0x8],eax
   18001e34d:	74 0c                	je     0x18001e35b
   18001e34f:	48 8b cb             	mov    rcx,rbx
   18001e352:	ff 15 60 ee 03 00    	call   QWORD PTR [rip+0x3ee60]        # 0x18005d1b8
   18001e358:	89 7b 08             	mov    DWORD PTR [rbx+0x8],edi
   18001e35b:	ff 43 0c             	inc    DWORD PTR [rbx+0xc]
   18001e35e:	49 8b 07             	mov    rax,QWORD PTR [r15]
   18001e361:	48 8b 3d b0 38 0f 00 	mov    rdi,QWORD PTR [rip+0xf38b0]        # 0x180111c18
   18001e368:	48 85 c0             	test   rax,rax
   18001e36b:	48 0f 45 f8          	cmovne rdi,rax
   18001e36f:	4c 89 6c 24 38       	mov    QWORD PTR [rsp+0x38],r13
   18001e374:	45 0f b6 e4          	movzx  r12d,r12b
   18001e378:	41 c1 e4 06          	shl    r12d,0x6
   18001e37c:	41 83 c4 02          	add    r12d,0x2
   18001e380:	44 8b 6d 77          	mov    r13d,DWORD PTR [rbp+0x77]
   18001e384:	41 83 fd 05          	cmp    r13d,0x5
   18001e388:	74 1e                	je     0x18001e3a8
   18001e38a:	8b 45 67             	mov    eax,DWORD PTR [rbp+0x67]
   18001e38d:	3b 44 24 40          	cmp    eax,DWORD PTR [rsp+0x40]
   18001e391:	7c 09                	jl     0x18001e39c
   18001e393:	8b 45 6f             	mov    eax,DWORD PTR [rbp+0x6f]
   18001e396:	3b 44 24 44          	cmp    eax,DWORD PTR [rsp+0x44]
   18001e39a:	7d 0c                	jge    0x18001e3a8
   18001e39c:	41 bf 05 00 00 00    	mov    r15d,0x5
   18001e3a2:	41 83 fd 03          	cmp    r13d,0x3
   18001e3a6:	75 06                	jne    0x18001e3ae
   18001e3a8:	41 bf 06 00 00 00    	mov    r15d,0x6
   18001e3ae:	b8 0b 00 00 00       	mov    eax,0xb
   18001e3b3:	80 bd 97 00 00 00 00 	cmp    BYTE PTR [rbp+0x97],0x0
   18001e3ba:	44 0f 44 f8          	cmove  r15d,eax
   18001e3be:	45 8b c7             	mov    r8d,r15d
   18001e3c1:	48 8b 4c 24 30       	mov    rcx,QWORD PTR [rsp+0x30]
   18001e3c6:	80 bd 8f 00 00 00 00 	cmp    BYTE PTR [rbp+0x8f],0x0
   18001e3cd:	74 65                	je     0x18001e434
   18001e3cf:	48 8d 15 8a 6f 04 00 	lea    rdx,[rip+0x46f8a]        # 0x180065360 ; 'RayReconstruction.Hint.Render.Preset.DLAA'
   18001e3d6:	e8 75 3b 03 00       	call   0x180051f50
   18001e3db:	45 8b c7             	mov    r8d,r15d
   18001e3de:	48 8d 15 ab 6f 04 00 	lea    rdx,[rip+0x46fab]        # 0x180065390 ; 'RayReconstruction.Hint.Render.Preset.UltraQuality'
   18001e3e5:	48 8b 4c 24 30       	mov    rcx,QWORD PTR [rsp+0x30]
   18001e3ea:	e8 61 3b 03 00       	call   0x180051f50
   18001e3ef:	45 8b c7             	mov    r8d,r15d
   18001e3f2:	48 8d 15 cf 6f 04 00 	lea    rdx,[rip+0x46fcf]        # 0x1800653c8 ; 'RayReconstruction.Hint.Render.Preset.Quality'
   18001e3f9:	48 8b 4c 24 30       	mov    rcx,QWORD PTR [rsp+0x30]
   18001e3fe:	e8 4d 3b 03 00       	call   0x180051f50
   18001e403:	45 8b c7             	mov    r8d,r15d
   18001e406:	48 8d 15 eb 6f 04 00 	lea    rdx,[rip+0x46feb]        # 0x1800653f8 ; 'RayReconstruction.Hint.Render.Preset.Balanced'
   18001e40d:	48 8b 4c 24 30       	mov    rcx,QWORD PTR [rsp+0x30]
   18001e412:	e8 39 3b 03 00       	call   0x180051f50
   18001e417:	45 8b c7             	mov    r8d,r15d
   18001e41a:	48 8d 15 07 70 04 00 	lea    rdx,[rip+0x47007]        # 0x180065428 ; 'RayReconstruction.Hint.Render.Preset.Performance'
   18001e421:	48 8b 4c 24 30       	mov    rcx,QWORD PTR [rsp+0x30]
   18001e426:	e8 25 3b 03 00       	call   0x180051f50
   18001e42b:	48 8d 15 2e 70 04 00 	lea    rdx,[rip+0x4702e]        # 0x180065460 ; 'RayReconstruction.Hint.Render.Preset.UltraPerformance'
   18001e432:	eb 63                	jmp    0x18001e497
   18001e434:	48 8d 15 5d 70 04 00 	lea    rdx,[rip+0x4705d]        # 0x180065498 ; 'DLSS.Hint.Render.Preset.DLAA'
   18001e43b:	e8 10 3b 03 00       	call   0x180051f50
   18001e440:	45 8b c7             	mov    r8d,r15d
   18001e443:	48 8d 15 6e 70 04 00 	lea    rdx,[rip+0x4706e]        # 0x1800654b8 ; 'DLSS.Hint.Render.Preset.UltraQuality'
   18001e44a:	48 8b 4c 24 30       	mov    rcx,QWORD PTR [rsp+0x30]
   18001e44f:	e8 fc 3a 03 00       	call   0x180051f50
   18001e454:	45 8b c7             	mov    r8d,r15d
   18001e457:	48 8d 15 82 70 04 00 	lea    rdx,[rip+0x47082]        # 0x1800654e0 ; 'DLSS.Hint.Render.Preset.Quality'
   18001e45e:	48 8b 4c 24 30       	mov    rcx,QWORD PTR [rsp+0x30]
   18001e463:	e8 e8 3a 03 00       	call   0x180051f50
   18001e468:	45 8b c7             	mov    r8d,r15d
   18001e46b:	48 8d 15 8e 70 04 00 	lea    rdx,[rip+0x4708e]        # 0x180065500 ; 'DLSS.Hint.Render.Preset.Balanced'
   18001e472:	48 8b 4c 24 30       	mov    rcx,QWORD PTR [rsp+0x30]
   18001e477:	e8 d4 3a 03 00       	call   0x180051f50
   18001e47c:	45 8b c7             	mov    r8d,r15d
   18001e47f:	48 8d 15 a2 70 04 00 	lea    rdx,[rip+0x470a2]        # 0x180065528 ; 'DLSS.Hint.Render.Preset.Performance'
   18001e486:	48 8b 4c 24 30       	mov    rcx,QWORD PTR [rsp+0x30]
   18001e48b:	e8 c0 3a 03 00       	call   0x180051f50
   18001e490:	48 8d 15 b9 70 04 00 	lea    rdx,[rip+0x470b9]        # 0x180065550 ; 'DLSS.Hint.Render.Preset.UltraPerformance'
   18001e497:	45 8b c7             	mov    r8d,r15d
   18001e49a:	48 8b 4c 24 30       	mov    rcx,QWORD PTR [rsp+0x30]
   18001e49f:	e8 ac 3a 03 00       	call   0x180051f50
   18001e4a4:	44 0f b6 bd 8f 00 00 	movzx  r15d,BYTE PTR [rbp+0x8f]
   18001e4ab:	00 
   18001e4ac:	80 bd 87 00 00 00 00 	cmp    BYTE PTR [rbp+0x87],0x0
   18001e4b3:	75 05                	jne    0x18001e4ba
   18001e4b5:	45 84 ff             	test   r15b,r15b
   18001e4b8:	74 04                	je     0x18001e4be
   18001e4ba:	41 83 cc 01          	or     r12d,0x1
   18001e4be:	45 33 c9             	xor    r9d,r9d
   18001e4c1:	44 89 4d 97          	mov    DWORD PTR [rbp-0x69],r9d
   18001e4c5:	8b 44 24 40          	mov    eax,DWORD PTR [rsp+0x40]
   18001e4c9:	89 45 87             	mov    DWORD PTR [rbp-0x79],eax
   18001e4cc:	8b 4c 24 44          	mov    ecx,DWORD PTR [rsp+0x44]
   18001e4d0:	89 4d 8b             	mov    DWORD PTR [rbp-0x75],ecx
   18001e4d3:	8b 55 67             	mov    edx,DWORD PTR [rbp+0x67]
   18001e4d6:	89 54 24 48          	mov    DWORD PTR [rsp+0x48],edx
   18001e4da:	44 8b 45 6f          	mov    r8d,DWORD PTR [rbp+0x6f]
   18001e4de:	44 89 45 83          	mov    DWORD PTR [rbp-0x7d],r8d
   18001e4e2:	44 89 6d 8f          	mov    DWORD PTR [rbp-0x71],r13d
   18001e4e6:	44 89 65 93          	mov    DWORD PTR [rbp-0x6d],r12d
   18001e4ea:	44 89 4d cb          	mov    DWORD PTR [rbp-0x35],r9d
   18001e4ee:	c7 45 a7 01 00 00 00 	mov    DWORD PTR [rbp-0x59],0x1
   18001e4f5:	c7 45 ab 01 00 00 00 	mov    DWORD PTR [rbp-0x55],0x1
   18001e4fc:	c7 45 af 01 00 00 00 	mov    DWORD PTR [rbp-0x51],0x1
   18001e503:	89 45 bb             	mov    DWORD PTR [rbp-0x45],eax
   18001e506:	89 4d bf             	mov    DWORD PTR [rbp-0x41],ecx
   18001e509:	89 55 b3             	mov    DWORD PTR [rbp-0x4d],edx
   18001e50c:	44 89 45 b7          	mov    DWORD PTR [rbp-0x49],r8d
   18001e510:	44 89 6d c3          	mov    DWORD PTR [rbp-0x3d],r13d
   18001e514:	44 89 65 c7          	mov    DWORD PTR [rbp-0x39],r12d
   18001e518:	48 ff 47 08          	inc    QWORD PTR [rdi+0x8]
   18001e51c:	4c 8d 4c 24 38       	lea    r9,[rsp+0x38]
   18001e521:	48 8b 4f 18          	mov    rcx,QWORD PTR [rdi+0x18]
   18001e525:	45 84 ff             	test   r15b,r15b
   18001e528:	74 1a                	je     0x18001e544
   18001e52a:	48 8d 45 a7          	lea    rax,[rbp-0x59]
   18001e52e:	48 89 44 24 28       	mov    QWORD PTR [rsp+0x28],rax
   18001e533:	48 8b 44 24 30       	mov    rax,QWORD PTR [rsp+0x30]
   18001e538:	48 89 44 24 20       	mov    QWORD PTR [rsp+0x20],rax
   18001e53d:	e8 de ed ff ff       	call   0x18001d320
   18001e542:	eb 19                	jmp    0x18001e55d
   18001e544:	48 8d 44 24 48       	lea    rax,[rsp+0x48]
   18001e549:	48 89 44 24 28       	mov    QWORD PTR [rsp+0x28],rax
   18001e54e:	48 8b 44 24 30       	mov    rax,QWORD PTR [rsp+0x30]
   18001e553:	48 89 44 24 20       	mov    QWORD PTR [rsp+0x20],rax
   18001e558:	e8 93 e7 ff ff       	call   0x18001ccf0
   18001e55d:	25 00 00 f0 ff       	and    eax,0xfff00000
   18001e562:	3d 00 00 d0 ba       	cmp    eax,0xbad00000
   18001e567:	75 60                	jne    0x18001e5c9
   18001e569:	45 33 c0             	xor    r8d,r8d
   18001e56c:	48 8d 15 8d 6f 04 00 	lea    rdx,[rip+0x46f8d]        # 0x180065500 ; 'DLSS.Hint.Render.Preset.Balanced'
   18001e573:	48 8b 4c 24 30       	mov    rcx,QWORD PTR [rsp+0x30]
   18001e578:	e8 d3 39 03 00       	call   0x180051f50
   18001e57d:	c7 45 8f 01 00 00 00 	mov    DWORD PTR [rbp-0x71],0x1
   18001e584:	48 ff 47 08          	inc    QWORD PTR [rdi+0x8]
   18001e588:	4c 8d 4c 24 38       	lea    r9,[rsp+0x38]
   18001e58d:	48 8b 4f 18          	mov    rcx,QWORD PTR [rdi+0x18]
   18001e591:	45 84 ff             	test   r15b,r15b
   18001e594:	74 1a                	je     0x18001e5b0
   18001e596:	48 8d 45 a7          	lea    rax,[rbp-0x59]
   18001e59a:	48 89 44 24 28       	mov    QWORD PTR [rsp+0x28],rax
   18001e59f:	48 8b 44 24 30       	mov    rax,QWORD PTR [rsp+0x30]
   18001e5a4:	48 89 44 24 20       	mov    QWORD PTR [rsp+0x20],rax
   18001e5a9:	e8 72 ed ff ff       	call   0x18001d320
   18001e5ae:	eb 19                	jmp    0x18001e5c9
   18001e5b0:	48 8d 44 24 48       	lea    rax,[rsp+0x48]
   18001e5b5:	48 89 44 24 28       	mov    QWORD PTR [rsp+0x28],rax
   18001e5ba:	48 8b 44 24 30       	mov    rax,QWORD PTR [rsp+0x30]
   18001e5bf:	48 89 44 24 20       	mov    QWORD PTR [rsp+0x20],rax
   18001e5c4:	e8 27 e7 ff ff       	call   0x18001ccf0
   18001e5c9:	48 8b 44 24 38       	mov    rax,QWORD PTR [rsp+0x38]
   18001e5ce:	48 89 44 24 48       	mov    QWORD PTR [rsp+0x48],rax
   18001e5d3:	48 8b 44 24 30       	mov    rax,QWORD PTR [rsp+0x30]
   18001e5d8:	48 89 45 87          	mov    QWORD PTR [rbp-0x79],rax
   18001e5dc:	48 8d 55 9f          	lea    rdx,[rbp-0x61]
   18001e5e0:	49 8b ce             	mov    rcx,r14
   18001e5e3:	e8 b8 1a 00 00       	call   0x1800200a0
   18001e5e8:	0f 10 44 24 48       	movups xmm0,XMMWORD PTR [rsp+0x48]
   18001e5ed:	0f 11 40 08          	movups XMMWORD PTR [rax+0x8],xmm0
   18001e5f1:	e8 ea 39 01 00       	call   0x180031fe0
   18001e5f6:	48 8b 44 24 38       	mov    rax,QWORD PTR [rsp+0x38]
   18001e5fb:	48 89 44 24 48       	mov    QWORD PTR [rsp+0x48],rax
   18001e600:	48 8b 44 24 30       	mov    rax,QWORD PTR [rsp+0x30]
   18001e605:	48 89 45 87          	mov    QWORD PTR [rbp-0x79],rax
   18001e609:	0f 10 44 24 48       	movups xmm0,XMMWORD PTR [rsp+0x48]
   18001e60e:	0f 11 06             	movups XMMWORD PTR [rsi],xmm0
   18001e611:	48 85 db             	test   rbx,rbx
   18001e614:	74 16                	je     0x18001e62c
   18001e616:	83 43 0c ff          	add    DWORD PTR [rbx+0xc],0xffffffff
   18001e61a:	75 10                	jne    0x18001e62c
   18001e61c:	c7 43 08 ff ff ff ff 	mov    DWORD PTR [rbx+0x8],0xffffffff
   18001e623:	48 8b cb             	mov    rcx,rbx
   18001e626:	ff 15 7c eb 03 00    	call   QWORD PTR [rip+0x3eb7c]        # 0x18005d1a8
   18001e62c:	48 8b c6             	mov    rax,rsi
   18001e62f:	48 8b 4d f7          	mov    rcx,QWORD PTR [rbp-0x9]
   18001e633:	48 33 cc             	xor    rcx,rsp
   18001e636:	e8 35 8a 03 00       	call   0x180057070
   18001e63b:	48 8b 9c 24 10 01 00 	mov    rbx,QWORD PTR [rsp+0x110]
   18001e642:	00 
   18001e643:	48 81 c4 d0 00 00 00 	add    rsp,0xd0
   18001e64a:	41 5f                	pop    r15
   18001e64c:	41 5e                	pop    r14
   18001e64e:	41 5d                	pop    r13
   18001e650:	41 5c                	pop    r12
   18001e652:	5f                   	pop    rdi
   18001e653:	5e                   	pop    rsi
   18001e654:	5d                   	pop    rbp
   18001e655:	c3                   	ret
   18001e656:	cc                   	int3
   18001e657:	cc                   	int3
   18001e658:	cc                   	int3
   18001e659:	cc                   	int3
   18001e65a:	cc                   	int3
   18001e65b:	cc                   	int3
   18001e65c:	cc                   	int3
   18001e65d:	cc                   	int3
   18001e65e:	cc                   	int3
   18001e65f:	cc                   	int3
   18001e660:	40 57                	rex push rdi
   18001e662:	48 83 ec 30          	sub    rsp,0x30
   18001e666:	48 c7 44 24 20 fe ff 	mov    QWORD PTR [rsp+0x20],0xfffffffffffffffe
   18001e66d:	ff ff 
   18001e66f:	48 89 5c 24 48       	mov    QWORD PTR [rsp+0x48],rbx
   18001e674:	48 89 6c 24 50       	mov    QWORD PTR [rsp+0x50],rbp
   18001e679:	48 89 74 24 58       	mov    QWORD PTR [rsp+0x58],rsi
   18001e67e:	8b ea                	mov    ebp,edx
   18001e680:	48 8b f9             	mov    rdi,rcx
   18001e683:	48 8d 05 26 6f 04 00 	lea    rax,[rip+0x46f26]        # 0x1800655b0
   18001e68a:	48 89 01             	mov    QWORD PTR [rcx],rax
   18001e68d:	48 8b 99 a0 00 00 00 	mov    rbx,QWORD PTR [rcx+0xa0]
   18001e694:	48 8b 81 80 00 00 00 	mov    rax,QWORD PTR [rcx+0x80]
   18001e69b:	4c 8d 04 40          	lea    r8,[rax+rax*2]
   18001e69f:	4a 8d 34 c3          	lea    rsi,[rbx+r8*8]
   18001e6a3:	48 3b de             	cmp    rbx,rsi
   18001e6a6:	0f 84 97 00 00 00    	je     0x18001e743
   18001e6ac:	48 8b 89 88 00 00 00 	mov    rcx,QWORD PTR [rcx+0x88]
   18001e6b3:	0f 1f 40 00          	nop    DWORD PTR [rax+0x0]
   18001e6b7:	66 0f 1f 84 00 00 00 	nop    WORD PTR [rax+rax*1+0x0]
   18001e6be:	00 00 
   18001e6c0:	48 8b 03             	mov    rax,QWORD PTR [rbx]
   18001e6c3:	48 3b c8             	cmp    rcx,rax
   18001e6c6:	74 0d                	je     0x18001e6d5
   18001e6c8:	48 83 7f 70 00       	cmp    QWORD PTR [rdi+0x70],0x0
   18001e6cd:	76 0f                	jbe    0x18001e6de
   18001e6cf:	48 39 47 68          	cmp    QWORD PTR [rdi+0x68],rax
   18001e6d3:	75 09                	jne    0x18001e6de
   18001e6d5:	48 83 c3 18          	add    rbx,0x18
   18001e6d9:	48 3b de             	cmp    rbx,rsi
   18001e6dc:	75 e2                	jne    0x18001e6c0
   18001e6de:	48 3b de             	cmp    rbx,rsi
   18001e6e1:	74 60                	je     0x18001e743
   18001e6e3:	48 8b 4b 08          	mov    rcx,QWORD PTR [rbx+0x8]
   18001e6e7:	48 85 c9             	test   rcx,rcx
   18001e6ea:	74 06                	je     0x18001e6f2
   18001e6ec:	e8 df 46 03 00       	call   0x180052dd0
   18001e6f1:	90                   	nop
   18001e6f2:	48 8b 4b 10          	mov    rcx,QWORD PTR [rbx+0x10]
   18001e6f6:	48 85 c9             	test   rcx,rcx
   18001e6f9:	74 06                	je     0x18001e701
   18001e6fb:	e8 50 42 03 00       	call   0x180052950
   18001e700:	90                   	nop
   18001e701:	48 83 c3 18          	add    rbx,0x18
   18001e705:	48 3b de             	cmp    rbx,rsi
   18001e708:	74 39                	je     0x18001e743
   18001e70a:	48 8b 8f 88 00 00 00 	mov    rcx,QWORD PTR [rdi+0x88]
   18001e711:	0f 1f 40 00          	nop    DWORD PTR [rax+0x0]
   18001e715:	66 66 66 0f 1f 84 00 	data16 data16 nop WORD PTR [rax+rax*1+0x0]
   18001e71c:	00 00 00 00 
   18001e720:	48 8b 03             	mov    rax,QWORD PTR [rbx]
   18001e723:	48 3b c8             	cmp    rcx,rax
   18001e726:	74 0d                	je     0x18001e735
   18001e728:	48 83 7f 70 00       	cmp    QWORD PTR [rdi+0x70],0x0
   18001e72d:	76 0f                	jbe    0x18001e73e
   18001e72f:	48 39 47 68          	cmp    QWORD PTR [rdi+0x68],rax
   18001e733:	75 09                	jne    0x18001e73e
   18001e735:	48 83 c3 18          	add    rbx,0x18
   18001e739:	48 3b de             	cmp    rbx,rsi
   18001e73c:	75 e2                	jne    0x18001e720
   18001e73e:	48 3b de             	cmp    rbx,rsi
   18001e741:	75 a0                	jne    0x18001e6e3
   18001e743:	48 8b 4f 38          	mov    rcx,QWORD PTR [rdi+0x38]
   18001e747:	48 85 c9             	test   rcx,rcx
   18001e74a:	74 06                	je     0x18001e752
   18001e74c:	e8 ff 41 03 00       	call   0x180052950
   18001e751:	90                   	nop
   18001e752:	4c 8b 8f a8 00 00 00 	mov    r9,QWORD PTR [rdi+0xa8]
   18001e759:	4d 8b 01             	mov    r8,QWORD PTR [r9]
   18001e75c:	48 8d 54 24 40       	lea    rdx,[rsp+0x40]
   18001e761:	48 8d 8f a8 00 00 00 	lea    rcx,[rdi+0xa8]
   18001e768:	e8 93 be fe ff       	call   0x18000a600
   18001e76d:	90                   	nop
   18001e76e:	48 8b 8f a8 00 00 00 	mov    rcx,QWORD PTR [rdi+0xa8]
   18001e775:	ff 15 05 f1 03 00    	call   QWORD PTR [rip+0x3f105]        # 0x18005d880
   18001e77b:	90                   	nop
   18001e77c:	48 8b 8f a0 00 00 00 	mov    rcx,QWORD PTR [rdi+0xa0]
   18001e783:	48 85 c9             	test   rcx,rcx
   18001e786:	74 07                	je     0x18001e78f
   18001e788:	ff 15 f2 f0 03 00    	call   QWORD PTR [rip+0x3f0f2]        # 0x18005d880
   18001e78e:	90                   	nop
   18001e78f:	40 f6 c5 01          	test   bpl,0x1
   18001e793:	74 0e                	je     0x18001e7a3
   18001e795:	ba b8 00 00 00       	mov    edx,0xb8
   18001e79a:	48 8b cf             	mov    rcx,rdi
   18001e79d:	e8 f2 88 03 00       	call   0x180057094
   18001e7a2:	90                   	nop
   18001e7a3:	48 8b c7             	mov    rax,rdi
   18001e7a6:	48 8b 5c 24 48       	mov    rbx,QWORD PTR [rsp+0x48]
   18001e7ab:	48 8b 6c 24 50       	mov    rbp,QWORD PTR [rsp+0x50]
   18001e7b0:	48 8b 74 24 58       	mov    rsi,QWORD PTR [rsp+0x58]
   18001e7b5:	48 83 c4 30          	add    rsp,0x30
   18001e7b9:	5f                   	pop    rdi
   18001e7ba:	c3                   	ret
   18001e7bb:	cc                   	int3
   18001e7bc:	cc                   	int3
   18001e7bd:	cc                   	int3
   18001e7be:	cc                   	int3
   18001e7bf:	cc                   	int3
   18001e7c0:	48 83 ec 38          	sub    rsp,0x38
   18001e7c4:	48 c7 44 24 20 fe ff 	mov    QWORD PTR [rsp+0x20],0xfffffffffffffffe
   18001e7cb:	ff ff 
   18001e7cd:	48 8b 49 60          	mov    rcx,QWORD PTR [rcx+0x60]
   18001e7d1:	48 85 c9             	test   rcx,rcx
   18001e7d4:	74 07                	je     0x18001e7dd
   18001e7d6:	ff 15 a4 f0 03 00    	call   QWORD PTR [rip+0x3f0a4]        # 0x18005d880
   18001e7dc:	90                   	nop
   18001e7dd:	48 83 c4 38          	add    rsp,0x38
   18001e7e1:	c3                   	ret
   18001e7e2:	cc                   	int3
   18001e7e3:	cc                   	int3
   18001e7e4:	cc                   	int3
   18001e7e5:	cc                   	int3
   18001e7e6:	cc                   	int3
   18001e7e7:	cc                   	int3
   18001e7e8:	cc                   	int3
   18001e7e9:	cc                   	int3
   18001e7ea:	cc                   	int3
   18001e7eb:	cc                   	int3
   18001e7ec:	cc                   	int3
   18001e7ed:	cc                   	int3
   18001e7ee:	cc                   	int3
   18001e7ef:	cc                   	int3
