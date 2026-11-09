
/mnt/data/renderer_rmdwin10_f(5).dll:     file format pei-x86-64


Disassembly of section .text:

0000000180016786 <.text+0x15786>:
   180016786:	ff 15 34 95 5c 00    	call   QWORD PTR [rip+0x5c9534]        # 0x1805dfcc0
   18001678c:	4c 8b e6             	mov    r12,rsi
   18001678f:	4c 8b f3             	mov    r14,rbx
   180016792:	48 c7 45 c0 00 00 00 	mov    QWORD PTR [rbp-0x40],0x0
   180016799:	00 
   18001679a:	89 7d c8             	mov    DWORD PTR [rbp-0x38],edi
   18001679d:	48 c7 45 80 00 00 00 	mov    QWORD PTR [rbp-0x80],0x0
   1800167a4:	00 
   1800167a5:	89 7d 88             	mov    DWORD PTR [rbp-0x78],edi
   1800167a8:	48 8b 3d 69 c3 8f 00 	mov    rdi,QWORD PTR [rip+0x8fc369]        # 0x180912b18
   1800167af:	48 8b cb             	mov    rcx,rbx
   1800167b2:	ff 15 78 95 5c 00    	call   QWORD PTR [rip+0x5c9578]        # 0x1805dfd30
   1800167b8:	48 8b f0             	mov    rsi,rax
   1800167bb:	48 8b cf             	mov    rcx,rdi
   1800167be:	ff 15 6c 95 5c 00    	call   QWORD PTR [rip+0x5c956c]        # 0x1805dfd30
   1800167c4:	48 8b c8             	mov    rcx,rax
   1800167c7:	33 ff                	xor    edi,edi
   1800167c9:	48 89 7c 24 40       	mov    QWORD PTR [rsp+0x40],rdi
   1800167ce:	48 8d 45 c0          	lea    rax,[rbp-0x40]
   1800167d2:	48 89 44 24 38       	mov    QWORD PTR [rsp+0x38],rax
   1800167d7:	89 7c 24 30          	mov    DWORD PTR [rsp+0x30],edi
   1800167db:	89 7c 24 28          	mov    DWORD PTR [rsp+0x28],edi
   1800167df:	48 89 74 24 20       	mov    QWORD PTR [rsp+0x20],rsi
   1800167e4:	4c 8d 4d 80          	lea    r9,[rbp-0x80]
   1800167e8:	45 33 c0             	xor    r8d,r8d
   1800167eb:	33 d2                	xor    edx,edx
   1800167ed:	ff 15 55 95 5c 00    	call   QWORD PTR [rip+0x5c9555]        # 0x1805dfd48
   1800167f3:	8b f7                	mov    esi,edi
   1800167f5:	44 8b ad e0 01 00 00 	mov    r13d,DWORD PTR [rbp+0x1e0]
   1800167fc:	45 85 ed             	test   r13d,r13d
   1800167ff:	0f 84 51 03 00 00    	je     0x180016b56
