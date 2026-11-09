
../../upload/renderer_rmdwin10_f(4).dll:     file format pei-x86-64


Disassembly of section .text:

0000000180127cef <.text+0x126cef>:
   180127cef:	8b 0d 47 ab 6d 00    	mov    ecx,DWORD PTR [rip+0x6dab47]        # 0x18080283c
   180127cf5:	65 48 8b 04 25 58 00 	mov    rax,QWORD PTR gs:0x58
   180127cfc:	00 00 
   180127cfe:	48 8b 1c c8          	mov    rbx,QWORD PTR [rax+rcx*8]
   180127d02:	be 08 00 00 00       	mov    esi,0x8
   180127d07:	48 8b 34 1e          	mov    rsi,QWORD PTR [rsi+rbx*1]
   180127d0b:	48 89 74 24 58       	mov    QWORD PTR [rsp+0x58],rsi
   180127d10:	41 be 10 00 00 00    	mov    r14d,0x10
   180127d16:	4d 8b 34 1e          	mov    r14,QWORD PTR [r14+rbx*1]
   180127d1a:	4c 89 74 24 60       	mov    QWORD PTR [rsp+0x60],r14
   180127d1f:	48 8b ce             	mov    rcx,rsi
   180127d22:	e8 39 6e 05 00       	call   0x18017eb60
