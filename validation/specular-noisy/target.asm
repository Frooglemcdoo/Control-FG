
/mnt/data/renderer_rmdwin10_f(5).dll:     file format pei-x86-64


Disassembly of section .text:

00000001800164bc <.text+0x154bc>:
   1800164bc:	48 85 db             	test   rbx,rbx
   1800164bf:	74 0e                	je     0x1800164cf
   1800164c1:	48 8b cb             	mov    rcx,rbx
   1800164c4:	ff 15 66 98 5c 00    	call   QWORD PTR [rip+0x5c9866]        # 0x1805dfd30
   1800164ca:	48 8b f8             	mov    rdi,rax
   1800164cd:	eb 02                	jmp    0x1800164d1
   1800164cf:	33 ff                	xor    edi,edi
   1800164d1:	8b 0d 39 90 27 01    	mov    ecx,DWORD PTR [rip+0x1279039]        # 0x18128f510
   1800164d7:	ff 15 ab 97 5c 00    	call   QWORD PTR [rip+0x5c97ab]        # 0x1805dfc88
   1800164dd:	8b d0                	mov    edx,eax
   1800164df:	4c 8b c7             	mov    r8,rdi
   1800164e2:	48 8d 4d 80          	lea    rcx,[rbp-0x80]
   1800164e6:	ff 15 94 97 5c 00    	call   QWORD PTR [rip+0x5c9794]        # 0x1805dfc80
   1800164ec:	45 33 c0             	xor    r8d,r8d
   1800164ef:	48 8d 55 80          	lea    rdx,[rbp-0x80]
   1800164f3:	8b 0d 17 90 27 01    	mov    ecx,DWORD PTR [rip+0x1279017]        # 0x18128f510
   1800164f9:	ff 15 91 9a 5c 00    	call   QWORD PTR [rip+0x5c9a91]        # 0x1805dff90
   1800164ff:	8b 0d 0b 90 27 01    	mov    ecx,DWORD PTR [rip+0x127900b]        # 0x18128f510
   180016505:	ff 15 7d 97 5c 00    	call   QWORD PTR [rip+0x5c977d]        # 0x1805dfc88
   18001650b:	8b d0                	mov    edx,eax
   18001650d:	4c 8b c7             	mov    r8,rdi
   180016510:	48 8d 0d 01 90 27 01 	lea    rcx,[rip+0x1279001]        # 0x18128f518
   180016517:	ff 15 73 97 5c 00    	call   QWORD PTR [rip+0x5c9773]        # 0x1805dfc90
   18001651d:	48 8d 3d 1c 90 27 01 	lea    rdi,[rip+0x127901c]        # 0x18128f540
   180016524:	b8                   	.byte 0xb8
   180016525:	40                   	rex
