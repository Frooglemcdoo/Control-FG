
/workspace/scratch/65fb49337344/upload/renderer_rmdwin10_f(4).dll:     file format pei-x86-64


Disassembly of section .text:

0000000180016010 <.text+0x15010>:
   180016010:	48 8b 0d 01 cb 8f 00 	mov    rcx,QWORD PTR [rip+0x8fcb01]        # 0x180912b18
   180016017:	48 85 c9             	test   rcx,rcx
   18001601a:	74 3b                	je     0x180016057
   18001601c:	8b 3d 66 84 8f 00    	mov    edi,DWORD PTR [rip+0x8f8466]        # 0x18090e488
   180016022:	ff 15 c0 9c 5c 00    	call   QWORD PTR [rip+0x5c9cc0]        # 0x1805dfce8
   180016028:	3b c7                	cmp    eax,edi
   18001602a:	75 17                	jne    0x180016043
   18001602c:	8b 3d 5a 84 8f 00    	mov    edi,DWORD PTR [rip+0x8f845a]        # 0x18090e48c
   180016032:	48 8b 0d df ca 8f 00 	mov    rcx,QWORD PTR [rip+0x8fcadf]        # 0x180912b18
   180016039:	ff 15 a1 9c 5c 00    	call   QWORD PTR [rip+0x5c9ca1]        # 0x1805dfce0
   18001603f:	3b c7                	cmp    eax,edi
   180016041:	74 05                	je     0x180016048
   180016043:	e8 d8 dd ff ff       	call   0x180013e20
   180016048:	33 ff                	xor    edi,edi
   18001604a:	48 39 3d c7 ca 8f 00 	cmp    QWORD PTR [rip+0x8fcac7],rdi        # 0x180912b18
   180016051:	0f 85 72 01 00 00    	jne    0x1800161c9
