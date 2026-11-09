
/mnt/data/renderer_rmdwin10_f(5).dll:     file format pei-x86-64


Disassembly of section .text:

000000018012bd9d <.text+0x12ad9d>:
   18012bd9d:	48 8d 0d cc 87 7e 00 	lea    rcx,[rip+0x7e87cc]        # 0x180914570
   18012bda4:	e8 77 7f ee ff       	call   0x180013d20
   18012bda9:	84 c0                	test   al,al
   18012bdab:	75 09                	jne    0x18012bdb6
   18012bdad:	f3 44 0f 10 15 d2 31 	movss  xmm10,DWORD PTR [rip+0x5531d2]        # 0x18067ef88
   18012bdb4:	55 00 
   18012bdb6:	48 8d 0d 73 70 7e 00 	lea    rcx,[rip+0x7e7073]        # 0x180912e30
   18012bdbd:	e8 de 74 ee ff       	call   0x1800132a0
   18012bdc2:	f3 41 0f 59 c2       	mulss  xmm0,xmm10
