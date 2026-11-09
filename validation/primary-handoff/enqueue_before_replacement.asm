
../../upload/d3d_rmdwin10_f(4).dll:     file format pei-x86-64


Disassembly of section .text:

0000000180032962 <.text+0x31962>:
   180032962:	4c 89 ac 24 a8 00 00 	mov    QWORD PTR [rsp+0xa8],r13
   180032969:	00 
   18003296a:	48 8d 4e 18          	lea    rcx,[rsi+0x18]
   18003296e:	48 8d 94 24 a8 00 00 	lea    rdx,[rsp+0xa8]
   180032975:	00 
   180032976:	e8 65 fb fc ff       	call   0x1800024e0
   18003297b:	48 8b 94 24 a8 00 00 	mov    rdx,QWORD PTR [rsp+0xa8]
   180032982:	00 
   180032983:	48 83 c2 18          	add    rdx,0x18
   180032987:	48 83 3a 00          	cmp    QWORD PTR [rdx],0x0
   18003298b:	74 09                	je     0x180032996
   18003298d:	48 8d 4e 28          	lea    rcx,[rsi+0x28]
   180032991:	e8 4a fb fc ff       	call   0x1800024e0
   180032996:	8b 56 30             	mov    edx,DWORD PTR [rsi+0x30]
   180032999:	48 63 05 60 49 10 00 	movsxd rax,DWORD PTR [rip+0x104960]        # 0x180137300
   1800329a0:	48 3b d0             	cmp    rdx,rax
   1800329a3:	72 09                	jb     0x1800329ae
   1800329a5:	48 8b ce             	mov    rcx,rsi
   1800329a8:	e8 f3 0a 01 00       	call   0x1800434a0
   1800329ad:	90                   	nop
   1800329ae:	83 47 0c ff          	add    DWORD PTR [rdi+0xc],0xffffffff
   1800329b2:	75 10                	jne    0x1800329c4
   1800329b4:	c7 47 08 ff ff ff ff 	mov    DWORD PTR [rdi+0x8],0xffffffff
   1800329bb:	48 8b cf             	mov    rcx,rdi
   1800329be:	ff 15 e4 a7 02 00    	call   QWORD PTR [rip+0x2a7e4]        # 0x18005d1a8
   1800329c4:	45 33 ff             	xor    r15d,r15d
   1800329c7:	4c 89 7c 24 38       	mov    QWORD PTR [rsp+0x38],r15
   1800329cc:	c7 44 24 40 01 00 00 	mov    DWORD PTR [rsp+0x40],0x1
   1800329d3:	00 
   1800329d4:	4c 89 74 24 48       	mov    QWORD PTR [rsp+0x48],r14
   1800329d9:	48 8d 4c 24 38       	lea    rcx,[rsp+0x38]
   1800329de:	e8 dd 02 01 00       	call   0x180042cc0
