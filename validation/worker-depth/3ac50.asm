
upload/d3d_rmdwin10_f(4).dll:     file format pei-x86-64


Disassembly of section .text:

000000018003ac50 <.text+0x39c50>:
   18003ac50:	41 56                	push   r14
   18003ac52:	48 83 ec 50          	sub    rsp,0x50
   18003ac56:	48 c7 44 24 20 fe ff 	mov    QWORD PTR [rsp+0x20],0xfffffffffffffffe
   18003ac5d:	ff ff 
   18003ac5f:	48 89 5c 24 68       	mov    QWORD PTR [rsp+0x68],rbx
   18003ac64:	48 89 74 24 70       	mov    QWORD PTR [rsp+0x70],rsi
   18003ac69:	48 89 7c 24 78       	mov    QWORD PTR [rsp+0x78],rdi
   18003ac6e:	48 8b d1             	mov    rdx,rcx
   18003ac71:	80 79 68 00          	cmp    BYTE PTR [rcx+0x68],0x0
   18003ac75:	0f 85 fc 00 00 00    	jne    0x18003ad77
   18003ac7b:	48 8b 79 50          	mov    rdi,QWORD PTR [rcx+0x50]
   18003ac7f:	48 85 ff             	test   rdi,rdi
   18003ac82:	75 04                	jne    0x18003ac88
   18003ac84:	48 8d 79 40          	lea    rdi,[rcx+0x40]
   18003ac88:	33 c0                	xor    eax,eax
   18003ac8a:	48 89 44 24 28       	mov    QWORD PTR [rsp+0x28],rax
   18003ac8f:	48 89 44 24 40       	mov    QWORD PTR [rsp+0x40],rax
   18003ac94:	48 8b 81 88 00 00 00 	mov    rax,QWORD PTR [rcx+0x88]
   18003ac9b:	48 89 44 24 30       	mov    QWORD PTR [rsp+0x30],rax
   18003aca0:	c7 44 24 38 ff ff ff 	mov    DWORD PTR [rsp+0x38],0xffffffff
   18003aca7:	ff 
   18003aca8:	8b 4f 20             	mov    ecx,DWORD PTR [rdi+0x20]
   18003acab:	89 4c 24 3c          	mov    DWORD PTR [rsp+0x3c],ecx
   18003acaf:	f6 42 1c 02          	test   BYTE PTR [rdx+0x1c],0x2
   18003acb3:	b8 04 00 00 00       	mov    eax,0x4
   18003acb8:	ba 10 00 00 00       	mov    edx,0x10
   18003acbd:	0f 45 c2             	cmovne eax,edx
   18003acc0:	89 44 24 40          	mov    DWORD PTR [rsp+0x40],eax
   18003acc4:	3b c1                	cmp    eax,ecx
   18003acc6:	0f 84 ab 00 00 00    	je     0x18003ad77
   18003accc:	8b 0d 2a 69 0d 00    	mov    ecx,DWORD PTR [rip+0xd692a]        # 0x1801115fc
   18003acd2:	65 48 8b 04 25 58 00 	mov    rax,QWORD PTR gs:0x58
   18003acd9:	00 00 
   18003acdb:	48 8b 1c c8          	mov    rbx,QWORD PTR [rax+rcx*8]
   18003acdf:	41 be 08 00 00 00    	mov    r14d,0x8
   18003ace5:	4c 03 f3             	add    r14,rbx
   18003ace8:	49 8b 06             	mov    rax,QWORD PTR [r14]
   18003aceb:	48 85 c0             	test   rax,rax
   18003acee:	74 0b                	je     0x18003acfb
   18003acf0:	b9 10 00 00 00       	mov    ecx,0x10
   18003acf5:	48 8b 1c 19          	mov    rbx,QWORD PTR [rcx+rbx*1]
   18003acf9:	eb 07                	jmp    0x18003ad02
   18003acfb:	48 8b 1d 1e 6f 0d 00 	mov    rbx,QWORD PTR [rip+0xd6f1e]        # 0x180111c20
   18003ad02:	48 89 5c 24 60       	mov    QWORD PTR [rsp+0x60],rbx
   18003ad07:	48 85 db             	test   rbx,rbx
   18003ad0a:	74 1f                	je     0x18003ad2b
   18003ad0c:	ff 15 9e 24 02 00    	call   QWORD PTR [rip+0x2249e]        # 0x18005d1b0
   18003ad12:	8b f0                	mov    esi,eax
   18003ad14:	39 43 08             	cmp    DWORD PTR [rbx+0x8],eax
   18003ad17:	74 0c                	je     0x18003ad25
   18003ad19:	48 8b cb             	mov    rcx,rbx
   18003ad1c:	ff 15 96 24 02 00    	call   QWORD PTR [rip+0x22496]        # 0x18005d1b8
   18003ad22:	89 73 08             	mov    DWORD PTR [rbx+0x8],esi
   18003ad25:	ff 43 0c             	inc    DWORD PTR [rbx+0xc]
   18003ad28:	49 8b 06             	mov    rax,QWORD PTR [r14]
   18003ad2b:	48 8b 0d e6 6e 0d 00 	mov    rcx,QWORD PTR [rip+0xd6ee6]        # 0x180111c18
   18003ad32:	48 85 c0             	test   rax,rax
   18003ad35:	48 0f 45 c8          	cmovne rcx,rax
   18003ad39:	48 ff 41 08          	inc    QWORD PTR [rcx+0x8]
   18003ad3d:	48 8b 49 18          	mov    rcx,QWORD PTR [rcx+0x18]
   18003ad41:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   18003ad44:	4c 8d 44 24 28       	lea    r8,[rsp+0x28]
   18003ad49:	ba 01 00 00 00       	mov    edx,0x1
   18003ad4e:	ff 90 d0 00 00 00    	call   QWORD PTR [rax+0xd0]
   18003ad54:	90                   	nop
   18003ad55:	48 85 db             	test   rbx,rbx
   18003ad58:	74 16                	je     0x18003ad70
   18003ad5a:	83 43 0c ff          	add    DWORD PTR [rbx+0xc],0xffffffff
   18003ad5e:	75 10                	jne    0x18003ad70
   18003ad60:	c7 43 08 ff ff ff ff 	mov    DWORD PTR [rbx+0x8],0xffffffff
   18003ad67:	48 8b cb             	mov    rcx,rbx
   18003ad6a:	ff 15 38 24 02 00    	call   QWORD PTR [rip+0x22438]        # 0x18005d1a8
   18003ad70:	8b 44 24 40          	mov    eax,DWORD PTR [rsp+0x40]
   18003ad74:	89 47 20             	mov    DWORD PTR [rdi+0x20],eax
   18003ad77:	48 8b 5c 24 68       	mov    rbx,QWORD PTR [rsp+0x68]
   18003ad7c:	48 8b 74 24 70       	mov    rsi,QWORD PTR [rsp+0x70]
   18003ad81:	48 8b 7c 24 78       	mov    rdi,QWORD PTR [rsp+0x78]
   18003ad86:	48 83 c4 50          	add    rsp,0x50
   18003ad8a:	41 5e                	pop    r14
   18003ad8c:	c3                   	ret
