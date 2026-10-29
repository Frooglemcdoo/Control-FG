
../gi-research-input/Control FG mod/renderer_rmdwin10_f(4).dll:     file format pei-x86-64


Disassembly of section .text:

00000001801db750 <.text+0x1da750>:
   1801db750:	4c 8b dc             	mov    r11,rsp
   1801db753:	49 89 5b 10          	mov    QWORD PTR [r11+0x10],rbx
   1801db757:	49 89 73 18          	mov    QWORD PTR [r11+0x18],rsi
   1801db75b:	57                   	push   rdi
   1801db75c:	48 83 ec 60          	sub    rsp,0x60
   1801db760:	48 8b 05 a9 08 61 00 	mov    rax,QWORD PTR [rip+0x6108a9]        # 0x1807ec010
   1801db767:	48 33 c4             	xor    rax,rsp
   1801db76a:	48 89 44 24 58       	mov    QWORD PTR [rsp+0x58],rax
   1801db76f:	48 8b 41 10          	mov    rax,QWORD PTR [rcx+0x10]
   1801db773:	48 8b d9             	mov    rbx,rcx
   1801db776:	49 89 43 b8          	mov    QWORD PTR [r11-0x48],rax
   1801db77a:	48 8b 41 20          	mov    rax,QWORD PTR [rcx+0x20]
   1801db77e:	49 89 43 c0          	mov    QWORD PTR [r11-0x40],rax
   1801db782:	48 8b 41 30          	mov    rax,QWORD PTR [rcx+0x30]
   1801db786:	49 89 43 c8          	mov    QWORD PTR [r11-0x38],rax
   1801db78a:	48 8b 41 40          	mov    rax,QWORD PTR [rcx+0x40]
   1801db78e:	49 89 43 d0          	mov    QWORD PTR [r11-0x30],rax
   1801db792:	48 8b 41 50          	mov    rax,QWORD PTR [rcx+0x50]
   1801db796:	49 89 43 d8          	mov    QWORD PTR [r11-0x28],rax
   1801db79a:	48 8b 41 60          	mov    rax,QWORD PTR [rcx+0x60]
   1801db79e:	49 89 43 e0          	mov    QWORD PTR [r11-0x20],rax
   1801db7a2:	48 8b 41 70          	mov    rax,QWORD PTR [rcx+0x70]
   1801db7a6:	49 89 43 e8          	mov    QWORD PTR [r11-0x18],rax
   1801db7aa:	ff 15 00 45 40 00    	call   QWORD PTR [rip+0x404500]        # 0x1805dfcb0
   1801db7b0:	44 0f b6 43 01       	movzx  r8d,BYTE PTR [rbx+0x1]
   1801db7b5:	48 8d 54 24 20       	lea    rdx,[rsp+0x20]
   1801db7ba:	48 8b c8             	mov    rcx,rax
   1801db7bd:	48 8b f0             	mov    rsi,rax
   1801db7c0:	ff 15 4a 43 40 00    	call   QWORD PTR [rip+0x40434a]        # 0x1805dfb10
   1801db7c6:	48 83 c3 18          	add    rbx,0x18
   1801db7ca:	bf 07 00 00 00       	mov    edi,0x7
   1801db7cf:	48 83 7b f8 00       	cmp    QWORD PTR [rbx-0x8],0x0
   1801db7d4:	74 0c                	je     0x1801db7e2
   1801db7d6:	48 8b 0b             	mov    rcx,QWORD PTR [rbx]
   1801db7d9:	48 8b d6             	mov    rdx,rsi
   1801db7dc:	ff 15 36 43 40 00    	call   QWORD PTR [rip+0x404336]        # 0x1805dfb18
   1801db7e2:	48 83 c3 10          	add    rbx,0x10
   1801db7e6:	48 83 ef 01          	sub    rdi,0x1
   1801db7ea:	75 e3                	jne    0x1801db7cf
   1801db7ec:	48 8b 4c 24 58       	mov    rcx,QWORD PTR [rsp+0x58]
   1801db7f1:	48 33 cc             	xor    rcx,rsp
   1801db7f4:	e8 d7 be 3a 00       	call   0x1805876d0
   1801db7f9:	48 8b 5c 24 78       	mov    rbx,QWORD PTR [rsp+0x78]
   1801db7fe:	48 8b b4 24 80 00 00 	mov    rsi,QWORD PTR [rsp+0x80]
   1801db805:	00 
   1801db806:	48 83 c4 60          	add    rsp,0x60
   1801db80a:	5f                   	pop    rdi
   1801db80b:	c3                   	ret
   1801db80c:	cc                   	int3
   1801db80d:	cc                   	int3
   1801db80e:	cc                   	int3
   1801db80f:	cc                   	int3
