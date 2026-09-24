
/workspace/scratch/65fb49337344/upload/renderer_rmdwin10_f(4).dll:     file format pei-x86-64


Disassembly of section .text:

000000018011b668 <.text+0x11a668>:
   18011b668:	48 8d 05 77 1b 17 01 	lea    rax,[rip+0x1171b77]        # 0x18128d1e6
   18011b66f:	48 89 44 24 40       	mov    QWORD PTR [rsp+0x40],rax
   18011b674:	0f b6 86 49 07 00 00 	movzx  eax,BYTE PTR [rsi+0x749]
   18011b67b:	88 44 24 38          	mov    BYTE PTR [rsp+0x38],al
   18011b67f:	0f b6 05 9a 8f 7f 00 	movzx  eax,BYTE PTR [rip+0x7f8f9a]        # 0x180914620
   18011b686:	88 44 24 30          	mov    BYTE PTR [rsp+0x30],al
   18011b68a:	44 88 44 24 28       	mov    BYTE PTR [rsp+0x28],r8b
   18011b68f:	c6 44 24 20 00       	mov    BYTE PTR [rsp+0x20],0x0
   18011b694:	45 8b c2             	mov    r8d,r10d
   18011b697:	ff 15 bb 40 4c 00    	call   QWORD PTR [rip+0x4c40bb]        # 0x1805df758
   18011b69d:	88 05 9c 73 6e 00    	mov    BYTE PTR [rip+0x6e739c],al        # 0x180802a3f
   18011b6a3:	84 c0                	test   al,al
   18011b6a5:	75 2d                	jne    0x18011b6d4
   18011b6a7:	88 05 75 72 6e 00    	mov    BYTE PTR [rip+0x6e7275],al        # 0x180802922
   18011b6ad:	88 05 6d 8f 7f 00    	mov    BYTE PTR [rip+0x7f8f6d],al        # 0x180914620
   18011b6b3:	c6 05 62 8f 7f 00 01 	mov    BYTE PTR [rip+0x7f8f62],0x1        # 0x18091461c
   18011b6ba:	c6 05 25 1b 17 01 01 	mov    BYTE PTR [rip+0x1171b25],0x1        # 0x18128d1e6
   18011b6c1:	eb 11                	jmp    0x18011b6d4
