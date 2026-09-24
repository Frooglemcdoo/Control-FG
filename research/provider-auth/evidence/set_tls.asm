
../../upload/d3d_rmdwin10_f(4).dll:     file format pei-x86-64


Disassembly of section .text:

0000000180016980 <.text+0x15980>:
   180016980:	48 89 5c 24 10       	mov    QWORD PTR [rsp+0x10],rbx
   180016985:	56                   	push   rsi
   180016986:	48 83 ec 20          	sub    rsp,0x20
   18001698a:	48 63 d9             	movsxd rbx,ecx
   18001698d:	48 8d 35 ac be 0f 00 	lea    rsi,[rip+0xfbeac]        # 0x180112840
   180016994:	48 03 db             	add    rbx,rbx
   180016997:	4c 63 14 de          	movsxd r10,DWORD PTR [rsi+rbx*8]
   18001699b:	45 85 d2             	test   r10d,r10d
   18001699e:	78 59                	js     0x1800169f9
   1800169a0:	8b 0d 56 ac 0f 00    	mov    ecx,DWORD PTR [rip+0xfac56]        # 0x1801115fc
   1800169a6:	65 48 8b 04 25 58 00 	mov    rax,QWORD PTR gs:0x58
   1800169ad:	00 00 
   1800169af:	48 89 7c 24 30       	mov    QWORD PTR [rsp+0x30],rdi
   1800169b4:	48 8b 3c c8          	mov    rdi,QWORD PTR [rax+rcx*8]
   1800169b8:	b9 50 42 00 00       	mov    ecx,0x4250
   1800169bd:	4a 8d 04 17          	lea    rax,[rdi+r10*1]
   1800169c1:	48 03 c8             	add    rcx,rax
   1800169c4:	48 03 ca             	add    rcx,rdx
   1800169c7:	49 8b d1             	mov    rdx,r9
   1800169ca:	e8 2f 1c 04 00       	call   0x1800585fe
   1800169cf:	48 63 44 de 0c       	movsxd rax,DWORD PTR [rsi+rbx*8+0xc]
   1800169d4:	83 f8 ff             	cmp    eax,0xffffffff
   1800169d7:	74 1b                	je     0x1800169f4
   1800169d9:	48 8b c8             	mov    rcx,rax
   1800169dc:	b8 50 00 00 00       	mov    eax,0x50
   1800169e1:	48 03 c7             	add    rax,rdi
   1800169e4:	ff 04 88             	inc    DWORD PTR [rax+rcx*4]
   1800169e7:	83 3c 88 ff          	cmp    DWORD PTR [rax+rcx*4],0xffffffff
   1800169eb:	75 07                	jne    0x1800169f4
   1800169ed:	c7 04 88 00 00 00 00 	mov    DWORD PTR [rax+rcx*4],0x0
   1800169f4:	48 8b 7c 24 30       	mov    rdi,QWORD PTR [rsp+0x30]
   1800169f9:	48 8b 5c 24 38       	mov    rbx,QWORD PTR [rsp+0x38]
   1800169fe:	48 83 c4 20          	add    rsp,0x20
   180016a02:	5e                   	pop    rsi
   180016a03:	c3                   	ret
