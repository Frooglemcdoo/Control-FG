
upload/renderer_rmdwin10_f(4).dll:     file format pei-x86-64


Disassembly of section .text:

00000001800a3940 <.text+0xa2940>:
   1800a3940:	41 56                	push   r14
   1800a3942:	48 83 ec 40          	sub    rsp,0x40
   1800a3946:	48 c7 44 24 20 fe ff 	mov    QWORD PTR [rsp+0x20],0xfffffffffffffffe
   1800a394d:	ff ff 
   1800a394f:	48 89 5c 24 50       	mov    QWORD PTR [rsp+0x50],rbx
   1800a3954:	48 89 6c 24 58       	mov    QWORD PTR [rsp+0x58],rbp
   1800a3959:	48 89 74 24 60       	mov    QWORD PTR [rsp+0x60],rsi
   1800a395e:	48 89 7c 24 68       	mov    QWORD PTR [rsp+0x68],rdi
   1800a3963:	48 8b f9             	mov    rdi,rcx
   1800a3966:	48 8b 09             	mov    rcx,QWORD PTR [rcx]
   1800a3969:	ff 15 c1 c3 53 00    	call   QWORD PTR [rip+0x53c3c1]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   1800a396f:	48 8b c8             	mov    rcx,rax
   1800a3972:	ff 15 00 c6 53 00    	call   QWORD PTR [rip+0x53c600]        # 0x1805dff78 ; ?prepareForConsuming@NativeTexture@d3d@@QEAAXXZ
   1800a3978:	48 8b 4f 08          	mov    rcx,QWORD PTR [rdi+0x8]
   1800a397c:	ff 15 ae c3 53 00    	call   QWORD PTR [rip+0x53c3ae]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   1800a3982:	48 8b c8             	mov    rcx,rax
   1800a3985:	ff 15 ed c5 53 00    	call   QWORD PTR [rip+0x53c5ed]        # 0x1805dff78 ; ?prepareForConsuming@NativeTexture@d3d@@QEAAXXZ
   1800a398b:	48 8b 4f 10          	mov    rcx,QWORD PTR [rdi+0x10]
   1800a398f:	ff 15 9b c3 53 00    	call   QWORD PTR [rip+0x53c39b]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   1800a3995:	48 8b c8             	mov    rcx,rax
   1800a3998:	ff 15 da c5 53 00    	call   QWORD PTR [rip+0x53c5da]        # 0x1805dff78 ; ?prepareForConsuming@NativeTexture@d3d@@QEAAXXZ
   1800a399e:	48 8b 4f 18          	mov    rcx,QWORD PTR [rdi+0x18]
   1800a39a2:	48 85 c9             	test   rcx,rcx
   1800a39a5:	74 0f                	je     0x1800a39b6
   1800a39a7:	ff 15 83 c3 53 00    	call   QWORD PTR [rip+0x53c383]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   1800a39ad:	48 8b c8             	mov    rcx,rax
   1800a39b0:	ff 15 c2 c5 53 00    	call   QWORD PTR [rip+0x53c5c2]        # 0x1805dff78 ; ?prepareForConsuming@NativeTexture@d3d@@QEAAXXZ
   1800a39b6:	48 8b 4f 20          	mov    rcx,QWORD PTR [rdi+0x20]
   1800a39ba:	48 85 c9             	test   rcx,rcx
   1800a39bd:	74 0f                	je     0x1800a39ce
   1800a39bf:	ff 15 6b c3 53 00    	call   QWORD PTR [rip+0x53c36b]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   1800a39c5:	48 8b c8             	mov    rcx,rax
   1800a39c8:	ff 15 aa c5 53 00    	call   QWORD PTR [rip+0x53c5aa]        # 0x1805dff78 ; ?prepareForConsuming@NativeTexture@d3d@@QEAAXXZ
   1800a39ce:	48 8b 4f 28          	mov    rcx,QWORD PTR [rdi+0x28]
   1800a39d2:	48 85 c9             	test   rcx,rcx
   1800a39d5:	74 0f                	je     0x1800a39e6
   1800a39d7:	ff 15 53 c3 53 00    	call   QWORD PTR [rip+0x53c353]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   1800a39dd:	48 8b c8             	mov    rcx,rax
   1800a39e0:	ff 15 92 c5 53 00    	call   QWORD PTR [rip+0x53c592]        # 0x1805dff78 ; ?prepareForConsuming@NativeTexture@d3d@@QEAAXXZ
   1800a39e6:	48 8b 4f 30          	mov    rcx,QWORD PTR [rdi+0x30]
   1800a39ea:	48 85 c9             	test   rcx,rcx
   1800a39ed:	74 0f                	je     0x1800a39fe
   1800a39ef:	ff 15 3b c3 53 00    	call   QWORD PTR [rip+0x53c33b]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   1800a39f5:	48 8b c8             	mov    rcx,rax
   1800a39f8:	ff 15 7a c5 53 00    	call   QWORD PTR [rip+0x53c57a]        # 0x1805dff78 ; ?prepareForConsuming@NativeTexture@d3d@@QEAAXXZ
   1800a39fe:	8b 0d 38 ee 75 00    	mov    ecx,DWORD PTR [rip+0x75ee38]        # 0x18080283c
   1800a3a04:	65 48 8b 04 25 58 00 	mov    rax,QWORD PTR gs:0x58
   1800a3a0b:	00 00 
   1800a3a0d:	ba 40 00 00 00       	mov    edx,0x40
   1800a3a12:	4c 8b 34 c8          	mov    r14,QWORD PTR [rax+rcx*8]
   1800a3a16:	4c 03 f2             	add    r14,rdx
   1800a3a19:	33 db                	xor    ebx,ebx
   1800a3a1b:	48 8d 2d 66 de 1e 01 	lea    rbp,[rip+0x11ede66]        # 0x181291888
   1800a3a22:	41 8b 06             	mov    eax,DWORD PTR [r14]
   1800a3a25:	39 05 45 de 1e 01    	cmp    DWORD PTR [rip+0x11ede45],eax        # 0x181291870
   1800a3a2b:	7e 63                	jle    0x1800a3a90
   1800a3a2d:	48 8d 0d 3c de 1e 01 	lea    rcx,[rip+0x11ede3c]        # 0x181291870
   1800a3a34:	e8 2b 44 4e 00       	call   0x180587e64
   1800a3a39:	83 3d 30 de 1e 01 ff 	cmp    DWORD PTR [rip+0x11ede30],0xffffffff        # 0x181291870
   1800a3a40:	75 4e                	jne    0x1800a3a90
   1800a3a42:	48 89 1d 2f de 1e 01 	mov    QWORD PTR [rip+0x11ede2f],rbx        # 0x181291878
   1800a3a49:	48 8d 15 f0 3d 58 00 	lea    rdx,[rip+0x583df0]        # 0x180627840 ; 'g_tMaterialId'
   1800a3a50:	48 8d 0d 29 de 1e 01 	lea    rcx,[rip+0x11ede29]        # 0x181291880
   1800a3a57:	e8 14 c8 16 00       	call   0x180210270
   1800a3a5c:	c7 05 1e de 1e 01 09 	mov    DWORD PTR [rip+0x11ede1e],0x9        # 0x181291884
   1800a3a63:	00 00 00 
   1800a3a66:	48 89 2d 0b de 1e 01 	mov    QWORD PTR [rip+0x11ede0b],rbp        # 0x181291878
   1800a3a6d:	48 8b cd             	mov    rcx,rbp
   1800a3a70:	ff 15 fa c1 53 00    	call   QWORD PTR [rip+0x53c1fa]        # 0x1805dfc70 ; ??0ShaderTexture@d3d@@QEAA@XZ
   1800a3a76:	90                   	nop
   1800a3a77:	48 8d 0d c2 72 52 00 	lea    rcx,[rip+0x5272c2]        # 0x1805cad40
   1800a3a7e:	e8 41 41 4e 00       	call   0x180587bc4
   1800a3a83:	90                   	nop
   1800a3a84:	48 8d 0d e5 dd 1e 01 	lea    rcx,[rip+0x11edde5]        # 0x181291870
   1800a3a8b:	e8 74 43 4e 00       	call   0x180587e04
   1800a3a90:	48 8b 0f             	mov    rcx,QWORD PTR [rdi]
   1800a3a93:	48 85 c9             	test   rcx,rcx
   1800a3a96:	74 0b                	je     0x1800a3aa3
   1800a3a98:	ff 15 92 c2 53 00    	call   QWORD PTR [rip+0x53c292]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   1800a3a9e:	48 8b f0             	mov    rsi,rax
   1800a3aa1:	eb 03                	jmp    0x1800a3aa6
   1800a3aa3:	48 8b f3             	mov    rsi,rbx
   1800a3aa6:	8b 0d d4 dd 1e 01    	mov    ecx,DWORD PTR [rip+0x11eddd4]        # 0x181291880
   1800a3aac:	ff 15 d6 c1 53 00    	call   QWORD PTR [rip+0x53c1d6]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   1800a3ab2:	8b d0                	mov    edx,eax
   1800a3ab4:	4c 8b c6             	mov    r8,rsi
   1800a3ab7:	48 8d 4c 24 28       	lea    rcx,[rsp+0x28]
   1800a3abc:	ff 15 be c1 53 00    	call   QWORD PTR [rip+0x53c1be]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   1800a3ac2:	45 33 c0             	xor    r8d,r8d
   1800a3ac5:	48 8d 54 24 28       	lea    rdx,[rsp+0x28]
   1800a3aca:	8b 0d b0 dd 1e 01    	mov    ecx,DWORD PTR [rip+0x11eddb0]        # 0x181291880
   1800a3ad0:	ff 15 ba c4 53 00    	call   QWORD PTR [rip+0x53c4ba]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   1800a3ad6:	8b 0d a4 dd 1e 01    	mov    ecx,DWORD PTR [rip+0x11edda4]        # 0x181291880
   1800a3adc:	ff 15 a6 c1 53 00    	call   QWORD PTR [rip+0x53c1a6]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   1800a3ae2:	8b d0                	mov    edx,eax
   1800a3ae4:	4c 8b c6             	mov    r8,rsi
   1800a3ae7:	48 8b cd             	mov    rcx,rbp
   1800a3aea:	ff 15 a0 c1 53 00    	call   QWORD PTR [rip+0x53c1a0]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   1800a3af0:	48 8d 2d b9 dd 1e 01 	lea    rbp,[rip+0x11eddb9]        # 0x1812918b0
   1800a3af7:	41 8b 06             	mov    eax,DWORD PTR [r14]
   1800a3afa:	39 05 98 dd 1e 01    	cmp    DWORD PTR [rip+0x11edd98],eax        # 0x181291898
   1800a3b00:	7e 63                	jle    0x1800a3b65
   1800a3b02:	48 8d 0d 8f dd 1e 01 	lea    rcx,[rip+0x11edd8f]        # 0x181291898
   1800a3b09:	e8 56 43 4e 00       	call   0x180587e64
   1800a3b0e:	83 3d 83 dd 1e 01 ff 	cmp    DWORD PTR [rip+0x11edd83],0xffffffff        # 0x181291898
   1800a3b15:	75 4e                	jne    0x1800a3b65
   1800a3b17:	48 89 1d 82 dd 1e 01 	mov    QWORD PTR [rip+0x11edd82],rbx        # 0x1812918a0
   1800a3b1e:	48 8d 15 2b 3d 58 00 	lea    rdx,[rip+0x583d2b]        # 0x180627850 ; 'g_tNormal_TexcoordX'
   1800a3b25:	48 8d 0d 7c dd 1e 01 	lea    rcx,[rip+0x11edd7c]        # 0x1812918a8
   1800a3b2c:	e8 3f c7 16 00       	call   0x180210270
   1800a3b31:	c7 05 71 dd 1e 01 09 	mov    DWORD PTR [rip+0x11edd71],0x9        # 0x1812918ac
   1800a3b38:	00 00 00 
   1800a3b3b:	48 89 2d 5e dd 1e 01 	mov    QWORD PTR [rip+0x11edd5e],rbp        # 0x1812918a0
   1800a3b42:	48 8b cd             	mov    rcx,rbp
   1800a3b45:	ff 15 25 c1 53 00    	call   QWORD PTR [rip+0x53c125]        # 0x1805dfc70 ; ??0ShaderTexture@d3d@@QEAA@XZ
   1800a3b4b:	90                   	nop
   1800a3b4c:	48 8d 0d bd 71 52 00 	lea    rcx,[rip+0x5271bd]        # 0x1805cad10
   1800a3b53:	e8 6c 40 4e 00       	call   0x180587bc4
   1800a3b58:	90                   	nop
   1800a3b59:	48 8d 0d 38 dd 1e 01 	lea    rcx,[rip+0x11edd38]        # 0x181291898
   1800a3b60:	e8 9f 42 4e 00       	call   0x180587e04
   1800a3b65:	48 8b 4f 08          	mov    rcx,QWORD PTR [rdi+0x8]
   1800a3b69:	48 85 c9             	test   rcx,rcx
   1800a3b6c:	74 0b                	je     0x1800a3b79
   1800a3b6e:	ff 15 bc c1 53 00    	call   QWORD PTR [rip+0x53c1bc]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   1800a3b74:	48 8b f0             	mov    rsi,rax
   1800a3b77:	eb 03                	jmp    0x1800a3b7c
   1800a3b79:	48 8b f3             	mov    rsi,rbx
   1800a3b7c:	8b 0d 26 dd 1e 01    	mov    ecx,DWORD PTR [rip+0x11edd26]        # 0x1812918a8
   1800a3b82:	ff 15 00 c1 53 00    	call   QWORD PTR [rip+0x53c100]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   1800a3b88:	8b d0                	mov    edx,eax
   1800a3b8a:	4c 8b c6             	mov    r8,rsi
   1800a3b8d:	48 8d 4c 24 28       	lea    rcx,[rsp+0x28]
   1800a3b92:	ff 15 e8 c0 53 00    	call   QWORD PTR [rip+0x53c0e8]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   1800a3b98:	45 33 c0             	xor    r8d,r8d
   1800a3b9b:	48 8d 54 24 28       	lea    rdx,[rsp+0x28]
   1800a3ba0:	8b 0d 02 dd 1e 01    	mov    ecx,DWORD PTR [rip+0x11edd02]        # 0x1812918a8
   1800a3ba6:	ff 15 e4 c3 53 00    	call   QWORD PTR [rip+0x53c3e4]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   1800a3bac:	8b 0d f6 dc 1e 01    	mov    ecx,DWORD PTR [rip+0x11edcf6]        # 0x1812918a8
   1800a3bb2:	ff 15 d0 c0 53 00    	call   QWORD PTR [rip+0x53c0d0]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   1800a3bb8:	8b d0                	mov    edx,eax
   1800a3bba:	4c 8b c6             	mov    r8,rsi
   1800a3bbd:	48 8b cd             	mov    rcx,rbp
   1800a3bc0:	ff 15 ca c0 53 00    	call   QWORD PTR [rip+0x53c0ca]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   1800a3bc6:	48 8d 2d 0b dd 1e 01 	lea    rbp,[rip+0x11edd0b]        # 0x1812918d8
   1800a3bcd:	41 8b 06             	mov    eax,DWORD PTR [r14]
   1800a3bd0:	39 05 ea dc 1e 01    	cmp    DWORD PTR [rip+0x11edcea],eax        # 0x1812918c0
   1800a3bd6:	7e 63                	jle    0x1800a3c3b
   1800a3bd8:	48 8d 0d e1 dc 1e 01 	lea    rcx,[rip+0x11edce1]        # 0x1812918c0
   1800a3bdf:	e8 80 42 4e 00       	call   0x180587e64
   1800a3be4:	83 3d d5 dc 1e 01 ff 	cmp    DWORD PTR [rip+0x11edcd5],0xffffffff        # 0x1812918c0
   1800a3beb:	75 4e                	jne    0x1800a3c3b
   1800a3bed:	48 89 1d d4 dc 1e 01 	mov    QWORD PTR [rip+0x11edcd4],rbx        # 0x1812918c8
   1800a3bf4:	48 8d 15 6d 3c 58 00 	lea    rdx,[rip+0x583c6d]        # 0x180627868 ; 'g_tPosition_TexcoordY'
   1800a3bfb:	48 8d 0d ce dc 1e 01 	lea    rcx,[rip+0x11edcce]        # 0x1812918d0
   1800a3c02:	e8 69 c6 16 00       	call   0x180210270
   1800a3c07:	c7 05 c3 dc 1e 01 09 	mov    DWORD PTR [rip+0x11edcc3],0x9        # 0x1812918d4
   1800a3c0e:	00 00 00 
   1800a3c11:	48 89 2d b0 dc 1e 01 	mov    QWORD PTR [rip+0x11edcb0],rbp        # 0x1812918c8
   1800a3c18:	48 8b cd             	mov    rcx,rbp
   1800a3c1b:	ff 15 4f c0 53 00    	call   QWORD PTR [rip+0x53c04f]        # 0x1805dfc70 ; ??0ShaderTexture@d3d@@QEAA@XZ
   1800a3c21:	90                   	nop
   1800a3c22:	48 8d 0d b7 70 52 00 	lea    rcx,[rip+0x5270b7]        # 0x1805cace0
   1800a3c29:	e8 96 3f 4e 00       	call   0x180587bc4
   1800a3c2e:	90                   	nop
   1800a3c2f:	48 8d 0d 8a dc 1e 01 	lea    rcx,[rip+0x11edc8a]        # 0x1812918c0
   1800a3c36:	e8 c9 41 4e 00       	call   0x180587e04
   1800a3c3b:	48 8b 4f 10          	mov    rcx,QWORD PTR [rdi+0x10]
   1800a3c3f:	48 85 c9             	test   rcx,rcx
   1800a3c42:	74 0b                	je     0x1800a3c4f
   1800a3c44:	ff 15 e6 c0 53 00    	call   QWORD PTR [rip+0x53c0e6]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   1800a3c4a:	48 8b f0             	mov    rsi,rax
   1800a3c4d:	eb 03                	jmp    0x1800a3c52
   1800a3c4f:	48 8b f3             	mov    rsi,rbx
   1800a3c52:	8b 0d 78 dc 1e 01    	mov    ecx,DWORD PTR [rip+0x11edc78]        # 0x1812918d0
   1800a3c58:	ff 15 2a c0 53 00    	call   QWORD PTR [rip+0x53c02a]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   1800a3c5e:	8b d0                	mov    edx,eax
   1800a3c60:	4c 8b c6             	mov    r8,rsi
   1800a3c63:	48 8d 4c 24 28       	lea    rcx,[rsp+0x28]
   1800a3c68:	ff 15 12 c0 53 00    	call   QWORD PTR [rip+0x53c012]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   1800a3c6e:	45 33 c0             	xor    r8d,r8d
   1800a3c71:	48 8d 54 24 28       	lea    rdx,[rsp+0x28]
   1800a3c76:	8b 0d 54 dc 1e 01    	mov    ecx,DWORD PTR [rip+0x11edc54]        # 0x1812918d0
   1800a3c7c:	ff 15 0e c3 53 00    	call   QWORD PTR [rip+0x53c30e]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   1800a3c82:	8b 0d 48 dc 1e 01    	mov    ecx,DWORD PTR [rip+0x11edc48]        # 0x1812918d0
   1800a3c88:	ff 15 fa bf 53 00    	call   QWORD PTR [rip+0x53bffa]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   1800a3c8e:	8b d0                	mov    edx,eax
   1800a3c90:	4c 8b c6             	mov    r8,rsi
   1800a3c93:	48 8b cd             	mov    rcx,rbp
   1800a3c96:	ff 15 f4 bf 53 00    	call   QWORD PTR [rip+0x53bff4]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   1800a3c9c:	48 8d 2d 5d dc 1e 01 	lea    rbp,[rip+0x11edc5d]        # 0x181291900
   1800a3ca3:	41 8b 06             	mov    eax,DWORD PTR [r14]
   1800a3ca6:	39 05 3c dc 1e 01    	cmp    DWORD PTR [rip+0x11edc3c],eax        # 0x1812918e8
   1800a3cac:	7e 63                	jle    0x1800a3d11
   1800a3cae:	48 8d 0d 33 dc 1e 01 	lea    rcx,[rip+0x11edc33]        # 0x1812918e8
   1800a3cb5:	e8 aa 41 4e 00       	call   0x180587e64
   1800a3cba:	83 3d 27 dc 1e 01 ff 	cmp    DWORD PTR [rip+0x11edc27],0xffffffff        # 0x1812918e8
   1800a3cc1:	75 4e                	jne    0x1800a3d11
   1800a3cc3:	48 89 1d 26 dc 1e 01 	mov    QWORD PTR [rip+0x11edc26],rbx        # 0x1812918f0
   1800a3cca:	48 8d 15 af 3b 58 00 	lea    rdx,[rip+0x583baf]        # 0x180627880 ; 'g_tAlbedoColor'
   1800a3cd1:	48 8d 0d 20 dc 1e 01 	lea    rcx,[rip+0x11edc20]        # 0x1812918f8
   1800a3cd8:	e8 93 c5 16 00       	call   0x180210270
   1800a3cdd:	c7 05 15 dc 1e 01 09 	mov    DWORD PTR [rip+0x11edc15],0x9        # 0x1812918fc
   1800a3ce4:	00 00 00 
   1800a3ce7:	48 89 2d 02 dc 1e 01 	mov    QWORD PTR [rip+0x11edc02],rbp        # 0x1812918f0
   1800a3cee:	48 8b cd             	mov    rcx,rbp
   1800a3cf1:	ff 15 79 bf 53 00    	call   QWORD PTR [rip+0x53bf79]        # 0x1805dfc70 ; ??0ShaderTexture@d3d@@QEAA@XZ
   1800a3cf7:	90                   	nop
   1800a3cf8:	48 8d 0d b1 6f 52 00 	lea    rcx,[rip+0x526fb1]        # 0x1805cacb0
   1800a3cff:	e8 c0 3e 4e 00       	call   0x180587bc4
   1800a3d04:	90                   	nop
   1800a3d05:	48 8d 0d dc db 1e 01 	lea    rcx,[rip+0x11edbdc]        # 0x1812918e8
   1800a3d0c:	e8 f3 40 4e 00       	call   0x180587e04
   1800a3d11:	48 8b 4f 18          	mov    rcx,QWORD PTR [rdi+0x18]
   1800a3d15:	48 85 c9             	test   rcx,rcx
   1800a3d18:	74 0b                	je     0x1800a3d25
   1800a3d1a:	ff 15 10 c0 53 00    	call   QWORD PTR [rip+0x53c010]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   1800a3d20:	48 8b f0             	mov    rsi,rax
   1800a3d23:	eb 03                	jmp    0x1800a3d28
   1800a3d25:	48 8b f3             	mov    rsi,rbx
   1800a3d28:	8b 0d ca db 1e 01    	mov    ecx,DWORD PTR [rip+0x11edbca]        # 0x1812918f8
   1800a3d2e:	ff 15 54 bf 53 00    	call   QWORD PTR [rip+0x53bf54]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   1800a3d34:	8b d0                	mov    edx,eax
   1800a3d36:	4c 8b c6             	mov    r8,rsi
   1800a3d39:	48 8d 4c 24 28       	lea    rcx,[rsp+0x28]
   1800a3d3e:	ff 15 3c bf 53 00    	call   QWORD PTR [rip+0x53bf3c]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   1800a3d44:	45 33 c0             	xor    r8d,r8d
   1800a3d47:	48 8d 54 24 28       	lea    rdx,[rsp+0x28]
   1800a3d4c:	8b 0d a6 db 1e 01    	mov    ecx,DWORD PTR [rip+0x11edba6]        # 0x1812918f8
   1800a3d52:	ff 15 38 c2 53 00    	call   QWORD PTR [rip+0x53c238]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   1800a3d58:	8b 0d 9a db 1e 01    	mov    ecx,DWORD PTR [rip+0x11edb9a]        # 0x1812918f8
   1800a3d5e:	ff 15 24 bf 53 00    	call   QWORD PTR [rip+0x53bf24]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   1800a3d64:	8b d0                	mov    edx,eax
   1800a3d66:	4c 8b c6             	mov    r8,rsi
   1800a3d69:	48 8b cd             	mov    rcx,rbp
   1800a3d6c:	ff 15 1e bf 53 00    	call   QWORD PTR [rip+0x53bf1e]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   1800a3d72:	48 8d 2d af db 1e 01 	lea    rbp,[rip+0x11edbaf]        # 0x181291928
   1800a3d79:	41 8b 06             	mov    eax,DWORD PTR [r14]
   1800a3d7c:	39 05 8e db 1e 01    	cmp    DWORD PTR [rip+0x11edb8e],eax        # 0x181291910
   1800a3d82:	7e 63                	jle    0x1800a3de7
   1800a3d84:	48 8d 0d 85 db 1e 01 	lea    rcx,[rip+0x11edb85]        # 0x181291910
   1800a3d8b:	e8 d4 40 4e 00       	call   0x180587e64
   1800a3d90:	83 3d 79 db 1e 01 ff 	cmp    DWORD PTR [rip+0x11edb79],0xffffffff        # 0x181291910
   1800a3d97:	75 4e                	jne    0x1800a3de7
   1800a3d99:	48 89 1d 78 db 1e 01 	mov    QWORD PTR [rip+0x11edb78],rbx        # 0x181291918
   1800a3da0:	48 8d 15 e9 3a 58 00 	lea    rdx,[rip+0x583ae9]        # 0x180627890 ; 'g_tEmissionColor'
   1800a3da7:	48 8d 0d 72 db 1e 01 	lea    rcx,[rip+0x11edb72]        # 0x181291920
   1800a3dae:	e8 bd c4 16 00       	call   0x180210270
   1800a3db3:	c7 05 67 db 1e 01 09 	mov    DWORD PTR [rip+0x11edb67],0x9        # 0x181291924
   1800a3dba:	00 00 00 
   1800a3dbd:	48 89 2d 54 db 1e 01 	mov    QWORD PTR [rip+0x11edb54],rbp        # 0x181291918
   1800a3dc4:	48 8b cd             	mov    rcx,rbp
   1800a3dc7:	ff 15 a3 be 53 00    	call   QWORD PTR [rip+0x53bea3]        # 0x1805dfc70 ; ??0ShaderTexture@d3d@@QEAA@XZ
   1800a3dcd:	90                   	nop
   1800a3dce:	48 8d 0d ab 6e 52 00 	lea    rcx,[rip+0x526eab]        # 0x1805cac80
   1800a3dd5:	e8 ea 3d 4e 00       	call   0x180587bc4
   1800a3dda:	90                   	nop
   1800a3ddb:	48 8d 0d 2e db 1e 01 	lea    rcx,[rip+0x11edb2e]        # 0x181291910
   1800a3de2:	e8 1d 40 4e 00       	call   0x180587e04
   1800a3de7:	48 8b 4f 20          	mov    rcx,QWORD PTR [rdi+0x20]
   1800a3deb:	48 85 c9             	test   rcx,rcx
   1800a3dee:	74 0b                	je     0x1800a3dfb
   1800a3df0:	ff 15 3a bf 53 00    	call   QWORD PTR [rip+0x53bf3a]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   1800a3df6:	48 8b f0             	mov    rsi,rax
   1800a3df9:	eb 03                	jmp    0x1800a3dfe
   1800a3dfb:	48 8b f3             	mov    rsi,rbx
   1800a3dfe:	8b 0d 1c db 1e 01    	mov    ecx,DWORD PTR [rip+0x11edb1c]        # 0x181291920
   1800a3e04:	ff 15 7e be 53 00    	call   QWORD PTR [rip+0x53be7e]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   1800a3e0a:	8b d0                	mov    edx,eax
   1800a3e0c:	4c 8b c6             	mov    r8,rsi
   1800a3e0f:	48 8d 4c 24 28       	lea    rcx,[rsp+0x28]
   1800a3e14:	ff 15 66 be 53 00    	call   QWORD PTR [rip+0x53be66]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   1800a3e1a:	45 33 c0             	xor    r8d,r8d
   1800a3e1d:	48 8d 54 24 28       	lea    rdx,[rsp+0x28]
   1800a3e22:	8b 0d f8 da 1e 01    	mov    ecx,DWORD PTR [rip+0x11edaf8]        # 0x181291920
   1800a3e28:	ff 15 62 c1 53 00    	call   QWORD PTR [rip+0x53c162]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   1800a3e2e:	8b 0d ec da 1e 01    	mov    ecx,DWORD PTR [rip+0x11edaec]        # 0x181291920
   1800a3e34:	ff 15 4e be 53 00    	call   QWORD PTR [rip+0x53be4e]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   1800a3e3a:	8b d0                	mov    edx,eax
   1800a3e3c:	4c 8b c6             	mov    r8,rsi
   1800a3e3f:	48 8b cd             	mov    rcx,rbp
   1800a3e42:	ff 15 48 be 53 00    	call   QWORD PTR [rip+0x53be48]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   1800a3e48:	48 8d 2d 01 db 1e 01 	lea    rbp,[rip+0x11edb01]        # 0x181291950
   1800a3e4f:	41 8b 06             	mov    eax,DWORD PTR [r14]
   1800a3e52:	39 05 e0 da 1e 01    	cmp    DWORD PTR [rip+0x11edae0],eax        # 0x181291938
   1800a3e58:	7e 63                	jle    0x1800a3ebd
   1800a3e5a:	48 8d 0d d7 da 1e 01 	lea    rcx,[rip+0x11edad7]        # 0x181291938
   1800a3e61:	e8 fe 3f 4e 00       	call   0x180587e64
   1800a3e66:	83 3d cb da 1e 01 ff 	cmp    DWORD PTR [rip+0x11edacb],0xffffffff        # 0x181291938
   1800a3e6d:	75 4e                	jne    0x1800a3ebd
   1800a3e6f:	48 89 1d ca da 1e 01 	mov    QWORD PTR [rip+0x11edaca],rbx        # 0x181291940
   1800a3e76:	48 8d 15 2b 3a 58 00 	lea    rdx,[rip+0x583a2b]        # 0x1806278a8 ; 'g_tSmoothnessSpecularAlbedo'
   1800a3e7d:	48 8d 0d c4 da 1e 01 	lea    rcx,[rip+0x11edac4]        # 0x181291948
   1800a3e84:	e8 e7 c3 16 00       	call   0x180210270
   1800a3e89:	c7 05 b9 da 1e 01 09 	mov    DWORD PTR [rip+0x11edab9],0x9        # 0x18129194c
   1800a3e90:	00 00 00 
   1800a3e93:	48 89 2d a6 da 1e 01 	mov    QWORD PTR [rip+0x11edaa6],rbp        # 0x181291940
   1800a3e9a:	48 8b cd             	mov    rcx,rbp
   1800a3e9d:	ff 15 cd bd 53 00    	call   QWORD PTR [rip+0x53bdcd]        # 0x1805dfc70 ; ??0ShaderTexture@d3d@@QEAA@XZ
   1800a3ea3:	90                   	nop
   1800a3ea4:	48 8d 0d a5 6d 52 00 	lea    rcx,[rip+0x526da5]        # 0x1805cac50
   1800a3eab:	e8 14 3d 4e 00       	call   0x180587bc4
   1800a3eb0:	90                   	nop
   1800a3eb1:	48 8d 0d 80 da 1e 01 	lea    rcx,[rip+0x11eda80]        # 0x181291938
   1800a3eb8:	e8 47 3f 4e 00       	call   0x180587e04
   1800a3ebd:	48 8b 4f 28          	mov    rcx,QWORD PTR [rdi+0x28]
   1800a3ec1:	48 85 c9             	test   rcx,rcx
   1800a3ec4:	74 0b                	je     0x1800a3ed1
   1800a3ec6:	ff 15 64 be 53 00    	call   QWORD PTR [rip+0x53be64]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   1800a3ecc:	48 8b f0             	mov    rsi,rax
   1800a3ecf:	eb 03                	jmp    0x1800a3ed4
   1800a3ed1:	48 8b f3             	mov    rsi,rbx
   1800a3ed4:	8b 0d 6e da 1e 01    	mov    ecx,DWORD PTR [rip+0x11eda6e]        # 0x181291948
   1800a3eda:	ff 15 a8 bd 53 00    	call   QWORD PTR [rip+0x53bda8]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   1800a3ee0:	8b d0                	mov    edx,eax
   1800a3ee2:	4c 8b c6             	mov    r8,rsi
   1800a3ee5:	48 8d 4c 24 28       	lea    rcx,[rsp+0x28]
   1800a3eea:	ff 15 90 bd 53 00    	call   QWORD PTR [rip+0x53bd90]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   1800a3ef0:	45 33 c0             	xor    r8d,r8d
   1800a3ef3:	48 8d 54 24 28       	lea    rdx,[rsp+0x28]
   1800a3ef8:	8b 0d 4a da 1e 01    	mov    ecx,DWORD PTR [rip+0x11eda4a]        # 0x181291948
   1800a3efe:	ff 15 8c c0 53 00    	call   QWORD PTR [rip+0x53c08c]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   1800a3f04:	8b 0d 3e da 1e 01    	mov    ecx,DWORD PTR [rip+0x11eda3e]        # 0x181291948
   1800a3f0a:	ff 15 78 bd 53 00    	call   QWORD PTR [rip+0x53bd78]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   1800a3f10:	8b d0                	mov    edx,eax
   1800a3f12:	4c 8b c6             	mov    r8,rsi
   1800a3f15:	48 8b cd             	mov    rcx,rbp
   1800a3f18:	ff 15 72 bd 53 00    	call   QWORD PTR [rip+0x53bd72]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   1800a3f1e:	48 8d 35 53 da 1e 01 	lea    rsi,[rip+0x11eda53]        # 0x181291978
   1800a3f25:	41 8b 06             	mov    eax,DWORD PTR [r14]
   1800a3f28:	39 05 32 da 1e 01    	cmp    DWORD PTR [rip+0x11eda32],eax        # 0x181291960
   1800a3f2e:	7e 63                	jle    0x1800a3f93
   1800a3f30:	48 8d 0d 29 da 1e 01 	lea    rcx,[rip+0x11eda29]        # 0x181291960
   1800a3f37:	e8 28 3f 4e 00       	call   0x180587e64
   1800a3f3c:	83 3d 1d da 1e 01 ff 	cmp    DWORD PTR [rip+0x11eda1d],0xffffffff        # 0x181291960
   1800a3f43:	75 4e                	jne    0x1800a3f93
   1800a3f45:	48 89 1d 1c da 1e 01 	mov    QWORD PTR [rip+0x11eda1c],rbx        # 0x181291968
   1800a3f4c:	48 8d 15 75 39 58 00 	lea    rdx,[rip+0x583975]        # 0x1806278c8 ; 'g_tShadow'
   1800a3f53:	48 8d 0d 16 da 1e 01 	lea    rcx,[rip+0x11eda16]        # 0x181291970
   1800a3f5a:	e8 11 c3 16 00       	call   0x180210270
   1800a3f5f:	c7 05 0b da 1e 01 09 	mov    DWORD PTR [rip+0x11eda0b],0x9        # 0x181291974
   1800a3f66:	00 00 00 
   1800a3f69:	48 89 35 f8 d9 1e 01 	mov    QWORD PTR [rip+0x11ed9f8],rsi        # 0x181291968
   1800a3f70:	48 8b ce             	mov    rcx,rsi
   1800a3f73:	ff 15 f7 bc 53 00    	call   QWORD PTR [rip+0x53bcf7]        # 0x1805dfc70 ; ??0ShaderTexture@d3d@@QEAA@XZ
   1800a3f79:	90                   	nop
   1800a3f7a:	48 8d 0d 9f 6c 52 00 	lea    rcx,[rip+0x526c9f]        # 0x1805cac20
   1800a3f81:	e8 3e 3c 4e 00       	call   0x180587bc4
   1800a3f86:	90                   	nop
   1800a3f87:	48 8d 0d d2 d9 1e 01 	lea    rcx,[rip+0x11ed9d2]        # 0x181291960
   1800a3f8e:	e8 71 3e 4e 00       	call   0x180587e04
   1800a3f93:	48 8b 4f 30          	mov    rcx,QWORD PTR [rdi+0x30]
   1800a3f97:	48 85 c9             	test   rcx,rcx
   1800a3f9a:	74 09                	je     0x1800a3fa5
   1800a3f9c:	ff 15 8e bd 53 00    	call   QWORD PTR [rip+0x53bd8e]        # 0x1805dfd30 ; ?getNativeTexture@NativeTextureContainer@d3d@@QEBAPEAVNativeTexture@2@XZ
   1800a3fa2:	48 8b d8             	mov    rbx,rax
   1800a3fa5:	8b 0d c5 d9 1e 01    	mov    ecx,DWORD PTR [rip+0x11ed9c5]        # 0x181291970
   1800a3fab:	ff 15 d7 bc 53 00    	call   QWORD PTR [rip+0x53bcd7]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   1800a3fb1:	8b d0                	mov    edx,eax
   1800a3fb3:	4c 8b c3             	mov    r8,rbx
   1800a3fb6:	48 8d 4c 24 28       	lea    rcx,[rsp+0x28]
   1800a3fbb:	ff 15 bf bc 53 00    	call   QWORD PTR [rip+0x53bcbf]        # 0x1805dfc80 ; ??0ShaderTexture@d3d@@QEAA@W4ResourceType@1@PEAVNativeTexture@1@@Z
   1800a3fc1:	45 33 c0             	xor    r8d,r8d
   1800a3fc4:	48 8d 54 24 28       	lea    rdx,[rsp+0x28]
   1800a3fc9:	8b 0d a1 d9 1e 01    	mov    ecx,DWORD PTR [rip+0x11ed9a1]        # 0x181291970
   1800a3fcf:	ff 15 bb bf 53 00    	call   QWORD PTR [rip+0x53bfbb]        # 0x1805dff90 ; ?setProviderData@ConstantValueProviderRegistry@d3d@@SAXHPEBXPEBD@Z
   1800a3fd5:	8b 0d 95 d9 1e 01    	mov    ecx,DWORD PTR [rip+0x11ed995]        # 0x181291970
   1800a3fdb:	ff 15 a7 bc 53 00    	call   QWORD PTR [rip+0x53bca7]        # 0x1805dfc88 ; ?getResourceTypeForValue@ConstantValueProviderRegistry@d3d@@SA?AW4ResourceType@2@H@Z
   1800a3fe1:	8b d0                	mov    edx,eax
   1800a3fe3:	4c 8b c3             	mov    r8,rbx
   1800a3fe6:	48 8b ce             	mov    rcx,rsi
   1800a3fe9:	ff 15 a1 bc 53 00    	call   QWORD PTR [rip+0x53bca1]        # 0x1805dfc90 ; ?setNativeTexture@ShaderTexture@d3d@@QEAAXW4ResourceType@2@PEAVNativeTexture@2@@Z
   1800a3fef:	48 8b 5c 24 50       	mov    rbx,QWORD PTR [rsp+0x50]
   1800a3ff4:	48 8b 6c 24 58       	mov    rbp,QWORD PTR [rsp+0x58]
   1800a3ff9:	48 8b 74 24 60       	mov    rsi,QWORD PTR [rsp+0x60]
   1800a3ffe:	48 8b 7c 24 68       	mov    rdi,QWORD PTR [rsp+0x68]
   1800a4003:	48 83 c4 40          	add    rsp,0x40
   1800a4007:	41 5e                	pop    r14
   1800a4009:	c3                   	ret
