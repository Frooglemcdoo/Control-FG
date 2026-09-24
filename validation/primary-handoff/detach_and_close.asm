
../../upload/d3d_rmdwin10_f(4).dll:     file format pei-x86-64


Disassembly of section .text:

000000018004300b <.text+0x4200b>:
   18004300b:	48 8b 05 06 ec 0c 00 	mov    rax,QWORD PTR [rip+0xcec06]        # 0x180111c18
   180043012:	48 89 46 18          	mov    QWORD PTR [rsi+0x18],rax
   180043016:	48 8b 15 f3 eb 0c 00 	mov    rdx,QWORD PTR [rip+0xcebf3]        # 0x180111c10
   18004301d:	48 89 56 20          	mov    QWORD PTR [rsi+0x20],rdx
   180043021:	e8 ea 06 00 00       	call   0x180043710
   180043026:	48 89 3d eb eb 0c 00 	mov    QWORD PTR [rip+0xcebeb],rdi        # 0x180111c18
   18004302d:	48 89 3d dc eb 0c 00 	mov    QWORD PTR [rip+0xcebdc],rdi        # 0x180111c10
   180043034:	48 8b 5e 18          	mov    rbx,QWORD PTR [rsi+0x18]
   180043038:	48 85 db             	test   rbx,rbx
   18004303b:	74 2e                	je     0x18004306b
   18004303d:	48 8b 4b 18          	mov    rcx,QWORD PTR [rbx+0x18]
   180043041:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   180043044:	ff 50 48             	call   QWORD PTR [rax+0x48]
   180043047:	3d 0e 00 07 80       	cmp    eax,0x8007000e
   18004304c:	74 0b                	je     0x180043059
   18004304e:	8d 88 fb ff 85 77    	lea    ecx,[rax+0x7785fffb]
   180043054:	83 f9 02             	cmp    ecx,0x2
   180043057:	77 0e                	ja     0x180043067
   180043059:	8b d0                	mov    edx,eax
   18004305b:	48 8d 0d 9e e4 01 00 	lea    rcx,[rip+0x1e49e]        # 0x180061500
   180043062:	e8 f9 05 fe ff       	call   0x180023660
   180043067:	c6 43 28 01          	mov    BYTE PTR [rbx+0x28],0x1
