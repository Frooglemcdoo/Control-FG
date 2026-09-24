
/workspace/scratch/65fb49337344/upload/d3d_rmdwin10_f(4).dll:     file format pei-x86-64


Disassembly of section .text:

0000000180051d30 <.text+0x50d30>:
   180051d30:	48 89 5c 24 08       	mov    QWORD PTR [rsp+0x8],rbx
   180051d35:	48 89 74 24 10       	mov    QWORD PTR [rsp+0x10],rsi
   180051d3a:	48 89 7c 24 18       	mov    QWORD PTR [rsp+0x18],rdi
   180051d3f:	41 56                	push   r14
   180051d41:	48 83 ec 20          	sub    rsp,0x20
   180051d45:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   180051d48:	4c 8b f1             	mov    r14,rcx
   180051d4b:	49 8b f8             	mov    rdi,r8
   180051d4e:	48 8b f2             	mov    rsi,rdx
   180051d51:	48 8b 58 40          	mov    rbx,QWORD PTR [rax+0x40]
   180051d55:	48 8b cb             	mov    rcx,rbx
   180051d58:	ff 15 6a bb 00 00    	call   QWORD PTR [rip+0xbb6a]        # 0x18005d8c8
   180051d5e:	4c 8b c7             	mov    r8,rdi
   180051d61:	48 8b d6             	mov    rdx,rsi
   180051d64:	49 8b ce             	mov    rcx,r14
   180051d67:	48 8b c3             	mov    rax,rbx
   180051d6a:	48 8b 5c 24 30       	mov    rbx,QWORD PTR [rsp+0x30]
   180051d6f:	48 8b 74 24 38       	mov    rsi,QWORD PTR [rsp+0x38]
   180051d74:	48 8b 7c 24 40       	mov    rdi,QWORD PTR [rsp+0x40]
   180051d79:	48 83 c4 20          	add    rsp,0x20
   180051d7d:	41 5e                	pop    r14
   180051d7f:	48 ff e0             	rex.W jmp rax
