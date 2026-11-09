
/workspace/scratch/65fb49337344/upload/renderer_rmdwin10_f(4).dll:     file format pei-x86-64


Disassembly of section .text:

0000000180137bb1 <.text+0x136bb1>:
   180137bb1:	8b 97 e0 06 00 00    	mov    edx,DWORD PTR [rdi+0x6e0]
   180137bb7:	85 d2                	test   edx,edx
   180137bb9:	74 20                	je     0x180137bdb
   180137bbb:	83 ea 01             	sub    edx,0x1
   180137bbe:	74 05                	je     0x180137bc5
   180137bc0:	83 fa 01             	cmp    edx,0x1
   180137bc3:	75 24                	jne    0x180137be9
   180137bc5:	c6 05 34 b1 7d 00 01 	mov    BYTE PTR [rip+0x7db134],0x1        # 0x180912d00
   180137bcc:	89 0d 0e bc 7d 00    	mov    DWORD PTR [rip+0x7dbc0e],ecx        # 0x1809137e0
   180137bd2:	c6 05 03 bc 7d 00 01 	mov    BYTE PTR [rip+0x7dbc03],0x1        # 0x1809137dc
   180137bd9:	eb 07                	jmp    0x180137be2
   180137bdb:	44 88 3d 1e b1 7d 00 	mov    BYTE PTR [rip+0x7db11e],r15b        # 0x180912d00
   180137be2:	c6 05 13 b1 7d 00 01 	mov    BYTE PTR [rip+0x7db113],0x1        # 0x180912cfc
   180137be9:	83 f9 06             	cmp    ecx,0x6
   180137bec:	c6 05 39 ba 7d 00 01 	mov    BYTE PTR [rip+0x7dba39],0x1        # 0x18091362c
   180137bf3:	b8 04 00 00 00       	mov    eax,0x4
   180137bf8:	41 0f 4d c4          	cmovge eax,r12d
   180137bfc:	89 05 2e ba 7d 00    	mov    DWORD PTR [rip+0x7dba2e],eax        # 0x180913630
   180137c02:	8b 97 e4 06 00 00    	mov    edx,DWORD PTR [rdi+0x6e4]
   180137c08:	85 d2                	test   edx,edx
   180137c0a:	74 13                	je     0x180137c1f
   180137c0c:	83 ea 01             	sub    edx,0x1
   180137c0f:	74 05                	je     0x180137c16
   180137c11:	83 fa 01             	cmp    edx,0x1
   180137c14:	75 17                	jne    0x180137c2d
   180137c16:	c6 05 43 c6 7d 00 01 	mov    BYTE PTR [rip+0x7dc643],0x1        # 0x180914260
   180137c1d:	eb 07                	jmp    0x180137c26
   180137c1f:	44 88 3d 3a c6 7d 00 	mov    BYTE PTR [rip+0x7dc63a],r15b        # 0x180914260
   180137c26:	c6 05 2f c6 7d 00 01 	mov    BYTE PTR [rip+0x7dc62f],0x1        # 0x18091425c
   180137c2d:	8b 97 e8 06 00 00    	mov    edx,DWORD PTR [rdi+0x6e8]
   180137c33:	85 d2                	test   edx,edx
   180137c35:	74 20                	je     0x180137c57
   180137c37:	83 ea 01             	sub    edx,0x1
   180137c3a:	74 05                	je     0x180137c41
   180137c3c:	83 fa 01             	cmp    edx,0x1
   180137c3f:	75 24                	jne    0x180137c65
   180137c41:	c6 05 a8 b1 7d 00 01 	mov    BYTE PTR [rip+0x7db1a8],0x1        # 0x180912df0
   180137c48:	89 0d 82 bf 7d 00    	mov    DWORD PTR [rip+0x7dbf82],ecx        # 0x180913bd0
   180137c4e:	c6 05 77 bf 7d 00 01 	mov    BYTE PTR [rip+0x7dbf77],0x1        # 0x180913bcc
   180137c55:	eb 07                	jmp    0x180137c5e
   180137c57:	44 88 3d 92 b1 7d 00 	mov    BYTE PTR [rip+0x7db192],r15b        # 0x180912df0
   180137c5e:	c6 05 87 b1 7d 00 01 	mov    BYTE PTR [rip+0x7db187],0x1        # 0x180912dec
   180137c65:	8b 97 ec 06 00 00    	mov    edx,DWORD PTR [rdi+0x6ec]
   180137c6b:	85 d2                	test   edx,edx
   180137c6d:	74 20                	je     0x180137c8f
   180137c6f:	83 ea 01             	sub    edx,0x1
   180137c72:	74 05                	je     0x180137c79
   180137c74:	83 ea 01             	sub    edx,0x1
   180137c77:	75 24                	jne    0x180137c9d
   180137c79:	c6 05 e0 b3 7d 00 01 	mov    BYTE PTR [rip+0x7db3e0],0x1        # 0x180913060
   180137c80:	89 0d 0a c0 7d 00    	mov    DWORD PTR [rip+0x7dc00a],ecx        # 0x180913c90
   180137c86:	c6 05 ff bf 7d 00 01 	mov    BYTE PTR [rip+0x7dbfff],0x1        # 0x180913c8c
   180137c8d:	eb 07                	jmp    0x180137c96
   180137c8f:	44 88 3d ca b3 7d 00 	mov    BYTE PTR [rip+0x7db3ca],r15b        # 0x180913060
