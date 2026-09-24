
../../upload/d3d_rmdwin10_f(4).dll:     file format pei-x86-64


Disassembly of section .text:

0000000180016a40 <.text+0x15a40>:
   180016a40:	40 53                	rex push rbx
   180016a42:	48 83 ec 20          	sub    rsp,0x20
   180016a46:	48 63 c1             	movsxd rax,ecx
   180016a49:	49 8b d8             	mov    rbx,r8
   180016a4c:	48 c1 e0 04          	shl    rax,0x4
   180016a50:	48 8d 0d e9 bd 0f 00 	lea    rcx,[rip+0xfbde9]        # 0x180112840
   180016a57:	4c 63 0c 08          	movsxd r9,DWORD PTR [rax+rcx*1]
   180016a5b:	45 85 c9             	test   r9d,r9d
   180016a5e:	79 08                	jns    0x180016a68
   180016a60:	32 c0                	xor    al,al
   180016a62:	48 83 c4 20          	add    rsp,0x20
   180016a66:	5b                   	pop    rbx
   180016a67:	c3                   	ret
   180016a68:	4c 63 44 08 08       	movsxd r8,DWORD PTR [rax+rcx*1+0x8]
   180016a6d:	65 48 8b 04 25 58 00 	mov    rax,QWORD PTR gs:0x58
   180016a74:	00 00 
   180016a76:	8b 0d 80 ab 0f 00    	mov    ecx,DWORD PTR [rip+0xfab80]        # 0x1801115fc
   180016a7c:	48 8b 0c c8          	mov    rcx,QWORD PTR [rax+rcx*8]
   180016a80:	49 03 c9             	add    rcx,r9
   180016a83:	b8 50 42 00 00       	mov    eax,0x4250
   180016a88:	48 03 c8             	add    rcx,rax
   180016a8b:	e8 86 1b 04 00       	call   0x180058616
   180016a90:	89 03                	mov    DWORD PTR [rbx],eax
   180016a92:	b0 01                	mov    al,0x1
   180016a94:	48 83 c4 20          	add    rsp,0x20
   180016a98:	5b                   	pop    rbx
   180016a99:	c3                   	ret
