
/mnt/data/d3d_rmdwin10_f(5).dll:     file format pei-x86-64


Disassembly of section .text:

000000018003d190 <.text+0x3c190>:
   18003d190:	40 55                	rex push rbp
   18003d192:	53                   	push   rbx
   18003d193:	56                   	push   rsi
   18003d194:	57                   	push   rdi
   18003d195:	41 54                	push   r12
   18003d197:	41 55                	push   r13
   18003d199:	41 56                	push   r14
   18003d19b:	41 57                	push   r15
   18003d19d:	48 8d 6c 24 98       	lea    rbp,[rsp-0x68]
   18003d1a2:	48 81 ec 68 01 00 00 	sub    rsp,0x168
   18003d1a9:	48 c7 45 f0 fe ff ff 	mov    QWORD PTR [rbp-0x10],0xfffffffffffffffe
   18003d1b0:	ff 
   18003d1b1:	48 8b 05 38 9a 0b 00 	mov    rax,QWORD PTR [rip+0xb9a38]        # 0x1800f6bf0
   18003d1b8:	48 33 c4             	xor    rax,rsp
   18003d1bb:	48 89 45 50          	mov    QWORD PTR [rbp+0x50],rax
   18003d1bf:	4c 89 4c 24 40       	mov    QWORD PTR [rsp+0x40],r9
   18003d1c4:	45 8b e8             	mov    r13d,r8d
   18003d1c7:	8b fa                	mov    edi,edx
   18003d1c9:	48 8b d9             	mov    rbx,rcx
   18003d1cc:	48 8b b5 d0 00 00 00 	mov    rsi,QWORD PTR [rbp+0xd0]
   18003d1d3:	4c 8b bd e8 00 00 00 	mov    r15,QWORD PTR [rbp+0xe8]
   18003d1da:	4c 8b b5 f0 00 00 00 	mov    r14,QWORD PTR [rbp+0xf0]
   18003d1e1:	33 c0                	xor    eax,eax
   18003d1e3:	89 45 84             	mov    DWORD PTR [rbp-0x7c],eax
   18003d1e6:	0f 57 c0             	xorps  xmm0,xmm0
   18003d1e9:	f3 0f 7f 45 8c       	movdqu XMMWORD PTR [rbp-0x74],xmm0
   18003d1ee:	48 89 45 9c          	mov    QWORD PTR [rbp-0x64],rax
   18003d1f2:	89 45 a4             	mov    DWORD PTR [rbp-0x5c],eax
   18003d1f5:	89 44 24 54          	mov    DWORD PTR [rsp+0x54],eax
   18003d1f9:	f3 0f 7f 44 24 5c    	movdqu XMMWORD PTR [rsp+0x5c],xmm0
   18003d1ff:	48 89 44 24 6c       	mov    QWORD PTR [rsp+0x6c],rax
   18003d204:	89 44 24 74          	mov    DWORD PTR [rsp+0x74],eax
   18003d208:	48 8b 81 88 00 00 00 	mov    rax,QWORD PTR [rcx+0x88]
   18003d20f:	48 89 44 24 78       	mov    QWORD PTR [rsp+0x78],rax
   18003d214:	48 8b 8e 88 00 00 00 	mov    rcx,QWORD PTR [rsi+0x88]
   18003d21b:	48 89 4c 24 48       	mov    QWORD PTR [rsp+0x48],rcx
   18003d220:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   18003d223:	48 8d 55 00          	lea    rdx,[rbp+0x0]
   18003d227:	ff 50 50             	call   QWORD PTR [rax+0x50]
   18003d22a:	44 8b a5 e0 00 00 00 	mov    r12d,DWORD PTR [rbp+0xe0]
   18003d231:	4c 8d 05 e8 51 02 00 	lea    r8,[rip+0x251e8]        # 0x180062420
   18003d238:	83 38 01             	cmp    DWORD PTR [rax],0x1
   18003d23b:	75 4c                	jne    0x18003d289
   18003d23d:	33 d2                	xor    edx,edx
   18003d23f:	89 55 1c             	mov    DWORD PTR [rbp+0x1c],edx
   18003d242:	8b 46 08             	mov    eax,DWORD PTR [rsi+0x8]
   18003d245:	89 45 14             	mov    DWORD PTR [rbp+0x14],eax
   18003d248:	8b 06                	mov    eax,DWORD PTR [rsi]
   18003d24a:	89 45 0c             	mov    DWORD PTR [rbp+0xc],eax
   18003d24d:	8b 46 04             	mov    eax,DWORD PTR [rsi+0x4]
   18003d250:	89 45 10             	mov    DWORD PTR [rbp+0x10],eax
   18003d253:	48 0f be 46 24       	movsx  rax,BYTE PTR [rsi+0x24]
   18003d258:	48 8d 0c 80          	lea    rcx,[rax+rax*4]
   18003d25c:	41 8b 04 88          	mov    eax,DWORD PTR [r8+rcx*4]
   18003d260:	89 45 08             	mov    DWORD PTR [rbp+0x8],eax
   18003d263:	8b 46 30             	mov    eax,DWORD PTR [rsi+0x30]
   18003d266:	89 45 18             	mov    DWORD PTR [rbp+0x18],eax
   18003d269:	48 89 55 00          	mov    QWORD PTR [rbp+0x0],rdx
   18003d26d:	0f 10 45 00          	movups xmm0,XMMWORD PTR [rbp+0x0]
   18003d271:	0f 11 44 24 58       	movups XMMWORD PTR [rsp+0x58],xmm0
   18003d276:	0f 10 4d 10          	movups xmm1,XMMWORD PTR [rbp+0x10]
   18003d27a:	0f 11 4c 24 68       	movups XMMWORD PTR [rsp+0x68],xmm1
   18003d27f:	c7 44 24 50 01 00 00 	mov    DWORD PTR [rsp+0x50],0x1
   18003d286:	00 
   18003d287:	eb 19                	jmp    0x18003d2a2
   18003d289:	c7 44 24 50 00 00 00 	mov    DWORD PTR [rsp+0x50],0x0
   18003d290:	00 
   18003d291:	8b 46 10             	mov    eax,DWORD PTR [rsi+0x10]
   18003d294:	0f af 85 d8 00 00 00 	imul   eax,DWORD PTR [rbp+0xd8]
   18003d29b:	41 03 c4             	add    eax,r12d
   18003d29e:	89 44 24 58          	mov    DWORD PTR [rsp+0x58],eax
   18003d2a2:	48 8b 8b 88 00 00 00 	mov    rcx,QWORD PTR [rbx+0x88]
   18003d2a9:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   18003d2ac:	48 8d 55 00          	lea    rdx,[rbp+0x0]
   18003d2b0:	ff 50 50             	call   QWORD PTR [rax+0x50]
   18003d2b3:	83 38 01             	cmp    DWORD PTR [rax],0x1
   18003d2b6:	75 4f                	jne    0x18003d307
   18003d2b8:	33 d2                	xor    edx,edx
   18003d2ba:	89 55 1c             	mov    DWORD PTR [rbp+0x1c],edx
   18003d2bd:	8b 43 08             	mov    eax,DWORD PTR [rbx+0x8]
   18003d2c0:	89 45 14             	mov    DWORD PTR [rbp+0x14],eax
   18003d2c3:	8b 03                	mov    eax,DWORD PTR [rbx]
   18003d2c5:	89 45 0c             	mov    DWORD PTR [rbp+0xc],eax
   18003d2c8:	8b 43 04             	mov    eax,DWORD PTR [rbx+0x4]
   18003d2cb:	89 45 10             	mov    DWORD PTR [rbp+0x10],eax
   18003d2ce:	48 0f be 43 24       	movsx  rax,BYTE PTR [rbx+0x24]
   18003d2d3:	48 8d 0c 80          	lea    rcx,[rax+rax*4]
   18003d2d7:	48 8d 05 42 51 02 00 	lea    rax,[rip+0x25142]        # 0x180062420
   18003d2de:	8b 04 88             	mov    eax,DWORD PTR [rax+rcx*4]
   18003d2e1:	89 45 08             	mov    DWORD PTR [rbp+0x8],eax
   18003d2e4:	8b 43 30             	mov    eax,DWORD PTR [rbx+0x30]
   18003d2e7:	89 45 18             	mov    DWORD PTR [rbp+0x18],eax
   18003d2ea:	48 89 55 00          	mov    QWORD PTR [rbp+0x0],rdx
   18003d2ee:	0f 10 45 00          	movups xmm0,XMMWORD PTR [rbp+0x0]
   18003d2f2:	0f 11 45 88          	movups XMMWORD PTR [rbp-0x78],xmm0
   18003d2f6:	0f 10 4d 10          	movups xmm1,XMMWORD PTR [rbp+0x10]
   18003d2fa:	0f 11 4d 98          	movups XMMWORD PTR [rbp-0x68],xmm1
   18003d2fe:	c7 45 80 01 00 00 00 	mov    DWORD PTR [rbp-0x80],0x1
   18003d305:	eb 11                	jmp    0x18003d318
   18003d307:	c7 45 80 00 00 00 00 	mov    DWORD PTR [rbp-0x80],0x0
   18003d30e:	0f af 7b 10          	imul   edi,DWORD PTR [rbx+0x10]
   18003d312:	41 03 fd             	add    edi,r13d
   18003d315:	89 7d 88             	mov    DWORD PTR [rbp-0x78],edi
   18003d318:	45 8b 07             	mov    r8d,DWORD PTR [r15]
   18003d31b:	44 89 45 38          	mov    DWORD PTR [rbp+0x38],r8d
   18003d31f:	45 8b 4f 04          	mov    r9d,DWORD PTR [r15+0x4]
   18003d323:	44 89 4d 3c          	mov    DWORD PTR [rbp+0x3c],r9d
   18003d327:	45 8b 57 08          	mov    r10d,DWORD PTR [r15+0x8]
   18003d32b:	44 89 55 40          	mov    DWORD PTR [rbp+0x40],r10d
   18003d32f:	4d 85 f6             	test   r14,r14
   18003d332:	74 1f                	je     0x18003d353
   18003d334:	41 8b 0e             	mov    ecx,DWORD PTR [r14]
   18003d337:	41 03 c8             	add    ecx,r8d
   18003d33a:	89 4d 44             	mov    DWORD PTR [rbp+0x44],ecx
   18003d33d:	41 8b 4e 04          	mov    ecx,DWORD PTR [r14+0x4]
   18003d341:	41 03 c9             	add    ecx,r9d
   18003d344:	89 4d 48             	mov    DWORD PTR [rbp+0x48],ecx
   18003d347:	41 8b 4e 08          	mov    ecx,DWORD PTR [r14+0x8]
   18003d34b:	41 03 ca             	add    ecx,r10d
   18003d34e:	89 4d 4c             	mov    DWORD PTR [rbp+0x4c],ecx
   18003d351:	eb 41                	jmp    0x18003d394
   18003d353:	8b 16                	mov    edx,DWORD PTR [rsi]
   18003d355:	41 8b cc             	mov    ecx,r12d
   18003d358:	d3 ea                	shr    edx,cl
   18003d35a:	b8 01 00 00 00       	mov    eax,0x1
   18003d35f:	3b d0                	cmp    edx,eax
   18003d361:	0f 4f c2             	cmovg  eax,edx
   18003d364:	41 03 c0             	add    eax,r8d
   18003d367:	89 45 44             	mov    DWORD PTR [rbp+0x44],eax
   18003d36a:	8b 56 04             	mov    edx,DWORD PTR [rsi+0x4]
   18003d36d:	d3 ea                	shr    edx,cl
   18003d36f:	b8 01 00 00 00       	mov    eax,0x1
   18003d374:	3b d0                	cmp    edx,eax
   18003d376:	0f 4f c2             	cmovg  eax,edx
   18003d379:	41 03 c1             	add    eax,r9d
   18003d37c:	89 45 48             	mov    DWORD PTR [rbp+0x48],eax
   18003d37f:	8b 56 08             	mov    edx,DWORD PTR [rsi+0x8]
   18003d382:	d3 ea                	shr    edx,cl
   18003d384:	b8 01 00 00 00       	mov    eax,0x1
   18003d389:	3b d0                	cmp    edx,eax
   18003d38b:	0f 4f c2             	cmovg  eax,edx
   18003d38e:	41 03 c2             	add    eax,r10d
   18003d391:	89 45 4c             	mov    DWORD PTR [rbp+0x4c],eax
   18003d394:	48 8b 46 50          	mov    rax,QWORD PTR [rsi+0x50]
   18003d398:	48 85 c0             	test   rax,rax
   18003d39b:	75 04                	jne    0x18003d3a1
   18003d39d:	48 8d 46 40          	lea    rax,[rsi+0x40]
   18003d3a1:	44 8b 70 20          	mov    r14d,DWORD PTR [rax+0x20]
   18003d3a5:	48 8b 43 50          	mov    rax,QWORD PTR [rbx+0x50]
   18003d3a9:	48 85 c0             	test   rax,rax
   18003d3ac:	75 04                	jne    0x18003d3b2
   18003d3ae:	48 8d 43 40          	lea    rax,[rbx+0x40]
   18003d3b2:	44 8b 78 20          	mov    r15d,DWORD PTR [rax+0x20]
   18003d3b6:	45 33 e4             	xor    r12d,r12d
   18003d3b9:	41 8b cc             	mov    ecx,r12d
   18003d3bc:	f6 46 38 30          	test   BYTE PTR [rsi+0x38],0x30
   18003d3c0:	75 45                	jne    0x18003d407
   18003d3c2:	41 81 fe 00 08 00 00 	cmp    r14d,0x800
   18003d3c9:	74 3c                	je     0x18003d407
   18003d3cb:	38 4e 68             	cmp    BYTE PTR [rsi+0x68],cl
   18003d3ce:	75 37                	jne    0x18003d407
   18003d3d0:	48 8b 86 88 00 00 00 	mov    rax,QWORD PTR [rsi+0x88]
   18003d3d7:	48 c7 45 18 00 08 00 	mov    QWORD PTR [rbp+0x18],0x800
   18003d3de:	00 
   18003d3df:	4c 89 65 00          	mov    QWORD PTR [rbp+0x0],r12
   18003d3e3:	48 89 45 08          	mov    QWORD PTR [rbp+0x8],rax
   18003d3e7:	44 89 75 14          	mov    DWORD PTR [rbp+0x14],r14d
   18003d3eb:	c7 45 10 ff ff ff ff 	mov    DWORD PTR [rbp+0x10],0xffffffff
   18003d3f2:	0f 10 45 00          	movups xmm0,XMMWORD PTR [rbp+0x0]
   18003d3f6:	0f 29 45 b0          	movaps XMMWORD PTR [rbp-0x50],xmm0
   18003d3fa:	0f 10 4d 10          	movups xmm1,XMMWORD PTR [rbp+0x10]
   18003d3fe:	0f 29 4d c0          	movaps XMMWORD PTR [rbp-0x40],xmm1
   18003d402:	41 8d 4c 24 01       	lea    ecx,[r12+0x1]
   18003d407:	f6 43 38 30          	test   BYTE PTR [rbx+0x38],0x30
   18003d40b:	75 4d                	jne    0x18003d45a
   18003d40d:	41 81 ff 00 04 00 00 	cmp    r15d,0x400
   18003d414:	74 44                	je     0x18003d45a
   18003d416:	44 38 63 68          	cmp    BYTE PTR [rbx+0x68],r12b
   18003d41a:	75 3e                	jne    0x18003d45a
   18003d41c:	48 8b 83 88 00 00 00 	mov    rax,QWORD PTR [rbx+0x88]
   18003d423:	48 c7 45 18 00 04 00 	mov    QWORD PTR [rbp+0x18],0x400
   18003d42a:	00 
   18003d42b:	4c 89 65 00          	mov    QWORD PTR [rbp+0x0],r12
   18003d42f:	48 89 45 08          	mov    QWORD PTR [rbp+0x8],rax
   18003d433:	44 89 7d 14          	mov    DWORD PTR [rbp+0x14],r15d
   18003d437:	c7 45 10 ff ff ff ff 	mov    DWORD PTR [rbp+0x10],0xffffffff
   18003d43e:	48 8b c1             	mov    rax,rcx
   18003d441:	48 c1 e0 05          	shl    rax,0x5
   18003d445:	0f 10 45 00          	movups xmm0,XMMWORD PTR [rbp+0x0]
   18003d449:	0f 11 44 05 b0       	movups XMMWORD PTR [rbp+rax*1-0x50],xmm0
   18003d44e:	0f 10 4d 10          	movups xmm1,XMMWORD PTR [rbp+0x10]
   18003d452:	0f 11 4c 05 c0       	movups XMMWORD PTR [rbp+rax*1-0x40],xmm1
   18003d457:	48 ff c1             	inc    rcx
   18003d45a:	48 85 c9             	test   rcx,rcx
   18003d45d:	74 09                	je     0x18003d468
   18003d45f:	48 8d 55 b0          	lea    rdx,[rbp-0x50]
   18003d463:	e8 98 db ff ff       	call   0x18003b000
   18003d468:	8b 0d 8e 41 0d 00    	mov    ecx,DWORD PTR [rip+0xd418e]        # 0x1801115fc
   18003d46e:	65 48 8b 04 25 58 00 	mov    rax,QWORD PTR gs:0x58
   18003d475:	00 00 
   18003d477:	48 8b 3c c8          	mov    rdi,QWORD PTR [rax+rcx*8]
   18003d47b:	41 bd 08 00 00 00    	mov    r13d,0x8
   18003d481:	4c 03 ef             	add    r13,rdi
   18003d484:	49 8b 4d 00          	mov    rcx,QWORD PTR [r13+0x0]
   18003d488:	48 85 c9             	test   rcx,rcx
   18003d48b:	74 0b                	je     0x18003d498
   18003d48d:	b8 10 00 00 00       	mov    eax,0x10
   18003d492:	48 8b 3c 38          	mov    rdi,QWORD PTR [rax+rdi*1]
   18003d496:	eb 07                	jmp    0x18003d49f
   18003d498:	48 8b 3d 81 47 0d 00 	mov    rdi,QWORD PTR [rip+0xd4781]        # 0x180111c20
   18003d49f:	48 89 7d f8          	mov    QWORD PTR [rbp-0x8],rdi
   18003d4a3:	48 85 ff             	test   rdi,rdi
   18003d4a6:	74 25                	je     0x18003d4cd
   18003d4a8:	ff 15 02 fd 01 00    	call   QWORD PTR [rip+0x1fd02]        # 0x18005d1b0
   18003d4ae:	44 8b e0             	mov    r12d,eax
   18003d4b1:	39 47 08             	cmp    DWORD PTR [rdi+0x8],eax
   18003d4b4:	74 0d                	je     0x18003d4c3
   18003d4b6:	48 8b cf             	mov    rcx,rdi
   18003d4b9:	ff 15 f9 fc 01 00    	call   QWORD PTR [rip+0x1fcf9]        # 0x18005d1b8
   18003d4bf:	44 89 67 08          	mov    DWORD PTR [rdi+0x8],r12d
   18003d4c3:	ff 47 0c             	inc    DWORD PTR [rdi+0xc]
   18003d4c6:	49 8b 4d 00          	mov    rcx,QWORD PTR [r13+0x0]
   18003d4ca:	45 33 e4             	xor    r12d,r12d
   18003d4cd:	48 8b 05 44 47 0d 00 	mov    rax,QWORD PTR [rip+0xd4744]        # 0x180111c18
   18003d4d4:	48 85 c9             	test   rcx,rcx
   18003d4d7:	48 0f 45 c1          	cmovne rax,rcx
   18003d4db:	48 ff 40 08          	inc    QWORD PTR [rax+0x8]
   18003d4df:	48 8b 48 18          	mov    rcx,QWORD PTR [rax+0x18]
   18003d4e3:	4c 8b 11             	mov    r10,QWORD PTR [rcx]
   18003d4e6:	48 8d 45 38          	lea    rax,[rbp+0x38]
   18003d4ea:	48 89 44 24 30       	mov    QWORD PTR [rsp+0x30],rax
   18003d4ef:	48 8d 44 24 48       	lea    rax,[rsp+0x48]
   18003d4f4:	48 89 44 24 28       	mov    QWORD PTR [rsp+0x28],rax
   18003d4f9:	48 8b 54 24 40       	mov    rdx,QWORD PTR [rsp+0x40]
   18003d4fe:	8b 42 08             	mov    eax,DWORD PTR [rdx+0x8]
   18003d501:	89 44 24 20          	mov    DWORD PTR [rsp+0x20],eax
   18003d505:	44 8b 4a 04          	mov    r9d,DWORD PTR [rdx+0x4]
   18003d509:	44 8b 02             	mov    r8d,DWORD PTR [rdx]
   18003d50c:	48 8d 54 24 78       	lea    rdx,[rsp+0x78]
   18003d511:	41 ff 92 80 00 00 00 	call   QWORD PTR [r10+0x80]
   18003d518:	90                   	nop
   18003d519:	48 85 ff             	test   rdi,rdi
   18003d51c:	74 16                	je     0x18003d534
   18003d51e:	83 47 0c ff          	add    DWORD PTR [rdi+0xc],0xffffffff
   18003d522:	75 10                	jne    0x18003d534
   18003d524:	c7 47 08 ff ff ff ff 	mov    DWORD PTR [rdi+0x8],0xffffffff
   18003d52b:	48 8b cf             	mov    rcx,rdi
   18003d52e:	ff 15 74 fc 01 00    	call   QWORD PTR [rip+0x1fc74]        # 0x18005d1a8
   18003d534:	49 8b cc             	mov    rcx,r12
   18003d537:	f6 46 38 30          	test   BYTE PTR [rsi+0x38],0x30
   18003d53b:	75 4d                	jne    0x18003d58a
   18003d53d:	41 81 fe 00 08 00 00 	cmp    r14d,0x800
   18003d544:	74 44                	je     0x18003d58a
   18003d546:	80 7e 68 00          	cmp    BYTE PTR [rsi+0x68],0x0
   18003d54a:	75 3e                	jne    0x18003d58a
   18003d54c:	44 89 65 1c          	mov    DWORD PTR [rbp+0x1c],r12d
   18003d550:	48 c7 45 00 00 00 00 	mov    QWORD PTR [rbp+0x0],0x0
   18003d557:	00 
   18003d558:	48 8b 86 88 00 00 00 	mov    rax,QWORD PTR [rsi+0x88]
   18003d55f:	48 89 45 08          	mov    QWORD PTR [rbp+0x8],rax
   18003d563:	c7 45 14 00 08 00 00 	mov    DWORD PTR [rbp+0x14],0x800
   18003d56a:	44 89 75 18          	mov    DWORD PTR [rbp+0x18],r14d
   18003d56e:	c7 45 10 ff ff ff ff 	mov    DWORD PTR [rbp+0x10],0xffffffff
   18003d575:	0f 10 45 00          	movups xmm0,XMMWORD PTR [rbp+0x0]
   18003d579:	0f 29 45 b0          	movaps XMMWORD PTR [rbp-0x50],xmm0
   18003d57d:	0f 10 4d 10          	movups xmm1,XMMWORD PTR [rbp+0x10]
   18003d581:	0f 29 4d c0          	movaps XMMWORD PTR [rbp-0x40],xmm1
   18003d585:	b9 01 00 00 00       	mov    ecx,0x1
   18003d58a:	f6 43 38 30          	test   BYTE PTR [rbx+0x38],0x30
   18003d58e:	75 54                	jne    0x18003d5e4
   18003d590:	41 81 ff 00 04 00 00 	cmp    r15d,0x400
   18003d597:	74 4b                	je     0x18003d5e4
   18003d599:	80 7b 68 00          	cmp    BYTE PTR [rbx+0x68],0x0
   18003d59d:	75 45                	jne    0x18003d5e4
   18003d59f:	44 89 65 1c          	mov    DWORD PTR [rbp+0x1c],r12d
   18003d5a3:	48 c7 45 00 00 00 00 	mov    QWORD PTR [rbp+0x0],0x0
   18003d5aa:	00 
   18003d5ab:	48 8b 83 88 00 00 00 	mov    rax,QWORD PTR [rbx+0x88]
   18003d5b2:	48 89 45 08          	mov    QWORD PTR [rbp+0x8],rax
   18003d5b6:	c7 45 14 00 04 00 00 	mov    DWORD PTR [rbp+0x14],0x400
   18003d5bd:	44 89 7d 18          	mov    DWORD PTR [rbp+0x18],r15d
   18003d5c1:	c7 45 10 ff ff ff ff 	mov    DWORD PTR [rbp+0x10],0xffffffff
   18003d5c8:	48 8b c1             	mov    rax,rcx
   18003d5cb:	48 c1 e0 05          	shl    rax,0x5
   18003d5cf:	0f 10 45 00          	movups xmm0,XMMWORD PTR [rbp+0x0]
   18003d5d3:	0f 11 44 05 b0       	movups XMMWORD PTR [rbp+rax*1-0x50],xmm0
   18003d5d8:	0f 10 4d 10          	movups xmm1,XMMWORD PTR [rbp+0x10]
   18003d5dc:	0f 11 4c 05 c0       	movups XMMWORD PTR [rbp+rax*1-0x40],xmm1
   18003d5e1:	48 ff c1             	inc    rcx
   18003d5e4:	48 85 c9             	test   rcx,rcx
   18003d5e7:	74 09                	je     0x18003d5f2
   18003d5e9:	48 8d 55 b0          	lea    rdx,[rbp-0x50]
   18003d5ed:	e8 0e da ff ff       	call   0x18003b000
   18003d5f2:	48 8b 4d 50          	mov    rcx,QWORD PTR [rbp+0x50]
   18003d5f6:	48 33 cc             	xor    rcx,rsp
   18003d5f9:	e8 72 9a 01 00       	call   0x180057070
   18003d5fe:	48 81 c4 68 01 00 00 	add    rsp,0x168
   18003d605:	41 5f                	pop    r15
   18003d607:	41 5e                	pop    r14
   18003d609:	41 5d                	pop    r13
   18003d60b:	41 5c                	pop    r12
   18003d60d:	5f                   	pop    rdi
   18003d60e:	5e                   	pop    rsi
   18003d60f:	5b                   	pop    rbx
   18003d610:	5d                   	pop    rbp
   18003d611:	c3                   	ret
