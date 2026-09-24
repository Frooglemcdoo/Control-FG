
upload/d3d_rmdwin10_f(4).dll:     file format pei-x86-64


Disassembly of section .text:

000000018002eb00 <.text+0x2db00>:
   18002eb00:	40 55                	rex push rbp
   18002eb02:	56                   	push   rsi
   18002eb03:	57                   	push   rdi
   18002eb04:	41 54                	push   r12
   18002eb06:	41 55                	push   r13
   18002eb08:	41 56                	push   r14
   18002eb0a:	41 57                	push   r15
   18002eb0c:	48 83 ec 40          	sub    rsp,0x40
   18002eb10:	48 c7 44 24 30 fe ff 	mov    QWORD PTR [rsp+0x30],0xfffffffffffffffe
   18002eb17:	ff ff 
   18002eb19:	48 89 9c 24 88 00 00 	mov    QWORD PTR [rsp+0x88],rbx
   18002eb20:	00 
   18002eb21:	4d 8b f9             	mov    r15,r9
   18002eb24:	49 8b f8             	mov    rdi,r8
   18002eb27:	8b ea                	mov    ebp,edx
   18002eb29:	48 8b d9             	mov    rbx,rcx
   18002eb2c:	44 8b 15 c9 2a 0e 00 	mov    r10d,DWORD PTR [rip+0xe2ac9]        # 0x1801115fc
   18002eb33:	65 48 8b 04 25 58 00 	mov    rax,QWORD PTR gs:0x58
   18002eb3a:	00 00 
   18002eb3c:	4e 8b 34 d0          	mov    r14,QWORD PTR [rax+r10*8]
   18002eb40:	4c 89 b4 24 80 00 00 	mov    QWORD PTR [rsp+0x80],r14
   18002eb47:	00 
   18002eb48:	b8 08 00 00 00       	mov    eax,0x8
   18002eb4d:	4a 83 3c 30 00       	cmp    QWORD PTR [rax+r14*1],0x0
   18002eb52:	75 2f                	jne    0x18002eb83
   18002eb54:	4c 63 f5             	movsxd r14,ebp
   18002eb57:	33 f6                	xor    esi,esi
   18002eb59:	85 d2                	test   edx,edx
   18002eb5b:	74 11                	je     0x18002eb6e
   18002eb5d:	48 8b 0c f7          	mov    rcx,QWORD PTR [rdi+rsi*8]
   18002eb61:	e8 ea c0 00 00       	call   0x18003ac50
   18002eb66:	48 ff c6             	inc    rsi
   18002eb69:	49 3b f6             	cmp    rsi,r14
   18002eb6c:	72 ef                	jb     0x18002eb5d
   18002eb6e:	4d 85 ff             	test   r15,r15
   18002eb71:	74 08                	je     0x18002eb7b
   18002eb73:	49 8b cf             	mov    rcx,r15
   18002eb76:	e8 d5 c0 00 00       	call   0x18003ac50
   18002eb7b:	4c 8b b4 24 80 00 00 	mov    r14,QWORD PTR [rsp+0x80]
   18002eb82:	00 
   18002eb83:	41 bc ff ff ff ff    	mov    r12d,0xffffffff
   18002eb89:	45 8b ec             	mov    r13d,r12d
   18002eb8c:	85 ed                	test   ebp,ebp
   18002eb8e:	7e 29                	jle    0x18002ebb9
   18002eb90:	48 8b cf             	mov    rcx,rdi
   18002eb93:	48 8b d5             	mov    rdx,rbp
   18002eb96:	66 66 0f 1f 84 00 00 	data16 nop WORD PTR [rax+rax*1+0x0]
   18002eb9d:	00 00 00 
   18002eba0:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   18002eba3:	48 85 c0             	test   rax,rax
   18002eba6:	74 07                	je     0x18002ebaf
   18002eba8:	44 8b 20             	mov    r12d,DWORD PTR [rax]
   18002ebab:	44 8b 68 04          	mov    r13d,DWORD PTR [rax+0x4]
   18002ebaf:	48 83 c1 08          	add    rcx,0x8
   18002ebb3:	48 83 ea 01          	sub    rdx,0x1
   18002ebb7:	75 e7                	jne    0x18002eba0
   18002ebb9:	4d 85 ff             	test   r15,r15
   18002ebbc:	74 07                	je     0x18002ebc5
   18002ebbe:	45 8b 27             	mov    r12d,DWORD PTR [r15]
   18002ebc1:	45 8b 6f 04          	mov    r13d,DWORD PTR [r15+0x4]
   18002ebc5:	85 ed                	test   ebp,ebp
   18002ebc7:	7e 20                	jle    0x18002ebe9
   18002ebc9:	48 85 ff             	test   rdi,rdi
   18002ebcc:	74 1b                	je     0x18002ebe9
   18002ebce:	48 8b 07             	mov    rax,QWORD PTR [rdi]
   18002ebd1:	48 8b 88 98 00 00 00 	mov    rcx,QWORD PTR [rax+0x98]
   18002ebd8:	48 89 0b             	mov    QWORD PTR [rbx],rcx
   18002ebdb:	48 8b 07             	mov    rax,QWORD PTR [rdi]
   18002ebde:	48 89 83 a0 09 00 00 	mov    QWORD PTR [rbx+0x9a0],rax
   18002ebe5:	33 c0                	xor    eax,eax
   18002ebe7:	eb 0c                	jmp    0x18002ebf5
   18002ebe9:	33 c0                	xor    eax,eax
   18002ebeb:	48 89 03             	mov    QWORD PTR [rbx],rax
   18002ebee:	48 89 83 a0 09 00 00 	mov    QWORD PTR [rbx+0x9a0],rax
   18002ebf5:	83 fd 01             	cmp    ebp,0x1
   18002ebf8:	7e 23                	jle    0x18002ec1d
   18002ebfa:	48 85 ff             	test   rdi,rdi
   18002ebfd:	74 1e                	je     0x18002ec1d
   18002ebff:	48 8b 47 08          	mov    rax,QWORD PTR [rdi+0x8]
   18002ec03:	48 8b 80 98 00 00 00 	mov    rax,QWORD PTR [rax+0x98]
   18002ec0a:	48 89 43 08          	mov    QWORD PTR [rbx+0x8],rax
   18002ec0e:	48 8b 47 08          	mov    rax,QWORD PTR [rdi+0x8]
   18002ec12:	48 89 83 a8 09 00 00 	mov    QWORD PTR [rbx+0x9a8],rax
   18002ec19:	33 c0                	xor    eax,eax
   18002ec1b:	eb 0b                	jmp    0x18002ec28
   18002ec1d:	48 89 43 08          	mov    QWORD PTR [rbx+0x8],rax
   18002ec21:	48 89 83 a8 09 00 00 	mov    QWORD PTR [rbx+0x9a8],rax
   18002ec28:	83 fd 02             	cmp    ebp,0x2
   18002ec2b:	7e 23                	jle    0x18002ec50
   18002ec2d:	48 85 ff             	test   rdi,rdi
   18002ec30:	74 1e                	je     0x18002ec50
   18002ec32:	48 8b 47 10          	mov    rax,QWORD PTR [rdi+0x10]
   18002ec36:	48 8b 80 98 00 00 00 	mov    rax,QWORD PTR [rax+0x98]
   18002ec3d:	48 89 43 10          	mov    QWORD PTR [rbx+0x10],rax
   18002ec41:	48 8b 47 10          	mov    rax,QWORD PTR [rdi+0x10]
   18002ec45:	48 89 83 b0 09 00 00 	mov    QWORD PTR [rbx+0x9b0],rax
   18002ec4c:	33 c0                	xor    eax,eax
   18002ec4e:	eb 0b                	jmp    0x18002ec5b
   18002ec50:	48 89 43 10          	mov    QWORD PTR [rbx+0x10],rax
   18002ec54:	48 89 83 b0 09 00 00 	mov    QWORD PTR [rbx+0x9b0],rax
   18002ec5b:	83 fd 03             	cmp    ebp,0x3
   18002ec5e:	7e 23                	jle    0x18002ec83
   18002ec60:	48 85 ff             	test   rdi,rdi
   18002ec63:	74 1e                	je     0x18002ec83
   18002ec65:	48 8b 47 18          	mov    rax,QWORD PTR [rdi+0x18]
   18002ec69:	48 8b 80 98 00 00 00 	mov    rax,QWORD PTR [rax+0x98]
   18002ec70:	48 89 43 18          	mov    QWORD PTR [rbx+0x18],rax
   18002ec74:	48 8b 47 18          	mov    rax,QWORD PTR [rdi+0x18]
   18002ec78:	48 89 83 b8 09 00 00 	mov    QWORD PTR [rbx+0x9b8],rax
   18002ec7f:	33 c0                	xor    eax,eax
   18002ec81:	eb 0b                	jmp    0x18002ec8e
   18002ec83:	48 89 43 18          	mov    QWORD PTR [rbx+0x18],rax
   18002ec87:	48 89 83 b8 09 00 00 	mov    QWORD PTR [rbx+0x9b8],rax
   18002ec8e:	83 fd 04             	cmp    ebp,0x4
   18002ec91:	7e 23                	jle    0x18002ecb6
   18002ec93:	48 85 ff             	test   rdi,rdi
   18002ec96:	74 1e                	je     0x18002ecb6
   18002ec98:	48 8b 47 20          	mov    rax,QWORD PTR [rdi+0x20]
   18002ec9c:	48 8b 80 98 00 00 00 	mov    rax,QWORD PTR [rax+0x98]
   18002eca3:	48 89 43 20          	mov    QWORD PTR [rbx+0x20],rax
   18002eca7:	48 8b 47 20          	mov    rax,QWORD PTR [rdi+0x20]
   18002ecab:	48 89 83 c0 09 00 00 	mov    QWORD PTR [rbx+0x9c0],rax
   18002ecb2:	33 c0                	xor    eax,eax
   18002ecb4:	eb 0b                	jmp    0x18002ecc1
   18002ecb6:	48 89 43 20          	mov    QWORD PTR [rbx+0x20],rax
   18002ecba:	48 89 83 c0 09 00 00 	mov    QWORD PTR [rbx+0x9c0],rax
   18002ecc1:	83 fd 05             	cmp    ebp,0x5
   18002ecc4:	7e 23                	jle    0x18002ece9
   18002ecc6:	48 85 ff             	test   rdi,rdi
   18002ecc9:	74 1e                	je     0x18002ece9
   18002eccb:	48 8b 47 28          	mov    rax,QWORD PTR [rdi+0x28]
   18002eccf:	48 8b 80 98 00 00 00 	mov    rax,QWORD PTR [rax+0x98]
   18002ecd6:	48 89 43 28          	mov    QWORD PTR [rbx+0x28],rax
   18002ecda:	48 8b 47 28          	mov    rax,QWORD PTR [rdi+0x28]
   18002ecde:	48 89 83 c8 09 00 00 	mov    QWORD PTR [rbx+0x9c8],rax
   18002ece5:	33 c0                	xor    eax,eax
   18002ece7:	eb 0b                	jmp    0x18002ecf4
   18002ece9:	48 89 43 28          	mov    QWORD PTR [rbx+0x28],rax
   18002eced:	48 89 83 c8 09 00 00 	mov    QWORD PTR [rbx+0x9c8],rax
   18002ecf4:	83 fd 06             	cmp    ebp,0x6
   18002ecf7:	7e 23                	jle    0x18002ed1c
   18002ecf9:	48 85 ff             	test   rdi,rdi
   18002ecfc:	74 1e                	je     0x18002ed1c
   18002ecfe:	48 8b 47 30          	mov    rax,QWORD PTR [rdi+0x30]
   18002ed02:	48 8b 80 98 00 00 00 	mov    rax,QWORD PTR [rax+0x98]
   18002ed09:	48 89 43 30          	mov    QWORD PTR [rbx+0x30],rax
   18002ed0d:	48 8b 47 30          	mov    rax,QWORD PTR [rdi+0x30]
   18002ed11:	48 89 83 d0 09 00 00 	mov    QWORD PTR [rbx+0x9d0],rax
   18002ed18:	33 c0                	xor    eax,eax
   18002ed1a:	eb 0b                	jmp    0x18002ed27
   18002ed1c:	48 89 43 30          	mov    QWORD PTR [rbx+0x30],rax
   18002ed20:	48 89 83 d0 09 00 00 	mov    QWORD PTR [rbx+0x9d0],rax
   18002ed27:	83 fd 07             	cmp    ebp,0x7
   18002ed2a:	7e 23                	jle    0x18002ed4f
   18002ed2c:	48 85 ff             	test   rdi,rdi
   18002ed2f:	74 1e                	je     0x18002ed4f
   18002ed31:	48 8b 47 38          	mov    rax,QWORD PTR [rdi+0x38]
   18002ed35:	48 8b 80 98 00 00 00 	mov    rax,QWORD PTR [rax+0x98]
   18002ed3c:	48 89 43 38          	mov    QWORD PTR [rbx+0x38],rax
   18002ed40:	48 8b 47 38          	mov    rax,QWORD PTR [rdi+0x38]
   18002ed44:	48 89 83 d8 09 00 00 	mov    QWORD PTR [rbx+0x9d8],rax
   18002ed4b:	33 c0                	xor    eax,eax
   18002ed4d:	eb 0b                	jmp    0x18002ed5a
   18002ed4f:	48 89 43 38          	mov    QWORD PTR [rbx+0x38],rax
   18002ed53:	48 89 83 d8 09 00 00 	mov    QWORD PTR [rbx+0x9d8],rax
   18002ed5a:	48 8d 73 40          	lea    rsi,[rbx+0x40]
   18002ed5e:	4d 85 ff             	test   r15,r15
   18002ed61:	74 10                	je     0x18002ed73
   18002ed63:	49 8b 87 a0 00 00 00 	mov    rax,QWORD PTR [r15+0xa0]
   18002ed6a:	4c 89 bb e0 09 00 00 	mov    QWORD PTR [rbx+0x9e0],r15
   18002ed71:	eb 07                	jmp    0x18002ed7a
   18002ed73:	48 89 83 e0 09 00 00 	mov    QWORD PTR [rbx+0x9e0],rax
   18002ed7a:	48 89 06             	mov    QWORD PTR [rsi],rax
   18002ed7d:	89 ab 3c 10 00 00    	mov    DWORD PTR [rbx+0x103c],ebp
   18002ed83:	41 bf 08 00 00 00    	mov    r15d,0x8
   18002ed89:	4b 8b 14 37          	mov    rdx,QWORD PTR [r15+r14*1]
   18002ed8d:	48 85 d2             	test   rdx,rdx
   18002ed90:	74 0b                	je     0x18002ed9d
   18002ed92:	bf 10 00 00 00       	mov    edi,0x10
   18002ed97:	4a 8b 3c 37          	mov    rdi,QWORD PTR [rdi+r14*1]
   18002ed9b:	eb 07                	jmp    0x18002eda4
   18002ed9d:	48 8b 3d 7c 2e 0e 00 	mov    rdi,QWORD PTR [rip+0xe2e7c]        # 0x180111c20
   18002eda4:	48 89 bc 24 98 00 00 	mov    QWORD PTR [rsp+0x98],rdi
   18002edab:	00 
   18002edac:	48 85 ff             	test   rdi,rdi
   18002edaf:	74 2d                	je     0x18002edde
   18002edb1:	ff 15 f9 e3 02 00    	call   QWORD PTR [rip+0x2e3f9]        # 0x18005d1b0
   18002edb7:	44 8b f0             	mov    r14d,eax
   18002edba:	39 47 08             	cmp    DWORD PTR [rdi+0x8],eax
   18002edbd:	74 0d                	je     0x18002edcc
   18002edbf:	48 8b cf             	mov    rcx,rdi
   18002edc2:	ff 15 f0 e3 02 00    	call   QWORD PTR [rip+0x2e3f0]        # 0x18005d1b8
   18002edc8:	44 89 77 08          	mov    DWORD PTR [rdi+0x8],r14d
   18002edcc:	ff 47 0c             	inc    DWORD PTR [rdi+0xc]
   18002edcf:	48 8b 06             	mov    rax,QWORD PTR [rsi]
   18002edd2:	48 8b 94 24 80 00 00 	mov    rdx,QWORD PTR [rsp+0x80]
   18002edd9:	00 
   18002edda:	49 8b 14 17          	mov    rdx,QWORD PTR [r15+rdx*1]
   18002edde:	48 85 c0             	test   rax,rax
   18002ede1:	41 be 00 00 00 00    	mov    r14d,0x0
   18002ede7:	49 0f 44 f6          	cmove  rsi,r14
   18002edeb:	48 8b 0d 26 2e 0e 00 	mov    rcx,QWORD PTR [rip+0xe2e26]        # 0x180111c18
   18002edf2:	48 85 d2             	test   rdx,rdx
   18002edf5:	48 0f 45 ca          	cmovne rcx,rdx
   18002edf9:	48 ff 41 08          	inc    QWORD PTR [rcx+0x8]
   18002edfd:	48 8b 49 18          	mov    rcx,QWORD PTR [rcx+0x18]
   18002ee01:	48 8b 01             	mov    rax,QWORD PTR [rcx]
   18002ee04:	48 89 74 24 20       	mov    QWORD PTR [rsp+0x20],rsi
   18002ee09:	45 33 c9             	xor    r9d,r9d
   18002ee0c:	4c 8b c3             	mov    r8,rbx
   18002ee0f:	8b d5                	mov    edx,ebp
   18002ee11:	ff 90 70 01 00 00    	call   QWORD PTR [rax+0x170]
   18002ee17:	90                   	nop
   18002ee18:	48 85 ff             	test   rdi,rdi
   18002ee1b:	74 16                	je     0x18002ee33
   18002ee1d:	83 47 0c ff          	add    DWORD PTR [rdi+0xc],0xffffffff
   18002ee21:	75 10                	jne    0x18002ee33
   18002ee23:	c7 47 08 ff ff ff ff 	mov    DWORD PTR [rdi+0x8],0xffffffff
   18002ee2a:	48 8b cf             	mov    rcx,rdi
   18002ee2d:	ff 15 75 e3 02 00    	call   QWORD PTR [rip+0x2e375]        # 0x18005d1a8
   18002ee33:	48 8d 93 94 08 00 00 	lea    rdx,[rbx+0x894]
   18002ee3a:	44 89 32             	mov    DWORD PTR [rdx],r14d
   18002ee3d:	44 89 b3 98 08 00 00 	mov    DWORD PTR [rbx+0x898],r14d
   18002ee44:	66 41 0f 6e c4       	movd   xmm0,r12d
   18002ee49:	0f 5b c0             	cvtdq2ps xmm0,xmm0
   18002ee4c:	f3 0f 11 83 9c 08 00 	movss  DWORD PTR [rbx+0x89c],xmm0
   18002ee53:	00 
   18002ee54:	66 41 0f 6e cd       	movd   xmm1,r13d
   18002ee59:	0f 5b c9             	cvtdq2ps xmm1,xmm1
   18002ee5c:	f3 0f 11 8b a0 08 00 	movss  DWORD PTR [rbx+0x8a0],xmm1
   18002ee63:	00 
   18002ee64:	44 89 b3 a4 08 00 00 	mov    DWORD PTR [rbx+0x8a4],r14d
   18002ee6b:	c7 83 a8 08 00 00 00 	mov    DWORD PTR [rbx+0x8a8],0x3f800000
   18002ee72:	00 80 3f 
   18002ee75:	48 8b cb             	mov    rcx,rbx
   18002ee78:	e8 b3 fa ff ff       	call   0x18002e930
   18002ee7d:	44 89 6c 24 20       	mov    DWORD PTR [rsp+0x20],r13d
   18002ee82:	45 8b cc             	mov    r9d,r12d
   18002ee85:	45 33 c0             	xor    r8d,r8d
   18002ee88:	33 d2                	xor    edx,edx
   18002ee8a:	48 8b cb             	mov    rcx,rbx
   18002ee8d:	e8 7e 07 00 00       	call   0x18002f610
   18002ee92:	c6 83 90 08 00 00 00 	mov    BYTE PTR [rbx+0x890],0x0
   18002ee99:	48 8b 9c 24 88 00 00 	mov    rbx,QWORD PTR [rsp+0x88]
   18002eea0:	00 
   18002eea1:	48 83 c4 40          	add    rsp,0x40
   18002eea5:	41 5f                	pop    r15
   18002eea7:	41 5e                	pop    r14
   18002eea9:	41 5d                	pop    r13
   18002eeab:	41 5c                	pop    r12
   18002eead:	5f                   	pop    rdi
   18002eeae:	5e                   	pop    rsi
   18002eeaf:	5d                   	pop    rbp
   18002eeb0:	c3                   	ret
