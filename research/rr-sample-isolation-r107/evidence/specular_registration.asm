
/workspace/scratch/093cfd391569/recovered-engine/renderer_rmdwin10_f(1).dll:     file format pei-x86-64


Disassembly of section .text:

00000001800060f0 <.text+0x50f0>:
   1800060f0:	48 83 ec 28          	sub    rsp,0x28
   1800060f4:	41 b8 01 00 00 00    	mov    r8d,0x1
   1800060fa:	48 8d 15 8f 8d 62 00 	lea    rdx,[rip+0x628d8f]        # 0x18062ee90
   180006101:	48 8d 0d 28 d6 90 00 	lea    rcx,[rip+0x90d628]        # 0x180913730
   180006108:	ff 15 1a b2 5d 00    	call   QWORD PTR [rip+0x5db21a]        # 0x1805e1328
   18000610e:	66 0f 6f 05 ba 97 67 	movdqa xmm0,XMMWORD PTR [rip+0x6797ba]        # 0x18067f8d0
   180006115:	00 
   180006116:	48 8d 05 bb bd 61 00 	lea    rax,[rip+0x61bdbb]        # 0x180621ed8
   18000611d:	48 8d 0d bc 6e 5c 00 	lea    rcx,[rip+0x5c6ebc]        # 0x1805ccfe0
   180006124:	48 89 05 05 d6 90 00 	mov    QWORD PTR [rip+0x90d605],rax        # 0x180913730
   18000612b:	66 0f 7f 05 ad d6 90 	movdqa XMMWORD PTR [rip+0x90d6ad],xmm0        # 0x1809137e0
   180006132:	00 
   180006133:	48 83 c4 28          	add    rsp,0x28
   180006137:	e9 88 1a 58 00       	jmp    0x180587bc4
