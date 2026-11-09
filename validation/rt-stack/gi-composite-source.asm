
/mnt/data/renderer_rmdwin10_f(5).dll:     file format pei-x86-64


Disassembly of section .text:

000000018012dbe3 <.text+0x12cbe3>:
   18012dbe3:	48 8b 0d 0e 0e 7e 00 	mov    rcx,QWORD PTR [rip+0x7e0e0e]        # 0x18090e9f8
   18012dbea:	48 85 c9             	test   rcx,rcx
   18012dbed:	74 0b                	je     0x18012dbfa
   18012dbef:	ff 15 3b 21 4b 00    	call   QWORD PTR [rip+0x4b213b]        # 0x1805dfd30
   18012dbf5:	48 8b d8             	mov    rbx,rax
   18012dbf8:	eb 03                	jmp    0x18012dbfd
   18012dbfa:	48 8b de             	mov    rbx,rsi
   18012dbfd:	8b 0d c5 99 16 01    	mov    ecx,DWORD PTR [rip+0x11699c5]        # 0x1812975c8
   18012dc03:	ff 15 7f 20 4b 00    	call   QWORD PTR [rip+0x4b207f]        # 0x1805dfc88
   18012dc09:	8b d0                	mov    edx,eax
   18012dc0b:	4c 8b c3             	mov    r8,rbx
   18012dc0e:	48 8d 4c 24 60       	lea    rcx,[rsp+0x60]
   18012dc13:	ff 15 67 20 4b 00    	call   QWORD PTR [rip+0x4b2067]        # 0x1805dfc80
   18012dc19:	45 33 c0             	xor    r8d,r8d
   18012dc1c:	48 8d 54 24 60       	lea    rdx,[rsp+0x60]
   18012dc21:	8b 0d a1 99 16 01    	mov    ecx,DWORD PTR [rip+0x11699a1]        # 0x1812975c8
   18012dc27:	ff 15 63 23 4b 00    	call   QWORD PTR [rip+0x4b2363]        # 0x1805dff90
   18012dc2d:	8b 0d 95 99 16 01    	mov    ecx,DWORD PTR [rip+0x1169995]        # 0x1812975c8
   18012dc33:	ff 15 4f 20 4b 00    	call   QWORD PTR [rip+0x4b204f]        # 0x1805dfc88
   18012dc39:	8b d0                	mov    edx,eax
   18012dc3b:	4c 8b c3             	mov    r8,rbx
   18012dc3e:	49 8b ce             	mov    rcx,r14
   18012dc41:	ff 15 49 20 4b 00    	call   QWORD PTR [rip+0x4b2049]        # 0x1805dfc90
