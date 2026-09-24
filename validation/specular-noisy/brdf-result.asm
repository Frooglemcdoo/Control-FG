
/mnt/data/renderer_rmdwin10_f(5).dll:     file format pei-x86-64


Disassembly of section .text:

0000000180016ee2 <.text+0x15ee2>:
   180016ee2:	ff 15 d8 8d 5c 00    	call   QWORD PTR [rip+0x5c8dd8]        # 0x1805dfcc0
   180016ee8:	4d 8b f4             	mov    r14,r12
   180016eeb:	48 8b b5 d0 01 00 00 	mov    rsi,QWORD PTR [rbp+0x1d0]
   180016ef2:	4c 3b f6             	cmp    r14,rsi
   180016ef5:	74 59                	je     0x180016f50
   180016ef7:	48 c7 45 a0 00 00 00 	mov    QWORD PTR [rbp-0x60],0x0
   180016efe:	00 
   180016eff:	89 7d a8             	mov    DWORD PTR [rbp-0x58],edi
   180016f02:	48 c7 45 d0 00 00 00 	mov    QWORD PTR [rbp-0x30],0x0
   180016f09:	00 
   180016f0a:	89 7d d8             	mov    DWORD PTR [rbp-0x28],edi
   180016f0d:	49 8b ce             	mov    rcx,r14
   180016f10:	ff 15 1a 8e 5c 00    	call   QWORD PTR [rip+0x5c8e1a]        # 0x1805dfd30
   180016f16:	48 8b f8             	mov    rdi,rax
   180016f19:	48 8b ce             	mov    rcx,rsi
   180016f1c:	ff 15 0e 8e 5c 00    	call   QWORD PTR [rip+0x5c8e0e]        # 0x1805dfd30
   180016f22:	48 8b c8             	mov    rcx,rax
   180016f25:	33 d2                	xor    edx,edx
   180016f27:	48 89 54 24 40       	mov    QWORD PTR [rsp+0x40],rdx
   180016f2c:	48 8d 45 a0          	lea    rax,[rbp-0x60]
   180016f30:	48 89 44 24 38       	mov    QWORD PTR [rsp+0x38],rax
   180016f35:	89 54 24 30          	mov    DWORD PTR [rsp+0x30],edx
   180016f39:	89 54 24 28          	mov    DWORD PTR [rsp+0x28],edx
   180016f3d:	48 89 7c 24 20       	mov    QWORD PTR [rsp+0x20],rdi
   180016f42:	4c 8d 4d d0          	lea    r9,[rbp-0x30]
   180016f46:	45 33 c0             	xor    r8d,r8d
   180016f49:	ff 15 f9 8d 5c 00    	call   QWORD PTR [rip+0x5c8df9]        # 0x1805dfd48
   180016f4f:	90                   	nop
   180016f50:	48 85 db             	test   rbx,rbx
   180016f53:	74 10                	je     0x180016f65
   180016f55:	48 8b d3             	mov    rdx,rbx
   180016f58:	48 8b 0d 79 b9 7e 00 	mov    rcx,QWORD PTR [rip+0x7eb979]        # 0x1808028d8
   180016f5f:	e8 9c 2e 09 00       	call   0x1800a9e00
   180016f64:	90                   	nop
