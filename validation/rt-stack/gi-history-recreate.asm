
/mnt/data/renderer_rmdwin10_f(5).dll:     file format pei-x86-64


Disassembly of section .text:

00000001800150d4 <.text+0x140d4>:
   1800150d4:	48 8b 0e             	mov    rcx,QWORD PTR [rsi]
   1800150d7:	48 85 c9             	test   rcx,rcx
   1800150da:	74 54                	je     0x180015130
   1800150dc:	8b 3d a6 93 8f 00    	mov    edi,DWORD PTR [rip+0x8f93a6]        # 0x18090e488
   1800150e2:	ff 15 00 ac 5c 00    	call   QWORD PTR [rip+0x5cac00]        # 0x1805dfce8
   1800150e8:	3b c7                	cmp    eax,edi
   1800150ea:	75 21                	jne    0x18001510d
   1800150ec:	8b 3d 9a 93 8f 00    	mov    edi,DWORD PTR [rip+0x8f939a]        # 0x18090e48c
   1800150f2:	48 8b 0e             	mov    rcx,QWORD PTR [rsi]
   1800150f5:	ff 15 e5 ab 5c 00    	call   QWORD PTR [rip+0x5cabe5]        # 0x1805dfce0
   1800150fb:	3b c7                	cmp    eax,edi
   1800150fd:	75 0e                	jne    0x18001510d
   1800150ff:	48 8b 0e             	mov    rcx,QWORD PTR [rsi]
   180015102:	ff 15 30 ac 5c 00    	call   QWORD PTR [rip+0x5cac30]        # 0x1805dfd38
   180015108:	41 3a c6             	cmp    al,r14b
   18001510b:	74 1c                	je     0x180015129
   18001510d:	48 8b 16             	mov    rdx,QWORD PTR [rsi]
   180015110:	48 c7 06 00 00 00 00 	mov    QWORD PTR [rsi],0x0
   180015117:	48 85 d2             	test   rdx,rdx
   18001511a:	74 0d                	je     0x180015129
   18001511c:	48 8b 0d b5 d7 7e 00 	mov    rcx,QWORD PTR [rip+0x7ed7b5]        # 0x1808028d8
   180015123:	e8 d8 4c 09 00       	call   0x1800a9e00
   180015128:	90                   	nop
   180015129:	48 8b bd d8 01 00 00 	mov    rdi,QWORD PTR [rbp+0x1d8]
   180015130:	48 83 3e 00          	cmp    QWORD PTR [rsi],0x0
   180015134:	0f 85 af 00 00 00    	jne    0x1800151e9
   18001513a:	0f 57 c0             	xorps  xmm0,xmm0
   18001513d:	0f 29 45 80          	movaps XMMWORD PTR [rbp-0x80],xmm0
   180015141:	c7 44 24 70 ff ff ff 	mov    DWORD PTR [rsp+0x70],0xffffffff
   180015148:	ff 
   180015149:	48 8d 45 80          	lea    rax,[rbp-0x80]
   18001514d:	48 89 44 24 68       	mov    QWORD PTR [rsp+0x68],rax
   180015152:	c7 44 24 60 01 00 00 	mov    DWORD PTR [rsp+0x60],0x1
   180015159:	00 
   18001515a:	f3 0f 11 74 24 58    	movss  DWORD PTR [rsp+0x58],xmm6
   180015160:	33 c0                	xor    eax,eax
   180015162:	89 44 24 50          	mov    DWORD PTR [rsp+0x50],eax
   180015166:	89 44 24 48          	mov    DWORD PTR [rsp+0x48],eax
   18001516a:	48 89 44 24 40       	mov    QWORD PTR [rsp+0x40],rax
   18001516f:	48 89 44 24 38       	mov    QWORD PTR [rsp+0x38],rax
   180015174:	89 44 24 30          	mov    DWORD PTR [rsp+0x30],eax
   180015178:	c7 44 24 28 05 00 00 	mov    DWORD PTR [rsp+0x28],0x5
   18001517f:	00 
   180015180:	44 88 74 24 20       	mov    BYTE PTR [rsp+0x20],r14b
   180015185:	44 8d 48 01          	lea    r9d,[rax+0x1]
   180015189:	44 8b 05 fc 92 8f 00 	mov    r8d,DWORD PTR [rip+0x8f92fc]        # 0x18090e48c
   180015190:	8b 15 f2 92 8f 00    	mov    edx,DWORD PTR [rip+0x8f92f2]        # 0x18090e488
   180015196:	48 8d 4d f0          	lea    rcx,[rbp-0x10]
   18001519a:	ff 15 88 ab 5c 00    	call   QWORD PTR [rip+0x5cab88]        # 0x1805dfd28
   1800151a0:	48 8b d0             	mov    rdx,rax
   1800151a3:	4c 8d 05 c6 a6 60 00 	lea    r8,[rip+0x60a6c6]        # 0x18061f870
   1800151aa:	48 8b cf             	mov    rcx,rdi
   1800151ad:	e8 ae 45 09 00       	call   0x1800a9760
   1800151b2:	48 8b 16             	mov    rdx,QWORD PTR [rsi]
   1800151b5:	48 89 06             	mov    QWORD PTR [rsi],rax
   1800151b8:	48 85 d2             	test   rdx,rdx
   1800151bb:	74 0f                	je     0x1800151cc
   1800151bd:	48 8b 0d 14 d7 7e 00 	mov    rcx,QWORD PTR [rip+0x7ed714]        # 0x1808028d8
   1800151c4:	e8 37 4c 09 00       	call   0x1800a9e00
   1800151c9:	48 8b 06             	mov    rax,QWORD PTR [rsi]
   1800151cc:	0f 57 c0             	xorps  xmm0,xmm0
   1800151cf:	0f 29 45 80          	movaps XMMWORD PTR [rbp-0x80],xmm0
   1800151d3:	48 8b c8             	mov    rcx,rax
   1800151d6:	ff 15 54 ab 5c 00    	call   QWORD PTR [rip+0x5cab54]        # 0x1805dfd30
   1800151dc:	48 8b c8             	mov    rcx,rax
   1800151df:	48 8d 55 80          	lea    rdx,[rbp-0x80]
   1800151e3:	ff 15 57 ab 5c 00    	call   QWORD PTR [rip+0x5cab57]        # 0x1805dfd40
