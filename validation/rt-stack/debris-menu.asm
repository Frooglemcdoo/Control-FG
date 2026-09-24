
/mnt/data/renderer_rmdwin10_f(5).dll:     file format pei-x86-64


Disassembly of section .text:

0000000180137c9d <.text+0x136c9d>:
   180137c9d:	8b 8f f0 06 00 00    	mov    ecx,DWORD PTR [rdi+0x6f0]
   180137ca3:	85 c9                	test   ecx,ecx
   180137ca5:	74 0d                	je     0x180137cb4
   180137ca7:	83 f9 01             	cmp    ecx,0x1
   180137caa:	75 16                	jne    0x180137cc2
   180137cac:	88 0d 1e 6d 7d 00    	mov    BYTE PTR [rip+0x7d6d1e],cl        # 0x18090e9d0
   180137cb2:	eb 07                	jmp    0x180137cbb
   180137cb4:	44 88 3d 15 6d 7d 00 	mov    BYTE PTR [rip+0x7d6d15],r15b        # 0x18090e9d0
   180137cbb:	c6 05 0a 6d 7d 00 01 	mov    BYTE PTR [rip+0x7d6d0a],0x1        # 0x18090e9cc
