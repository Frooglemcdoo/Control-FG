
upload/renderer_rmdwin10_f(4).dll:     file format pei-x86-64


Disassembly of section .text:

00000001801dd3f0 <.text+0x1dc3f0>:
   1801dd3f0:	44 8b 51 04          	mov    r10d,DWORD PTR [rcx+0x4]
   1801dd3f4:	45 33 c0             	xor    r8d,r8d
   1801dd3f7:	8b 41 08             	mov    eax,DWORD PTR [rcx+0x8]
   1801dd3fa:	44 0b d2             	or     r10d,edx
   1801dd3fd:	8b 91 40 02 00 00    	mov    edx,DWORD PTR [rcx+0x240]
   1801dd403:	f7 d0                	not    eax
   1801dd405:	44 23 d0             	and    r10d,eax
   1801dd408:	83 ea 01             	sub    edx,0x1
   1801dd40b:	78 30                	js     0x1801dd43d
   1801dd40d:	4c 8b 99 38 02 00 00 	mov    r11,QWORD PTR [rcx+0x238]
   1801dd414:	42 8d 04 02          	lea    eax,[rdx+r8*1]
   1801dd418:	d1 f8                	sar    eax,1
   1801dd41a:	48 63 c8             	movsxd rcx,eax
   1801dd41d:	4c 69 c9 a8 00 00 00 	imul   r9,rcx,0xa8
   1801dd424:	4d 03 cb             	add    r9,r11
   1801dd427:	45 3b 51 04          	cmp    r10d,DWORD PTR [r9+0x4]
   1801dd42b:	73 05                	jae    0x1801dd432
   1801dd42d:	8d 50 ff             	lea    edx,[rax-0x1]
   1801dd430:	eb 06                	jmp    0x1801dd438
   1801dd432:	76 0c                	jbe    0x1801dd440
   1801dd434:	44 8d 40 01          	lea    r8d,[rax+0x1]
   1801dd438:	44 3b c2             	cmp    r8d,edx
   1801dd43b:	7e d7                	jle    0x1801dd414
   1801dd43d:	33 c0                	xor    eax,eax
   1801dd43f:	c3                   	ret
   1801dd440:	49 8b c1             	mov    rax,r9
   1801dd443:	c3                   	ret
