
upload/renderer_rmdwin10_f(4).dll:     file format pei-x86-64


Disassembly of section .text:

00000001800181c0 <.text+0x171c0>:
   1800181c0:	48 8b c4             	mov    rax,rsp
   1800181c3:	48 81 ec b8 00 00 00 	sub    rsp,0xb8
   1800181ca:	f2 0f 10 61 08       	movsd  xmm4,QWORD PTR [rcx+0x8]
   1800181cf:	f2 0f 10 11          	movsd  xmm2,QWORD PTR [rcx]
   1800181d3:	0f 28 c4             	movaps xmm0,xmm4
   1800181d6:	f2 0f 59 42 20       	mulsd  xmm0,QWORD PTR [rdx+0x20]
   1800181db:	f2 0f 10 59 10       	movsd  xmm3,QWORD PTR [rcx+0x10]
   1800181e0:	f2 0f 10 69 18       	movsd  xmm5,QWORD PTR [rcx+0x18]
   1800181e5:	0f 28 cb             	movaps xmm1,xmm3
   1800181e8:	f2 0f 59 4a 40       	mulsd  xmm1,QWORD PTR [rdx+0x40]
   1800181ed:	0f 29 70 e8          	movaps XMMWORD PTR [rax-0x18],xmm6
   1800181f1:	0f 28 f2             	movaps xmm6,xmm2
   1800181f4:	f2 0f 59 32          	mulsd  xmm6,QWORD PTR [rdx]
   1800181f8:	0f 29 78 d8          	movaps XMMWORD PTR [rax-0x28],xmm7
   1800181fc:	44 0f 29 40 c8       	movaps XMMWORD PTR [rax-0x38],xmm8
   180018201:	44 0f 29 48 b8       	movaps XMMWORD PTR [rax-0x48],xmm9
   180018206:	f2 0f 58 f0          	addsd  xmm6,xmm0
   18001820a:	44 0f 29 50 a8       	movaps XMMWORD PTR [rax-0x58],xmm10
   18001820f:	44 0f 29 58 98       	movaps XMMWORD PTR [rax-0x68],xmm11
   180018214:	0f 28 c5             	movaps xmm0,xmm5
   180018217:	f2 0f 59 42 60       	mulsd  xmm0,QWORD PTR [rdx+0x60]
   18001821c:	44 0f 29 60 88       	movaps XMMWORD PTR [rax-0x78],xmm12
   180018221:	f2 0f 58 f1          	addsd  xmm6,xmm1
   180018225:	44 0f 29 6c 24 30    	movaps XMMWORD PTR [rsp+0x30],xmm13
   18001822b:	44 0f 29 74 24 20    	movaps XMMWORD PTR [rsp+0x20],xmm14
   180018231:	44 0f 29 7c 24 10    	movaps XMMWORD PTR [rsp+0x10],xmm15
   180018237:	f2 0f 58 f0          	addsd  xmm6,xmm0
   18001823b:	f2 0f 11 70 08       	movsd  QWORD PTR [rax+0x8],xmm6
   180018240:	f2 0f 10 72 08       	movsd  xmm6,QWORD PTR [rdx+0x8]
   180018245:	f2 0f 10 42 28       	movsd  xmm0,QWORD PTR [rdx+0x28]
   18001824a:	f2 0f 10 4a 48       	movsd  xmm1,QWORD PTR [rdx+0x48]
   18001824f:	f2 0f 59 c4          	mulsd  xmm0,xmm4
   180018253:	f2 0f 59 f2          	mulsd  xmm6,xmm2
   180018257:	f2 0f 59 cb          	mulsd  xmm1,xmm3
   18001825b:	f2 0f 58 f0          	addsd  xmm6,xmm0
   18001825f:	f2 0f 10 42 68       	movsd  xmm0,QWORD PTR [rdx+0x68]
   180018264:	f2 0f 59 c5          	mulsd  xmm0,xmm5
   180018268:	f2 0f 58 f1          	addsd  xmm6,xmm1
   18001826c:	f2 0f 58 f0          	addsd  xmm6,xmm0
   180018270:	f2 0f 11 70 10       	movsd  QWORD PTR [rax+0x10],xmm6
   180018275:	f2 0f 10 42 30       	movsd  xmm0,QWORD PTR [rdx+0x30]
   18001827a:	f2 0f 10 4a 50       	movsd  xmm1,QWORD PTR [rdx+0x50]
   18001827f:	f2 44 0f 10 42 10    	movsd  xmm8,QWORD PTR [rdx+0x10]
   180018285:	f2 0f 59 c4          	mulsd  xmm0,xmm4
   180018289:	41 0f 28 f0          	movaps xmm6,xmm8
   18001828d:	f2 0f 59 cb          	mulsd  xmm1,xmm3
   180018291:	f2 0f 59 f2          	mulsd  xmm6,xmm2
   180018295:	f2 0f 58 f0          	addsd  xmm6,xmm0
   180018299:	f2 0f 10 42 70       	movsd  xmm0,QWORD PTR [rdx+0x70]
   18001829e:	f2 0f 59 c5          	mulsd  xmm0,xmm5
   1800182a2:	f2 0f 58 f1          	addsd  xmm6,xmm1
   1800182a6:	f2 0f 58 f0          	addsd  xmm6,xmm0
   1800182aa:	f2 0f 11 70 18       	movsd  QWORD PTR [rax+0x18],xmm6
   1800182af:	f2 0f 10 4a 58       	movsd  xmm1,QWORD PTR [rdx+0x58]
   1800182b4:	f2 0f 10 7a 38       	movsd  xmm7,QWORD PTR [rdx+0x38]
   1800182b9:	f2 0f 10 72 18       	movsd  xmm6,QWORD PTR [rdx+0x18]
   1800182be:	0f 28 c7             	movaps xmm0,xmm7
   1800182c1:	f2 0f 59 c4          	mulsd  xmm0,xmm4
   1800182c5:	44 0f 28 f6          	movaps xmm14,xmm6
   1800182c9:	f2 44 0f 59 f2       	mulsd  xmm14,xmm2
   1800182ce:	f2 0f 59 cb          	mulsd  xmm1,xmm3
   1800182d2:	f2 44 0f 58 f0       	addsd  xmm14,xmm0
   1800182d7:	f2 0f 10 42 78       	movsd  xmm0,QWORD PTR [rdx+0x78]
   1800182dc:	f2 0f 59 c5          	mulsd  xmm0,xmm5
   1800182e0:	f2 44 0f 58 f1       	addsd  xmm14,xmm1
   1800182e5:	f2 44 0f 58 f0       	addsd  xmm14,xmm0
   1800182ea:	f2 44 0f 11 70 20    	movsd  QWORD PTR [rax+0x20],xmm14
   1800182f0:	f2 0f 10 69 20       	movsd  xmm5,QWORD PTR [rcx+0x20]
   1800182f5:	f2 0f 10 51 28       	movsd  xmm2,QWORD PTR [rcx+0x28]
   1800182fa:	44 0f 28 fd          	movaps xmm15,xmm5
   1800182fe:	f2 44 0f 59 3a       	mulsd  xmm15,QWORD PTR [rdx]
   180018303:	0f 28 c2             	movaps xmm0,xmm2
   180018306:	f2 0f 59 42 20       	mulsd  xmm0,QWORD PTR [rdx+0x20]
   18001830b:	44 0f 28 f5          	movaps xmm14,xmm5
   18001830f:	f2 0f 10 59 30       	movsd  xmm3,QWORD PTR [rcx+0x30]
   180018314:	f2 0f 10 61 38       	movsd  xmm4,QWORD PTR [rcx+0x38]
   180018319:	0f 28 cb             	movaps xmm1,xmm3
   18001831c:	f2 0f 59 4a 40       	mulsd  xmm1,QWORD PTR [rdx+0x40]
   180018321:	f2 44 0f 58 f8       	addsd  xmm15,xmm0
   180018326:	0f 28 c4             	movaps xmm0,xmm4
   180018329:	f2 0f 59 42 60       	mulsd  xmm0,QWORD PTR [rdx+0x60]
   18001832e:	f2 44 0f 58 f9       	addsd  xmm15,xmm1
   180018333:	f2 44 0f 58 f8       	addsd  xmm15,xmm0
   180018338:	f2 44 0f 59 72 08    	mulsd  xmm14,QWORD PTR [rdx+0x8]
   18001833e:	0f 28 cb             	movaps xmm1,xmm3
   180018341:	f2 0f 59 4a 48       	mulsd  xmm1,QWORD PTR [rdx+0x48]
   180018346:	0f 28 c2             	movaps xmm0,xmm2
   180018349:	f2 0f 59 42 28       	mulsd  xmm0,QWORD PTR [rdx+0x28]
   18001834e:	44 0f 28 ed          	movaps xmm13,xmm5
   180018352:	f2 44 0f 10 61 40    	movsd  xmm12,QWORD PTR [rcx+0x40]
   180018358:	f2 45 0f 59 e8       	mulsd  xmm13,xmm8
   18001835d:	45 0f 28 d4          	movaps xmm10,xmm12
   180018361:	f2 44 0f 59 52 08    	mulsd  xmm10,QWORD PTR [rdx+0x8]
   180018367:	f2 44 0f 58 f0       	addsd  xmm14,xmm0
   18001836c:	f2 0f 59 ee          	mulsd  xmm5,xmm6
   180018370:	45 0f 28 cc          	movaps xmm9,xmm12
   180018374:	0f 28 c4             	movaps xmm0,xmm4
   180018377:	f2 45 0f 59 c8       	mulsd  xmm9,xmm8
   18001837c:	f2 0f 59 42 68       	mulsd  xmm0,QWORD PTR [rdx+0x68]
   180018381:	f2 44 0f 58 f1       	addsd  xmm14,xmm1
   180018386:	f2 44 0f 10 41 60    	movsd  xmm8,QWORD PTR [rcx+0x60]
   18001838c:	0f 28 cb             	movaps xmm1,xmm3
   18001838f:	f2 0f 59 4a 50       	mulsd  xmm1,QWORD PTR [rdx+0x50]
   180018394:	f2 0f 59 5a 58       	mulsd  xmm3,QWORD PTR [rdx+0x58]
   180018399:	f2 44 0f 58 f0       	addsd  xmm14,xmm0
   18001839e:	0f 28 c2             	movaps xmm0,xmm2
   1800183a1:	f2 0f 59 d7          	mulsd  xmm2,xmm7
   1800183a5:	f2 0f 59 42 30       	mulsd  xmm0,QWORD PTR [rdx+0x30]
   1800183aa:	f2 0f 58 ea          	addsd  xmm5,xmm2
   1800183ae:	f2 0f 10 51 48       	movsd  xmm2,QWORD PTR [rcx+0x48]
   1800183b3:	44 0f 28 da          	movaps xmm11,xmm2
   1800183b7:	f2 44 0f 59 5a 20    	mulsd  xmm11,QWORD PTR [rdx+0x20]
   1800183bd:	f2 44 0f 58 e8       	addsd  xmm13,xmm0
   1800183c2:	0f 28 c4             	movaps xmm0,xmm4
   1800183c5:	f2 0f 59 62 78       	mulsd  xmm4,QWORD PTR [rdx+0x78]
   1800183ca:	f2 0f 59 42 70       	mulsd  xmm0,QWORD PTR [rdx+0x70]
   1800183cf:	f2 0f 58 eb          	addsd  xmm5,xmm3
   1800183d3:	f2 0f 10 59 50       	movsd  xmm3,QWORD PTR [rcx+0x50]
   1800183d8:	f2 44 0f 58 e9       	addsd  xmm13,xmm1
   1800183dd:	0f 28 cb             	movaps xmm1,xmm3
   1800183e0:	f2 0f 59 4a 40       	mulsd  xmm1,QWORD PTR [rdx+0x40]
   1800183e5:	f2 0f 58 ec          	addsd  xmm5,xmm4
   1800183e9:	f2 0f 10 61 58       	movsd  xmm4,QWORD PTR [rcx+0x58]
   1800183ee:	f2 44 0f 58 e8       	addsd  xmm13,xmm0
   1800183f3:	41 0f 28 c4          	movaps xmm0,xmm12
   1800183f7:	f2 0f 59 02          	mulsd  xmm0,QWORD PTR [rdx]
   1800183fb:	f2 44 0f 59 e6       	mulsd  xmm12,xmm6
   180018400:	f2 0f 10 71 78       	movsd  xmm6,QWORD PTR [rcx+0x78]
   180018405:	f2 44 0f 58 d8       	addsd  xmm11,xmm0
   18001840a:	f2 0f 11 2c 24       	movsd  QWORD PTR [rsp],xmm5
   18001840f:	0f 28 c4             	movaps xmm0,xmm4
   180018412:	f2 0f 59 42 60       	mulsd  xmm0,QWORD PTR [rdx+0x60]
   180018417:	f2 44 0f 58 d9       	addsd  xmm11,xmm1
   18001841c:	0f 28 cb             	movaps xmm1,xmm3
   18001841f:	f2 0f 59 4a 48       	mulsd  xmm1,QWORD PTR [rdx+0x48]
   180018424:	f2 44 0f 58 d8       	addsd  xmm11,xmm0
   180018429:	0f 28 c2             	movaps xmm0,xmm2
   18001842c:	f2 0f 59 42 28       	mulsd  xmm0,QWORD PTR [rdx+0x28]
   180018431:	f2 44 0f 58 d0       	addsd  xmm10,xmm0
   180018436:	0f 28 c4             	movaps xmm0,xmm4
   180018439:	f2 0f 59 42 68       	mulsd  xmm0,QWORD PTR [rdx+0x68]
   18001843e:	f2 44 0f 58 d1       	addsd  xmm10,xmm1
   180018443:	0f 28 cb             	movaps xmm1,xmm3
   180018446:	f2 0f 59 5a 58       	mulsd  xmm3,QWORD PTR [rdx+0x58]
   18001844b:	f2 0f 59 4a 50       	mulsd  xmm1,QWORD PTR [rdx+0x50]
   180018450:	f2 44 0f 58 d0       	addsd  xmm10,xmm0
   180018455:	0f 28 c2             	movaps xmm0,xmm2
   180018458:	f2 0f 59 42 30       	mulsd  xmm0,QWORD PTR [rdx+0x30]
   18001845d:	f2 0f 59 d7          	mulsd  xmm2,xmm7
   180018461:	41 0f 28 f8          	movaps xmm7,xmm8
   180018465:	f2 0f 59 3a          	mulsd  xmm7,QWORD PTR [rdx]
   180018469:	f2 44 0f 58 c8       	addsd  xmm9,xmm0
   18001846e:	0f 28 c4             	movaps xmm0,xmm4
   180018471:	f2 0f 59 42 70       	mulsd  xmm0,QWORD PTR [rdx+0x70]
   180018476:	f2 44 0f 58 e2       	addsd  xmm12,xmm2
   18001847b:	f2 0f 59 62 78       	mulsd  xmm4,QWORD PTR [rdx+0x78]
   180018480:	f2 44 0f 58 c9       	addsd  xmm9,xmm1
   180018485:	f2 44 0f 58 e3       	addsd  xmm12,xmm3
   18001848a:	f2 0f 10 59 68       	movsd  xmm3,QWORD PTR [rcx+0x68]
   18001848f:	f2 44 0f 58 c8       	addsd  xmm9,xmm0
   180018494:	0f 28 c3             	movaps xmm0,xmm3
   180018497:	f2 0f 59 42 20       	mulsd  xmm0,QWORD PTR [rdx+0x20]
   18001849c:	f2 44 0f 58 e4       	addsd  xmm12,xmm4
   1800184a1:	f2 0f 10 61 70       	movsd  xmm4,QWORD PTR [rcx+0x70]
   1800184a6:	f2 0f 58 f8          	addsd  xmm7,xmm0
   1800184aa:	41 0f 28 e8          	movaps xmm5,xmm8
   1800184ae:	f2 0f 59 6a 08       	mulsd  xmm5,QWORD PTR [rdx+0x8]
   1800184b3:	4c 8d 9c 24 b8 00 00 	lea    r11,[rsp+0xb8]
   1800184ba:	00 
   1800184bb:	41 0f 28 d0          	movaps xmm2,xmm8
   1800184bf:	0f 28 c6             	movaps xmm0,xmm6
   1800184c2:	f2 0f 59 42 60       	mulsd  xmm0,QWORD PTR [rdx+0x60]
   1800184c7:	0f 28 cc             	movaps xmm1,xmm4
   1800184ca:	f2 0f 59 4a 40       	mulsd  xmm1,QWORD PTR [rdx+0x40]
   1800184cf:	f2 44 0f 59 42 18    	mulsd  xmm8,QWORD PTR [rdx+0x18]
   1800184d5:	f2 0f 59 52 10       	mulsd  xmm2,QWORD PTR [rdx+0x10]
   1800184da:	f2 0f 58 f9          	addsd  xmm7,xmm1
   1800184de:	0f 28 cc             	movaps xmm1,xmm4
   1800184e1:	f2 0f 59 4a 48       	mulsd  xmm1,QWORD PTR [rdx+0x48]
   1800184e6:	f2 0f 58 f8          	addsd  xmm7,xmm0
   1800184ea:	0f 28 c3             	movaps xmm0,xmm3
   1800184ed:	f2 0f 59 42 28       	mulsd  xmm0,QWORD PTR [rdx+0x28]
   1800184f2:	f2 0f 58 e8          	addsd  xmm5,xmm0
   1800184f6:	0f 28 c6             	movaps xmm0,xmm6
   1800184f9:	f2 0f 59 42 68       	mulsd  xmm0,QWORD PTR [rdx+0x68]
   1800184fe:	f2 0f 58 e9          	addsd  xmm5,xmm1
   180018502:	0f 28 cc             	movaps xmm1,xmm4
   180018505:	f2 0f 59 4a 50       	mulsd  xmm1,QWORD PTR [rdx+0x50]
   18001850a:	f2 0f 59 62 58       	mulsd  xmm4,QWORD PTR [rdx+0x58]
   18001850f:	f2 0f 58 e8          	addsd  xmm5,xmm0
   180018513:	0f 28 c3             	movaps xmm0,xmm3
   180018516:	f2 0f 59 42 30       	mulsd  xmm0,QWORD PTR [rdx+0x30]
   18001851b:	f2 0f 59 5a 38       	mulsd  xmm3,QWORD PTR [rdx+0x38]
   180018520:	f2 0f 58 d0          	addsd  xmm2,xmm0
   180018524:	0f 28 c6             	movaps xmm0,xmm6
   180018527:	f2 0f 59 42 70       	mulsd  xmm0,QWORD PTR [rdx+0x70]
   18001852c:	f2 0f 59 72 78       	mulsd  xmm6,QWORD PTR [rdx+0x78]
   180018531:	f2 44 0f 58 c3       	addsd  xmm8,xmm3
   180018536:	f2 0f 58 d1          	addsd  xmm2,xmm1
   18001853a:	f2 44 0f 58 c4       	addsd  xmm8,xmm4
   18001853f:	f2 0f 58 d0          	addsd  xmm2,xmm0
   180018543:	f2 0f 10 40 08       	movsd  xmm0,QWORD PTR [rax+0x8]
   180018548:	f2 0f 11 01          	movsd  QWORD PTR [rcx],xmm0
   18001854c:	f2 0f 10 40 10       	movsd  xmm0,QWORD PTR [rax+0x10]
   180018551:	f2 0f 11 41 08       	movsd  QWORD PTR [rcx+0x8],xmm0
   180018556:	f2 44 0f 58 c6       	addsd  xmm8,xmm6
   18001855b:	f2 0f 10 40 18       	movsd  xmm0,QWORD PTR [rax+0x18]
   180018560:	41 0f 28 73 e8       	movaps xmm6,XMMWORD PTR [r11-0x18]
   180018565:	f2 0f 11 41 10       	movsd  QWORD PTR [rcx+0x10],xmm0
   18001856a:	f2 0f 10 40 20       	movsd  xmm0,QWORD PTR [rax+0x20]
   18001856f:	48 8b c1             	mov    rax,rcx
   180018572:	f2 0f 11 41 18       	movsd  QWORD PTR [rcx+0x18],xmm0
   180018577:	f2 0f 10 04 24       	movsd  xmm0,QWORD PTR [rsp]
   18001857c:	f2 44 0f 11 79 20    	movsd  QWORD PTR [rcx+0x20],xmm15
   180018582:	44 0f 28 7c 24 10    	movaps xmm15,XMMWORD PTR [rsp+0x10]
   180018588:	f2 44 0f 11 71 28    	movsd  QWORD PTR [rcx+0x28],xmm14
   18001858e:	44 0f 28 74 24 20    	movaps xmm14,XMMWORD PTR [rsp+0x20]
   180018594:	f2 44 0f 11 69 30    	movsd  QWORD PTR [rcx+0x30],xmm13
   18001859a:	44 0f 28 6c 24 30    	movaps xmm13,XMMWORD PTR [rsp+0x30]
   1800185a0:	f2 44 0f 11 59 40    	movsd  QWORD PTR [rcx+0x40],xmm11
   1800185a6:	45 0f 28 5b 98       	movaps xmm11,XMMWORD PTR [r11-0x68]
   1800185ab:	f2 44 0f 11 51 48    	movsd  QWORD PTR [rcx+0x48],xmm10
   1800185b1:	45 0f 28 53 a8       	movaps xmm10,XMMWORD PTR [r11-0x58]
   1800185b6:	f2 44 0f 11 49 50    	movsd  QWORD PTR [rcx+0x50],xmm9
   1800185bc:	45 0f 28 4b b8       	movaps xmm9,XMMWORD PTR [r11-0x48]
   1800185c1:	f2 44 0f 11 61 58    	movsd  QWORD PTR [rcx+0x58],xmm12
   1800185c7:	45 0f 28 63 88       	movaps xmm12,XMMWORD PTR [r11-0x78]
   1800185cc:	f2 0f 11 79 60       	movsd  QWORD PTR [rcx+0x60],xmm7
   1800185d1:	41 0f 28 7b d8       	movaps xmm7,XMMWORD PTR [r11-0x28]
   1800185d6:	f2 44 0f 11 41 78    	movsd  QWORD PTR [rcx+0x78],xmm8
   1800185dc:	45 0f 28 43 c8       	movaps xmm8,XMMWORD PTR [r11-0x38]
   1800185e1:	f2 0f 11 41 38       	movsd  QWORD PTR [rcx+0x38],xmm0
   1800185e6:	f2 0f 11 69 68       	movsd  QWORD PTR [rcx+0x68],xmm5
   1800185eb:	f2 0f 11 51 70       	movsd  QWORD PTR [rcx+0x70],xmm2
   1800185f0:	49 8b e3             	mov    rsp,r11
   1800185f3:	c3                   	ret
