
/workspace/scratch/65fb49337344/upload/renderer_rmdwin10_f(4).dll:     file format pei-x86-64


Disassembly of section .text:

000000018011de27 <.text+0x11ce27>:
   18011de27:	8b c8                	mov    ecx,eax
   18011de29:	b8 00 04 00 00       	mov    eax,0x400
   18011de2e:	38 1d ec 67 7f 00    	cmp    BYTE PTR [rip+0x7f67ec],bl        # 0x180914620
   18011de34:	0f 45 c8             	cmovne ecx,eax
   18011de37:	8b 05 93 4b 6e 00    	mov    eax,DWORD PTR [rip+0x6e4b93]        # 0x1808029d0
   18011de3d:	99                   	cdq
   18011de3e:	f7 f9                	idiv   ecx
   18011de40:	8b ca                	mov    ecx,edx
   18011de42:	8d 53 02             	lea    edx,[rbx+0x2]
