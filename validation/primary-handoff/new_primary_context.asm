
../../upload/d3d_rmdwin10_f(4).dll:     file format pei-x86-64


Disassembly of section .text:

0000000180042d01 <.text+0x41d01>:
   180042d01:	ff 15 d1 a8 01 00    	call   QWORD PTR [rip+0x1a8d1]        # 0x18005d5d8
   180042d07:	33 ff                	xor    edi,edi
   180042d09:	39 05 25 3f 0b 00    	cmp    DWORD PTR [rip+0xb3f25],eax        # 0x1800f6c34
   180042d0f:	75 44                	jne    0x180042d55
   180042d11:	b9 10 00 00 00       	mov    ecx,0x10
   180042d16:	48 8b 05 03 ef 0c 00 	mov    rax,QWORD PTR [rip+0xcef03]        # 0x180111c20
   180042d1d:	48 89 04 31          	mov    QWORD PTR [rcx+rsi*1],rax
   180042d21:	b9 20 00 00 00       	mov    ecx,0x20
   180042d26:	48 89 3c 31          	mov    QWORD PTR [rcx+rsi*1],rdi
   180042d2a:	49 8b 4e 10          	mov    rcx,QWORD PTR [r14+0x10]
   180042d2e:	48 8b 49 08          	mov    rcx,QWORD PTR [rcx+0x8]
   180042d32:	e8 59 f1 fb ff       	call   0x180001e90
   180042d37:	48 89 05 da ee 0c 00 	mov    QWORD PTR [rip+0xceeda],rax        # 0x180111c18
