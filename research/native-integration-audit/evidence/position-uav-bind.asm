
upload/renderer_rmdwin10_f(4).dll:     file format pei-x86-64


Disassembly of section .text:

00000001800a3270 <.text+0xa2270>:
   1800a3270:	41 56                	push   r14
   1800a3272:	48 83 ec 40          	sub    rsp,0x40
   1800a3276:	48 c7 44 24 20 fe ff 	mov    QWORD PTR [rsp+0x20],0xfffffffffffffffe
   1800a327d:	ff ff 
   1800a327f:	48 89 5c 24 50       	mov    QWORD PTR [rsp+0x50],rbx
   1800a3284:	48 89 6c 24 58       	mov    QWORD PTR [rsp+0x58],rbp
   1800a3289:	48 89 74 24 60       	mov    QWORD PTR [rsp+0x60],rsi
   1800a328e:	48 89 7c 24 68       	mov    QWORD PTR [rsp+0x68],rdi
   1800a3293:	48 8b f9             	mov    rdi,rcx
   1800a3296:	48 8b 09             	mov    rcx,QWORD PTR [rcx]
   1800a3299:	ff 15 91 ca 53 00    	call   QWORD PTR [rip+0x53ca91]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   1800a329f:	48 8b c8             	mov    rcx,rax
   1800a32a2:	ff 15 c8 cc 53 00    	call   QWORD PTR [rip+0x53ccc8]        # 0x1805dff70 ; ?prepareAsUAV@NativeTexture@d3d@@QEAAXXZ
   1800a32a8:	48 8b 4f 08          	mov    rcx,QWORD PTR [rdi+0x8]
   1800a32ac:	ff 15 7e ca 53 00    	call   QWORD PTR [rip+0x53ca7e]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   1800a32b2:	48 8b c8             	mov    rcx,rax
   1800a32b5:	ff 15 b5 cc 53 00    	call   QWORD PTR [rip+0x53ccb5]        # 0x1805dff70 ; ?prepareAsUAV@NativeTexture@d3d@@QEAAXXZ
   1800a32bb:	48 8b 4f 10          	mov    rcx,QWORD PTR [rdi+0x10]
   1800a32bf:	ff 15 6b ca 53 00    	call   QWORD PTR [rip+0x53ca6b]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   1800a32c5:	48 8b c8             	mov    rcx,rax
   1800a32c8:	ff 15 a2 cc 53 00    	call   QWORD PTR [rip+0x53cca2]        # 0x1805dff70 ; ?prepareAsUAV@NativeTexture@d3d@@QEAAXXZ
   1800a32ce:	48 8b 4f 18          	mov    rcx,QWORD PTR [rdi+0x18]
   1800a32d2:	48 85 c9             	test   rcx,rcx
   1800a32d5:	74 0f                	je     0x1800a32e6
   1800a32d7:	ff 15 53 ca 53 00    	call   QWORD PTR [rip+0x53ca53]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   1800a32dd:	48 8b c8             	mov    rcx,rax
   1800a32e0:	ff 15 8a cc 53 00    	call   QWORD PTR [rip+0x53cc8a]        # 0x1805dff70 ; ?prepareAsUAV@NativeTexture@d3d@@QEAAXXZ
   1800a32e6:	48 8b 4f 20          	mov    rcx,QWORD PTR [rdi+0x20]
   1800a32ea:	48 85 c9             	test   rcx,rcx
   1800a32ed:	74 0f                	je     0x1800a32fe
   1800a32ef:	ff 15 3b ca 53 00    	call   QWORD PTR [rip+0x53ca3b]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   1800a32f5:	48 8b c8             	mov    rcx,rax
   1800a32f8:	ff 15 72 cc 53 00    	call   QWORD PTR [rip+0x53cc72]        # 0x1805dff70 ; ?prepareAsUAV@NativeTexture@d3d@@QEAAXXZ
   1800a32fe:	48 8b 4f 28          	mov    rcx,QWORD PTR [rdi+0x28]
   1800a3302:	48 85 c9             	test   rcx,rcx
   1800a3305:	74 0f                	je     0x1800a3316
   1800a3307:	ff 15 23 ca 53 00    	call   QWORD PTR [rip+0x53ca23]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   1800a330d:	48 8b c8             	mov    rcx,rax
   1800a3310:	ff 15 5a cc 53 00    	call   QWORD PTR [rip+0x53cc5a]        # 0x1805dff70 ; ?prepareAsUAV@NativeTexture@d3d@@QEAAXXZ
   1800a3316:	48 8b 4f 30          	mov    rcx,QWORD PTR [rdi+0x30]
   1800a331a:	48 85 c9             	test   rcx,rcx
   1800a331d:	74 0f                	je     0x1800a332e
   1800a331f:	ff 15 0b ca 53 00    	call   QWORD PTR [rip+0x53ca0b]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   1800a3325:	48 8b c8             	mov    rcx,rax
   1800a3328:	ff 15 42 cc 53 00    	call   QWORD PTR [rip+0x53cc42]        # 0x1805dff70 ; ?prepareAsUAV@NativeTexture@d3d@@QEAAXXZ
   1800a332e:	8b 0d 08 f5 75 00    	mov    ecx,DWORD PTR [rip+0x75f508]        # 0x18080283c
   1800a3334:	65 48 8b 04 25 58 00 	mov    rax,QWORD PTR gs:0x58
   1800a333b:	00 00 
   1800a333d:	ba 40 00 00 00       	mov    edx,0x40
   1800a3342:	4c 8b 34 c8          	mov    r14,QWORD PTR [rax+rcx*8]
   1800a3346:	4c 03 f2             	add    r14,rdx
   1800a3349:	33 db                	xor    ebx,ebx
   1800a334b:	48 8d 2d 1e e4 1e 01 	lea    rbp,[rip+0x11ee41e]        # 0x181291770
   1800a3352:	41 8b 06             	mov    eax,DWORD PTR [r14]
   1800a3355:	39 05 fd e3 1e 01    	cmp    DWORD PTR [rip+0x11ee3fd],eax        # 0x181291758
   1800a335b:	7e 63                	jle    0x1800a33c0
   1800a335d:	48 8d 0d f4 e3 1e 01 	lea    rcx,[rip+0x11ee3f4]        # 0x181291758
   1800a3364:	e8 fb 4a 4e 00       	call   0x180587e64
   1800a3369:	83 3d e8 e3 1e 01 ff 	cmp    DWORD PTR [rip+0x11ee3e8],0xffffffff        # 0x181291758
   1800a3370:	75 4e                	jne    0x1800a33c0
   1800a3372:	48 89 1d e7 e3 1e 01 	mov    QWORD PTR [rip+0x11ee3e7],rbx        # 0x181291760
   1800a3379:	48 8d 15 20 44 58 00 	lea    rdx,[rip+0x584420]        # 0x1806277a0 ; 'g_rwtMaterialId'
   1800a3380:	48 8d 0d e1 e3 1e 01 	lea    rcx,[rip+0x11ee3e1]        # 0x181291768
   1800a3387:	e8 e4 ce 16 00       	call   0x180210270
   1800a338c:	c7 05 d6 e3 1e 01 09 	mov    DWORD PTR [rip+0x11ee3d6],0x9        # 0x18129176c
   1800a3393:	00 00 00 
   1800a3396:	48 89 2d c3 e3 1e 01 	mov    QWORD PTR [rip+0x11ee3c3],rbp        # 0x181291760
   1800a339d:	48 8b cd             	mov    rcx,rbp
   1800a33a0:	ff 15 ca c8 53 00    	call   QWORD PTR [rip+0x53c8ca]        # 0x1805dfc70 ; ??0ShaderTexture@d3d@@QEAA@XZ
   1800a33a6:	90                   	nop
   1800a33a7:	48 8d 0d 42 78 52 00 	lea    rcx,[rip+0x527842]        # 0x1805cabf0
   1800a33ae:	e8 11 48 4e 00       	call   0x180587bc4
   1800a33b3:	90                   	nop
   1800a33b4:	48 8d 0d 9d e3 1e 01 	lea    rcx,[rip+0x11ee39d]        # 0x181291758
   1800a33bb:	e8 44 4a 4e 00       	call   0x180587e04
   1800a33c0:	48 8b 0f             	mov    rcx,QWORD PTR [rdi]
   1800a33c3:	48 85 c9             	test   rcx,rcx
   1800a33c6:	74 0b                	je     0x1800a33d3
   1800a33c8:	ff 15 62 c9 53 00    	call   QWORD PTR [rip+0x53c962]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   1800a33ce:	48 8b f0             	mov    rsi,rax
   1800a33d1:	eb 03                	jmp    0x1800a33d6
   1800a33d3:	48 8b f3             	mov    rsi,rbx
   1800a33d6:	8b 0d 8c e3 1e 01    	mov    ecx,DWORD PTR [rip+0x11ee38c]        # 0x181291768
   1800a33dc:	ff 15 a6 c8 53 00    	call   QWORD PTR [rip+0x53c8a6]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   1800a33e2:	8b d0                	mov    edx,eax
   1800a33e4:	4c 8b c6             	mov    r8,rsi
   1800a33e7:	48 8d 4c 24 28       	lea    rcx,[rsp+0x28]
   1800a33ec:	ff 15 8e c8 53 00    	call   QWORD PTR [rip+0x53c88e]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   1800a33f2:	45 33 c0             	xor    r8d,r8d
   1800a33f5:	48 8d 54 24 28       	lea    rdx,[rsp+0x28]
   1800a33fa:	8b 0d 68 e3 1e 01    	mov    ecx,DWORD PTR [rip+0x11ee368]        # 0x181291768
   1800a3400:	ff 15 8a cb 53 00    	call   QWORD PTR [rip+0x53cb8a]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   1800a3406:	8b 0d 5c e3 1e 01    	mov    ecx,DWORD PTR [rip+0x11ee35c]        # 0x181291768
   1800a340c:	ff 15 76 c8 53 00    	call   QWORD PTR [rip+0x53c876]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   1800a3412:	8b d0                	mov    edx,eax
   1800a3414:	4c 8b c6             	mov    r8,rsi
   1800a3417:	48 8b cd             	mov    rcx,rbp
   1800a341a:	ff 15 70 c8 53 00    	call   QWORD PTR [rip+0x53c870]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   1800a3420:	48 8d 2d 71 e3 1e 01 	lea    rbp,[rip+0x11ee371]        # 0x181291798
   1800a3427:	41 8b 06             	mov    eax,DWORD PTR [r14]
   1800a342a:	39 05 50 e3 1e 01    	cmp    DWORD PTR [rip+0x11ee350],eax        # 0x181291780
   1800a3430:	7e 63                	jle    0x1800a3495
   1800a3432:	48 8d 0d 47 e3 1e 01 	lea    rcx,[rip+0x11ee347]        # 0x181291780
   1800a3439:	e8 26 4a 4e 00       	call   0x180587e64
   1800a343e:	83 3d 3b e3 1e 01 ff 	cmp    DWORD PTR [rip+0x11ee33b],0xffffffff        # 0x181291780
   1800a3445:	75 4e                	jne    0x1800a3495
   1800a3447:	48 89 1d 3a e3 1e 01 	mov    QWORD PTR [rip+0x11ee33a],rbx        # 0x181291788
   1800a344e:	48 8d 15 5b 43 58 00 	lea    rdx,[rip+0x58435b]        # 0x1806277b0 ; 'g_rwtNormal_TexcoordX'
   1800a3455:	48 8d 0d 34 e3 1e 01 	lea    rcx,[rip+0x11ee334]        # 0x181291790
   1800a345c:	e8 0f ce 16 00       	call   0x180210270
   1800a3461:	c7 05 29 e3 1e 01 09 	mov    DWORD PTR [rip+0x11ee329],0x9        # 0x181291794
   1800a3468:	00 00 00 
   1800a346b:	48 89 2d 16 e3 1e 01 	mov    QWORD PTR [rip+0x11ee316],rbp        # 0x181291788
   1800a3472:	48 8b cd             	mov    rcx,rbp
   1800a3475:	ff 15 f5 c7 53 00    	call   QWORD PTR [rip+0x53c7f5]        # 0x1805dfc70 ; ??0ShaderTexture@d3d@@QEAA@XZ
   1800a347b:	90                   	nop
   1800a347c:	48 8d 0d 3d 77 52 00 	lea    rcx,[rip+0x52773d]        # 0x1805cabc0
   1800a3483:	e8 3c 47 4e 00       	call   0x180587bc4
   1800a3488:	90                   	nop
   1800a3489:	48 8d 0d f0 e2 1e 01 	lea    rcx,[rip+0x11ee2f0]        # 0x181291780
   1800a3490:	e8 6f 49 4e 00       	call   0x180587e04
   1800a3495:	48 8b 4f 08          	mov    rcx,QWORD PTR [rdi+0x8]
   1800a3499:	48 85 c9             	test   rcx,rcx
   1800a349c:	74 0b                	je     0x1800a34a9
   1800a349e:	ff 15 8c c8 53 00    	call   QWORD PTR [rip+0x53c88c]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   1800a34a4:	48 8b f0             	mov    rsi,rax
   1800a34a7:	eb 03                	jmp    0x1800a34ac
   1800a34a9:	48 8b f3             	mov    rsi,rbx
   1800a34ac:	8b 0d de e2 1e 01    	mov    ecx,DWORD PTR [rip+0x11ee2de]        # 0x181291790
   1800a34b2:	ff 15 d0 c7 53 00    	call   QWORD PTR [rip+0x53c7d0]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   1800a34b8:	8b d0                	mov    edx,eax
   1800a34ba:	4c 8b c6             	mov    r8,rsi
   1800a34bd:	48 8d 4c 24 28       	lea    rcx,[rsp+0x28]
   1800a34c2:	ff 15 b8 c7 53 00    	call   QWORD PTR [rip+0x53c7b8]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   1800a34c8:	45 33 c0             	xor    r8d,r8d
   1800a34cb:	48 8d 54 24 28       	lea    rdx,[rsp+0x28]
   1800a34d0:	8b 0d ba e2 1e 01    	mov    ecx,DWORD PTR [rip+0x11ee2ba]        # 0x181291790
   1800a34d6:	ff 15 b4 ca 53 00    	call   QWORD PTR [rip+0x53cab4]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   1800a34dc:	8b 0d ae e2 1e 01    	mov    ecx,DWORD PTR [rip+0x11ee2ae]        # 0x181291790
   1800a34e2:	ff 15 a0 c7 53 00    	call   QWORD PTR [rip+0x53c7a0]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   1800a34e8:	8b d0                	mov    edx,eax
   1800a34ea:	4c 8b c6             	mov    r8,rsi
   1800a34ed:	48 8b cd             	mov    rcx,rbp
   1800a34f0:	ff 15 9a c7 53 00    	call   QWORD PTR [rip+0x53c79a]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   1800a34f6:	48 8d 2d c3 e2 1e 01 	lea    rbp,[rip+0x11ee2c3]        # 0x1812917c0
   1800a34fd:	41 8b 06             	mov    eax,DWORD PTR [r14]
   1800a3500:	39 05 a2 e2 1e 01    	cmp    DWORD PTR [rip+0x11ee2a2],eax        # 0x1812917a8
   1800a3506:	7e 63                	jle    0x1800a356b
   1800a3508:	48 8d 0d 99 e2 1e 01 	lea    rcx,[rip+0x11ee299]        # 0x1812917a8
   1800a350f:	e8 50 49 4e 00       	call   0x180587e64
   1800a3514:	83 3d 8d e2 1e 01 ff 	cmp    DWORD PTR [rip+0x11ee28d],0xffffffff        # 0x1812917a8
   1800a351b:	75 4e                	jne    0x1800a356b
   1800a351d:	48 89 1d 8c e2 1e 01 	mov    QWORD PTR [rip+0x11ee28c],rbx        # 0x1812917b0
   1800a3524:	48 8d 15 9d 42 58 00 	lea    rdx,[rip+0x58429d]        # 0x1806277c8 ; 'g_rwtPosition_TexcoordY'
   1800a352b:	48 8d 0d 86 e2 1e 01 	lea    rcx,[rip+0x11ee286]        # 0x1812917b8
   1800a3532:	e8 39 cd 16 00       	call   0x180210270
   1800a3537:	c7 05 7b e2 1e 01 09 	mov    DWORD PTR [rip+0x11ee27b],0x9        # 0x1812917bc
   1800a353e:	00 00 00 
   1800a3541:	48 89 2d 68 e2 1e 01 	mov    QWORD PTR [rip+0x11ee268],rbp        # 0x1812917b0
   1800a3548:	48 8b cd             	mov    rcx,rbp
   1800a354b:	ff 15 1f c7 53 00    	call   QWORD PTR [rip+0x53c71f]        # 0x1805dfc70 ; ??0ShaderTexture@d3d@@QEAA@XZ
   1800a3551:	90                   	nop
   1800a3552:	48 8d 0d 37 76 52 00 	lea    rcx,[rip+0x527637]        # 0x1805cab90
   1800a3559:	e8 66 46 4e 00       	call   0x180587bc4
   1800a355e:	90                   	nop
   1800a355f:	48 8d 0d 42 e2 1e 01 	lea    rcx,[rip+0x11ee242]        # 0x1812917a8
   1800a3566:	e8 99 48 4e 00       	call   0x180587e04
   1800a356b:	48 8b 4f 10          	mov    rcx,QWORD PTR [rdi+0x10]
   1800a356f:	48 85 c9             	test   rcx,rcx
   1800a3572:	74 0b                	je     0x1800a357f
   1800a3574:	ff 15 b6 c7 53 00    	call   QWORD PTR [rip+0x53c7b6]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   1800a357a:	48 8b f0             	mov    rsi,rax
   1800a357d:	eb 03                	jmp    0x1800a3582
   1800a357f:	48 8b f3             	mov    rsi,rbx
   1800a3582:	8b 0d 30 e2 1e 01    	mov    ecx,DWORD PTR [rip+0x11ee230]        # 0x1812917b8
   1800a3588:	ff 15 fa c6 53 00    	call   QWORD PTR [rip+0x53c6fa]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   1800a358e:	8b d0                	mov    edx,eax
   1800a3590:	4c 8b c6             	mov    r8,rsi
   1800a3593:	48 8d 4c 24 28       	lea    rcx,[rsp+0x28]
   1800a3598:	ff 15 e2 c6 53 00    	call   QWORD PTR [rip+0x53c6e2]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   1800a359e:	45 33 c0             	xor    r8d,r8d
   1800a35a1:	48 8d 54 24 28       	lea    rdx,[rsp+0x28]
   1800a35a6:	8b 0d 0c e2 1e 01    	mov    ecx,DWORD PTR [rip+0x11ee20c]        # 0x1812917b8
   1800a35ac:	ff 15 de c9 53 00    	call   QWORD PTR [rip+0x53c9de]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   1800a35b2:	8b 0d 00 e2 1e 01    	mov    ecx,DWORD PTR [rip+0x11ee200]        # 0x1812917b8
   1800a35b8:	ff 15 ca c6 53 00    	call   QWORD PTR [rip+0x53c6ca]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   1800a35be:	8b d0                	mov    edx,eax
   1800a35c0:	4c 8b c6             	mov    r8,rsi
   1800a35c3:	48 8b cd             	mov    rcx,rbp
   1800a35c6:	ff 15 c4 c6 53 00    	call   QWORD PTR [rip+0x53c6c4]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   1800a35cc:	48 8d 2d 15 e2 1e 01 	lea    rbp,[rip+0x11ee215]        # 0x1812917e8
   1800a35d3:	41 8b 06             	mov    eax,DWORD PTR [r14]
   1800a35d6:	39 05 f4 e1 1e 01    	cmp    DWORD PTR [rip+0x11ee1f4],eax        # 0x1812917d0
   1800a35dc:	7e 63                	jle    0x1800a3641
   1800a35de:	48 8d 0d eb e1 1e 01 	lea    rcx,[rip+0x11ee1eb]        # 0x1812917d0
   1800a35e5:	e8 7a 48 4e 00       	call   0x180587e64
   1800a35ea:	83 3d df e1 1e 01 ff 	cmp    DWORD PTR [rip+0x11ee1df],0xffffffff        # 0x1812917d0
   1800a35f1:	75 4e                	jne    0x1800a3641
   1800a35f3:	48 89 1d de e1 1e 01 	mov    QWORD PTR [rip+0x11ee1de],rbx        # 0x1812917d8
   1800a35fa:	48 8d 15 df 41 58 00 	lea    rdx,[rip+0x5841df]        # 0x1806277e0 ; 'g_rwtAlbedoColor'
   1800a3601:	48 8d 0d d8 e1 1e 01 	lea    rcx,[rip+0x11ee1d8]        # 0x1812917e0
   1800a3608:	e8 63 cc 16 00       	call   0x180210270
   1800a360d:	c7 05 cd e1 1e 01 09 	mov    DWORD PTR [rip+0x11ee1cd],0x9        # 0x1812917e4
   1800a3614:	00 00 00 
   1800a3617:	48 89 2d ba e1 1e 01 	mov    QWORD PTR [rip+0x11ee1ba],rbp        # 0x1812917d8
   1800a361e:	48 8b cd             	mov    rcx,rbp
   1800a3621:	ff 15 49 c6 53 00    	call   QWORD PTR [rip+0x53c649]        # 0x1805dfc70 ; ??0ShaderTexture@d3d@@QEAA@XZ
   1800a3627:	90                   	nop
   1800a3628:	48 8d 0d 31 75 52 00 	lea    rcx,[rip+0x527531]        # 0x1805cab60
   1800a362f:	e8 90 45 4e 00       	call   0x180587bc4
   1800a3634:	90                   	nop
   1800a3635:	48 8d 0d 94 e1 1e 01 	lea    rcx,[rip+0x11ee194]        # 0x1812917d0
   1800a363c:	e8 c3 47 4e 00       	call   0x180587e04
   1800a3641:	48 8b 4f 18          	mov    rcx,QWORD PTR [rdi+0x18]
   1800a3645:	48 85 c9             	test   rcx,rcx
   1800a3648:	74 0b                	je     0x1800a3655
   1800a364a:	ff 15 e0 c6 53 00    	call   QWORD PTR [rip+0x53c6e0]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   1800a3650:	48 8b f0             	mov    rsi,rax
   1800a3653:	eb 03                	jmp    0x1800a3658
   1800a3655:	48 8b f3             	mov    rsi,rbx
   1800a3658:	8b 0d 82 e1 1e 01    	mov    ecx,DWORD PTR [rip+0x11ee182]        # 0x1812917e0
   1800a365e:	ff 15 24 c6 53 00    	call   QWORD PTR [rip+0x53c624]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   1800a3664:	8b d0                	mov    edx,eax
   1800a3666:	4c 8b c6             	mov    r8,rsi
   1800a3669:	48 8d 4c 24 28       	lea    rcx,[rsp+0x28]
   1800a366e:	ff 15 0c c6 53 00    	call   QWORD PTR [rip+0x53c60c]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   1800a3674:	45 33 c0             	xor    r8d,r8d
   1800a3677:	48 8d 54 24 28       	lea    rdx,[rsp+0x28]
   1800a367c:	8b 0d 5e e1 1e 01    	mov    ecx,DWORD PTR [rip+0x11ee15e]        # 0x1812917e0
   1800a3682:	ff 15 08 c9 53 00    	call   QWORD PTR [rip+0x53c908]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   1800a3688:	8b 0d 52 e1 1e 01    	mov    ecx,DWORD PTR [rip+0x11ee152]        # 0x1812917e0
   1800a368e:	ff 15 f4 c5 53 00    	call   QWORD PTR [rip+0x53c5f4]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   1800a3694:	8b d0                	mov    edx,eax
   1800a3696:	4c 8b c6             	mov    r8,rsi
   1800a3699:	48 8b cd             	mov    rcx,rbp
   1800a369c:	ff 15 ee c5 53 00    	call   QWORD PTR [rip+0x53c5ee]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   1800a36a2:	48 8d 2d 67 e1 1e 01 	lea    rbp,[rip+0x11ee167]        # 0x181291810
   1800a36a9:	41 8b 06             	mov    eax,DWORD PTR [r14]
   1800a36ac:	39 05 46 e1 1e 01    	cmp    DWORD PTR [rip+0x11ee146],eax        # 0x1812917f8
   1800a36b2:	7e 63                	jle    0x1800a3717
   1800a36b4:	48 8d 0d 3d e1 1e 01 	lea    rcx,[rip+0x11ee13d]        # 0x1812917f8
   1800a36bb:	e8 a4 47 4e 00       	call   0x180587e64
   1800a36c0:	83 3d 31 e1 1e 01 ff 	cmp    DWORD PTR [rip+0x11ee131],0xffffffff        # 0x1812917f8
   1800a36c7:	75 4e                	jne    0x1800a3717
   1800a36c9:	48 89 1d 30 e1 1e 01 	mov    QWORD PTR [rip+0x11ee130],rbx        # 0x181291800
   1800a36d0:	48 8d 15 21 41 58 00 	lea    rdx,[rip+0x584121]        # 0x1806277f8 ; 'g_rwtEmissionColor'
   1800a36d7:	48 8d 0d 2a e1 1e 01 	lea    rcx,[rip+0x11ee12a]        # 0x181291808
   1800a36de:	e8 8d cb 16 00       	call   0x180210270
   1800a36e3:	c7 05 1f e1 1e 01 09 	mov    DWORD PTR [rip+0x11ee11f],0x9        # 0x18129180c
   1800a36ea:	00 00 00 
   1800a36ed:	48 89 2d 0c e1 1e 01 	mov    QWORD PTR [rip+0x11ee10c],rbp        # 0x181291800
   1800a36f4:	48 8b cd             	mov    rcx,rbp
   1800a36f7:	ff 15 73 c5 53 00    	call   QWORD PTR [rip+0x53c573]        # 0x1805dfc70 ; ??0ShaderTexture@d3d@@QEAA@XZ
   1800a36fd:	90                   	nop
   1800a36fe:	48 8d 0d 2b 74 52 00 	lea    rcx,[rip+0x52742b]        # 0x1805cab30
   1800a3705:	e8 ba 44 4e 00       	call   0x180587bc4
   1800a370a:	90                   	nop
   1800a370b:	48 8d 0d e6 e0 1e 01 	lea    rcx,[rip+0x11ee0e6]        # 0x1812917f8
   1800a3712:	e8 ed 46 4e 00       	call   0x180587e04
   1800a3717:	48 8b 4f 20          	mov    rcx,QWORD PTR [rdi+0x20]
   1800a371b:	48 85 c9             	test   rcx,rcx
   1800a371e:	74 0b                	je     0x1800a372b
   1800a3720:	ff 15 0a c6 53 00    	call   QWORD PTR [rip+0x53c60a]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   1800a3726:	48 8b f0             	mov    rsi,rax
   1800a3729:	eb 03                	jmp    0x1800a372e
   1800a372b:	48 8b f3             	mov    rsi,rbx
   1800a372e:	8b 0d d4 e0 1e 01    	mov    ecx,DWORD PTR [rip+0x11ee0d4]        # 0x181291808
   1800a3734:	ff 15 4e c5 53 00    	call   QWORD PTR [rip+0x53c54e]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   1800a373a:	8b d0                	mov    edx,eax
   1800a373c:	4c 8b c6             	mov    r8,rsi
   1800a373f:	48 8d 4c 24 28       	lea    rcx,[rsp+0x28]
   1800a3744:	ff 15 36 c5 53 00    	call   QWORD PTR [rip+0x53c536]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   1800a374a:	45 33 c0             	xor    r8d,r8d
   1800a374d:	48 8d 54 24 28       	lea    rdx,[rsp+0x28]
   1800a3752:	8b 0d b0 e0 1e 01    	mov    ecx,DWORD PTR [rip+0x11ee0b0]        # 0x181291808
   1800a3758:	ff 15 32 c8 53 00    	call   QWORD PTR [rip+0x53c832]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   1800a375e:	8b 0d a4 e0 1e 01    	mov    ecx,DWORD PTR [rip+0x11ee0a4]        # 0x181291808
   1800a3764:	ff 15 1e c5 53 00    	call   QWORD PTR [rip+0x53c51e]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   1800a376a:	8b d0                	mov    edx,eax
   1800a376c:	4c 8b c6             	mov    r8,rsi
   1800a376f:	48 8b cd             	mov    rcx,rbp
   1800a3772:	ff 15 18 c5 53 00    	call   QWORD PTR [rip+0x53c518]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   1800a3778:	48 8d 2d b9 e0 1e 01 	lea    rbp,[rip+0x11ee0b9]        # 0x181291838
   1800a377f:	41 8b 06             	mov    eax,DWORD PTR [r14]
   1800a3782:	39 05 98 e0 1e 01    	cmp    DWORD PTR [rip+0x11ee098],eax        # 0x181291820
   1800a3788:	7e 63                	jle    0x1800a37ed
   1800a378a:	48 8d 0d 8f e0 1e 01 	lea    rcx,[rip+0x11ee08f]        # 0x181291820
   1800a3791:	e8 ce 46 4e 00       	call   0x180587e64
   1800a3796:	83 3d 83 e0 1e 01 ff 	cmp    DWORD PTR [rip+0x11ee083],0xffffffff        # 0x181291820
   1800a379d:	75 4e                	jne    0x1800a37ed
   1800a379f:	48 89 1d 82 e0 1e 01 	mov    QWORD PTR [rip+0x11ee082],rbx        # 0x181291828
   1800a37a6:	48 8d 15 63 40 58 00 	lea    rdx,[rip+0x584063]        # 0x180627810 ; 'g_rwtSmoothnessSpecularAlbedo'
   1800a37ad:	48 8d 0d 7c e0 1e 01 	lea    rcx,[rip+0x11ee07c]        # 0x181291830
   1800a37b4:	e8 b7 ca 16 00       	call   0x180210270
   1800a37b9:	c7 05 71 e0 1e 01 09 	mov    DWORD PTR [rip+0x11ee071],0x9        # 0x181291834
   1800a37c0:	00 00 00 
   1800a37c3:	48 89 2d 5e e0 1e 01 	mov    QWORD PTR [rip+0x11ee05e],rbp        # 0x181291828
   1800a37ca:	48 8b cd             	mov    rcx,rbp
   1800a37cd:	ff 15 9d c4 53 00    	call   QWORD PTR [rip+0x53c49d]        # 0x1805dfc70 ; ??0ShaderTexture@d3d@@QEAA@XZ
   1800a37d3:	90                   	nop
   1800a37d4:	48 8d 0d 25 73 52 00 	lea    rcx,[rip+0x527325]        # 0x1805cab00
   1800a37db:	e8 e4 43 4e 00       	call   0x180587bc4
   1800a37e0:	90                   	nop
   1800a37e1:	48 8d 0d 38 e0 1e 01 	lea    rcx,[rip+0x11ee038]        # 0x181291820
   1800a37e8:	e8 17 46 4e 00       	call   0x180587e04
   1800a37ed:	48 8b 4f 28          	mov    rcx,QWORD PTR [rdi+0x28]
   1800a37f1:	48 85 c9             	test   rcx,rcx
   1800a37f4:	74 0b                	je     0x1800a3801
   1800a37f6:	ff 15 34 c5 53 00    	call   QWORD PTR [rip+0x53c534]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   1800a37fc:	48 8b f0             	mov    rsi,rax
   1800a37ff:	eb 03                	jmp    0x1800a3804
   1800a3801:	48 8b f3             	mov    rsi,rbx
   1800a3804:	8b 0d 26 e0 1e 01    	mov    ecx,DWORD PTR [rip+0x11ee026]        # 0x181291830
   1800a380a:	ff 15 78 c4 53 00    	call   QWORD PTR [rip+0x53c478]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   1800a3810:	8b d0                	mov    edx,eax
   1800a3812:	4c 8b c6             	mov    r8,rsi
   1800a3815:	48 8d 4c 24 28       	lea    rcx,[rsp+0x28]
   1800a381a:	ff 15 60 c4 53 00    	call   QWORD PTR [rip+0x53c460]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   1800a3820:	45 33 c0             	xor    r8d,r8d
   1800a3823:	48 8d 54 24 28       	lea    rdx,[rsp+0x28]
   1800a3828:	8b 0d 02 e0 1e 01    	mov    ecx,DWORD PTR [rip+0x11ee002]        # 0x181291830
   1800a382e:	ff 15 5c c7 53 00    	call   QWORD PTR [rip+0x53c75c]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   1800a3834:	8b 0d f6 df 1e 01    	mov    ecx,DWORD PTR [rip+0x11edff6]        # 0x181291830
   1800a383a:	ff 15 48 c4 53 00    	call   QWORD PTR [rip+0x53c448]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   1800a3840:	8b d0                	mov    edx,eax
   1800a3842:	4c 8b c6             	mov    r8,rsi
   1800a3845:	48 8b cd             	mov    rcx,rbp
   1800a3848:	ff 15 42 c4 53 00    	call   QWORD PTR [rip+0x53c442]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   1800a384e:	48 8d 35 0b e0 1e 01 	lea    rsi,[rip+0x11ee00b]        # 0x181291860
   1800a3855:	41 8b 06             	mov    eax,DWORD PTR [r14]
   1800a3858:	39 05 ea df 1e 01    	cmp    DWORD PTR [rip+0x11edfea],eax        # 0x181291848
   1800a385e:	7e 63                	jle    0x1800a38c3
   1800a3860:	48 8d 0d e1 df 1e 01 	lea    rcx,[rip+0x11edfe1]        # 0x181291848
   1800a3867:	e8 f8 45 4e 00       	call   0x180587e64
   1800a386c:	83 3d d5 df 1e 01 ff 	cmp    DWORD PTR [rip+0x11edfd5],0xffffffff        # 0x181291848
   1800a3873:	75 4e                	jne    0x1800a38c3
   1800a3875:	48 89 1d d4 df 1e 01 	mov    QWORD PTR [rip+0x11edfd4],rbx        # 0x181291850
   1800a387c:	48 8d 15 ad 3f 58 00 	lea    rdx,[rip+0x583fad]        # 0x180627830 ; 'g_rwtShadow'
   1800a3883:	48 8d 0d ce df 1e 01 	lea    rcx,[rip+0x11edfce]        # 0x181291858
   1800a388a:	e8 e1 c9 16 00       	call   0x180210270
   1800a388f:	c7 05 c3 df 1e 01 09 	mov    DWORD PTR [rip+0x11edfc3],0x9        # 0x18129185c
   1800a3896:	00 00 00 
   1800a3899:	48 89 35 b0 df 1e 01 	mov    QWORD PTR [rip+0x11edfb0],rsi        # 0x181291850
   1800a38a0:	48 8b ce             	mov    rcx,rsi
   1800a38a3:	ff 15 c7 c3 53 00    	call   QWORD PTR [rip+0x53c3c7]        # 0x1805dfc70 ; ??0ShaderTexture@d3d@@QEAA@XZ
   1800a38a9:	90                   	nop
   1800a38aa:	48 8d 0d 1f 72 52 00 	lea    rcx,[rip+0x52721f]        # 0x1805caad0
   1800a38b1:	e8 0e 43 4e 00       	call   0x180587bc4
   1800a38b6:	90                   	nop
   1800a38b7:	48 8d 0d 8a df 1e 01 	lea    rcx,[rip+0x11edf8a]        # 0x181291848
   1800a38be:	e8 41 45 4e 00       	call   0x180587e04
   1800a38c3:	48 8b 4f 30          	mov    rcx,QWORD PTR [rdi+0x30]
   1800a38c7:	48 85 c9             	test   rcx,rcx
   1800a38ca:	74 09                	je     0x1800a38d5
   1800a38cc:	ff 15 5e c4 53 00    	call   QWORD PTR [rip+0x53c45e]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   1800a38d2:	48 8b d8             	mov    rbx,rax
   1800a38d5:	8b 0d 7d df 1e 01    	mov    ecx,DWORD PTR [rip+0x11edf7d]        # 0x181291858
   1800a38db:	ff 15 a7 c3 53 00    	call   QWORD PTR [rip+0x53c3a7]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   1800a38e1:	8b d0                	mov    edx,eax
   1800a38e3:	4c 8b c3             	mov    r8,rbx
   1800a38e6:	48 8d 4c 24 28       	lea    rcx,[rsp+0x28]
   1800a38eb:	ff 15 8f c3 53 00    	call   QWORD PTR [rip+0x53c38f]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   1800a38f1:	45 33 c0             	xor    r8d,r8d
   1800a38f4:	48 8d 54 24 28       	lea    rdx,[rsp+0x28]
   1800a38f9:	8b 0d 59 df 1e 01    	mov    ecx,DWORD PTR [rip+0x11edf59]        # 0x181291858
   1800a38ff:	ff 15 8b c6 53 00    	call   QWORD PTR [rip+0x53c68b]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   1800a3905:	8b 0d 4d df 1e 01    	mov    ecx,DWORD PTR [rip+0x11edf4d]        # 0x181291858
   1800a390b:	ff 15 77 c3 53 00    	call   QWORD PTR [rip+0x53c377]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   1800a3911:	8b d0                	mov    edx,eax
   1800a3913:	4c 8b c3             	mov    r8,rbx
   1800a3916:	48 8b ce             	mov    rcx,rsi
   1800a3919:	ff 15 71 c3 53 00    	call   QWORD PTR [rip+0x53c371]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   1800a391f:	48 8b 5c 24 50       	mov    rbx,QWORD PTR [rsp+0x50]
   1800a3924:	48 8b 6c 24 58       	mov    rbp,QWORD PTR [rsp+0x58]
   1800a3929:	48 8b 74 24 60       	mov    rsi,QWORD PTR [rsp+0x60]
   1800a392e:	48 8b 7c 24 68       	mov    rdi,QWORD PTR [rsp+0x68]
   1800a3933:	48 83 c4 40          	add    rsp,0x40
   1800a3937:	41 5e                	pop    r14
   1800a3939:	c3                   	ret
