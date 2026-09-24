
/workspace/scratch/65fb49337344/upload/d3d_rmdwin10_f(4).dll:     file format pei-x86-64


Disassembly of section .text:

000000018001cdf0 <.text+0x1bdf0>:
   18001cdf0:	48 89 5c 24 08       	mov    QWORD PTR [rsp+0x8],rbx
   18001cdf5:	48 89 6c 24 10       	mov    QWORD PTR [rsp+0x10],rbp
   18001cdfa:	48 89 74 24 18       	mov    QWORD PTR [rsp+0x18],rsi
   18001cdff:	57                   	push   rdi
   18001ce00:	48 83 ec 40          	sub    rsp,0x40
   18001ce04:	49 8b d8             	mov    rbx,r8
   18001ce07:	0f 29 74 24 30       	movaps XMMWORD PTR [rsp+0x30],xmm6
   18001ce0c:	4d 8b 01             	mov    r8,QWORD PTR [r9]
   18001ce0f:	48 8b f2             	mov    rsi,rdx
   18001ce12:	48 8b e9             	mov    rbp,rcx
   18001ce15:	0f 29 7c 24 20       	movaps XMMWORD PTR [rsp+0x20],xmm7
   18001ce1a:	48 8b cb             	mov    rcx,rbx
   18001ce1d:	48 8d 15 c8 6f 04 00 	lea    rdx,[rip+0x46fc8]        # 0x180063dec
   18001ce24:	49 8b f9             	mov    rdi,r9
   18001ce27:	e8 14 50 03 00       	call   0x180051e40
   18001ce2c:	4c 8b 47 08          	mov    r8,QWORD PTR [rdi+0x8]
   18001ce30:	48 8d 15 bd 6f 04 00 	lea    rdx,[rip+0x46fbd]        # 0x180063df4
   18001ce37:	48 8b cb             	mov    rcx,rbx
   18001ce3a:	e8 01 50 03 00       	call   0x180051e40
   18001ce3f:	4c 8b 47 18          	mov    r8,QWORD PTR [rdi+0x18]
   18001ce43:	48 8d 15 b2 6f 04 00 	lea    rdx,[rip+0x46fb2]        # 0x180063dfc
   18001ce4a:	48 8b cb             	mov    rcx,rbx
   18001ce4d:	e8 ee 4f 03 00       	call   0x180051e40
   18001ce52:	4c 8b 47 20          	mov    r8,QWORD PTR [rdi+0x20]
   18001ce56:	48 8d 15 ab 6f 04 00 	lea    rdx,[rip+0x46fab]        # 0x180063e08
   18001ce5d:	48 8b cb             	mov    rcx,rbx
   18001ce60:	e8 db 4f 03 00       	call   0x180051e40
   18001ce65:	f3 0f 10 57 28       	movss  xmm2,DWORD PTR [rdi+0x28]
   18001ce6a:	48 8d 15 a7 6f 04 00 	lea    rdx,[rip+0x46fa7]        # 0x180063e18
   18001ce71:	48 8b cb             	mov    rcx,rbx
   18001ce74:	e8 27 50 03 00       	call   0x180051ea0
   18001ce79:	f3 0f 10 57 2c       	movss  xmm2,DWORD PTR [rdi+0x2c]
   18001ce7e:	48 8d 15 a3 6f 04 00 	lea    rdx,[rip+0x46fa3]        # 0x180063e28
   18001ce85:	48 8b cb             	mov    rcx,rbx
   18001ce88:	e8 13 50 03 00       	call   0x180051ea0
   18001ce8d:	f3 0f 10 57 10       	movss  xmm2,DWORD PTR [rdi+0x10]
   18001ce92:	48 8d 15 07 6f 04 00 	lea    rdx,[rip+0x46f07]        # 0x180063da0
   18001ce99:	48 8b cb             	mov    rcx,rbx
   18001ce9c:	e8 ff 4f 03 00       	call   0x180051ea0
   18001cea1:	44 8b 47 38          	mov    r8d,DWORD PTR [rdi+0x38]
   18001cea5:	48 8d 15 8c 6f 04 00 	lea    rdx,[rip+0x46f8c]        # 0x180063e38
   18001ceac:	48 8b cb             	mov    rcx,rbx
   18001ceaf:	e8 3c 50 03 00       	call   0x180051ef0
   18001ceb4:	f3 0f 10 57 3c       	movss  xmm2,DWORD PTR [rdi+0x3c]
   18001ceb9:	0f 57 ff             	xorps  xmm7,xmm7
   18001cebc:	0f 2e d7             	ucomiss xmm2,xmm7
   18001cebf:	f3 0f 10 35 f1 9f 04 	movss  xmm6,DWORD PTR [rip+0x49ff1]        # 0x180066eb8
   18001cec6:	00 
   18001cec7:	7a 05                	jp     0x18001cece
   18001cec9:	75 03                	jne    0x18001cece
   18001cecb:	0f 28 d6             	movaps xmm2,xmm6
   18001cece:	48 8d 15 6b 6f 04 00 	lea    rdx,[rip+0x46f6b]        # 0x180063e40
   18001ced5:	48 8b cb             	mov    rcx,rbx
   18001ced8:	e8 c3 4f 03 00       	call   0x180051ea0
   18001cedd:	f3 0f 10 57 40       	movss  xmm2,DWORD PTR [rdi+0x40]
   18001cee2:	0f 2e d7             	ucomiss xmm2,xmm7
   18001cee5:	7a 05                	jp     0x18001ceec
   18001cee7:	75 03                	jne    0x18001ceec
   18001cee9:	0f 28 d6             	movaps xmm2,xmm6
   18001ceec:	48 8d 15 5d 6f 04 00 	lea    rdx,[rip+0x46f5d]        # 0x180063e50
   18001cef3:	48 8b cb             	mov    rcx,rbx
   18001cef6:	e8 a5 4f 03 00       	call   0x180051ea0
   18001cefb:	4c 8b 47 48          	mov    r8,QWORD PTR [rdi+0x48]
   18001ceff:	48 8d 15 5a 6f 04 00 	lea    rdx,[rip+0x46f5a]        # 0x180063e60
   18001cf06:	48 8b cb             	mov    rcx,rbx
   18001cf09:	e8 32 4f 03 00       	call   0x180051e40
   18001cf0e:	4c 8b 47 50          	mov    r8,QWORD PTR [rdi+0x50]
   18001cf12:	48 8d 15 5f 6f 04 00 	lea    rdx,[rip+0x46f5f]        # 0x180063e78
   18001cf19:	48 8b cb             	mov    rcx,rbx
   18001cf1c:	e8 1f 4f 03 00       	call   0x180051e40
   18001cf21:	4c 8b 47 58          	mov    r8,QWORD PTR [rdi+0x58]
   18001cf25:	48 8d 15 5c 6f 04 00 	lea    rdx,[rip+0x46f5c]        # 0x180063e88
   18001cf2c:	48 8b cb             	mov    rcx,rbx
   18001cf2f:	e8 0c 4f 03 00       	call   0x180051e40
   18001cf34:	4c 8b 87 a0 00 00 00 	mov    r8,QWORD PTR [rdi+0xa0]
   18001cf3b:	48 8d 15 6e 6f 04 00 	lea    rdx,[rip+0x46f6e]        # 0x180063eb0
   18001cf42:	48 8b cb             	mov    rcx,rbx
   18001cf45:	e8 f6 4e 03 00       	call   0x180051e40
   18001cf4a:	4c 8b 87 a8 00 00 00 	mov    r8,QWORD PTR [rdi+0xa8]
   18001cf51:	48 8d 15 68 6f 04 00 	lea    rdx,[rip+0x46f68]        # 0x180063ec0
   18001cf58:	48 8b cb             	mov    rcx,rbx
   18001cf5b:	e8 e0 4e 03 00       	call   0x180051e40
   18001cf60:	4c 8b 87 b0 00 00 00 	mov    r8,QWORD PTR [rdi+0xb0]
   18001cf67:	48 8d 15 6a 6f 04 00 	lea    rdx,[rip+0x46f6a]        # 0x180063ed8
   18001cf6e:	48 8b cb             	mov    rcx,rbx
   18001cf71:	e8 ca 4e 03 00       	call   0x180051e40
   18001cf76:	4c 8b 87 b8 00 00 00 	mov    r8,QWORD PTR [rdi+0xb8]
   18001cf7d:	48 8d 15 6c 6f 04 00 	lea    rdx,[rip+0x46f6c]        # 0x180063ef0
   18001cf84:	48 8b cb             	mov    rcx,rbx
   18001cf87:	e8 b4 4e 03 00       	call   0x180051e40
   18001cf8c:	4c 8b 87 c0 00 00 00 	mov    r8,QWORD PTR [rdi+0xc0]
   18001cf93:	48 8d 15 6e 6f 04 00 	lea    rdx,[rip+0x46f6e]        # 0x180063f08
   18001cf9a:	48 8b cb             	mov    rcx,rbx
   18001cf9d:	e8 9e 4e 03 00       	call   0x180051e40
   18001cfa2:	4c 8b 87 c8 00 00 00 	mov    r8,QWORD PTR [rdi+0xc8]
   18001cfa9:	48 8d 15 70 6f 04 00 	lea    rdx,[rip+0x46f70]        # 0x180063f20
   18001cfb0:	48 8b cb             	mov    rcx,rbx
   18001cfb3:	e8 88 4e 03 00       	call   0x180051e40
   18001cfb8:	4c 8b 87 d0 00 00 00 	mov    r8,QWORD PTR [rdi+0xd0]
   18001cfbf:	48 8d 15 6a 6f 04 00 	lea    rdx,[rip+0x46f6a]        # 0x180063f30
   18001cfc6:	48 8b cb             	mov    rcx,rbx
   18001cfc9:	e8 72 4e 03 00       	call   0x180051e40
   18001cfce:	4c 8b 87 d8 00 00 00 	mov    r8,QWORD PTR [rdi+0xd8]
   18001cfd5:	48 8d 15 6c 6f 04 00 	lea    rdx,[rip+0x46f6c]        # 0x180063f48
   18001cfdc:	48 8b cb             	mov    rcx,rbx
   18001cfdf:	e8 5c 4e 03 00       	call   0x180051e40
   18001cfe4:	4c 8b 87 e0 00 00 00 	mov    r8,QWORD PTR [rdi+0xe0]
   18001cfeb:	48 8d 15 6e 6f 04 00 	lea    rdx,[rip+0x46f6e]        # 0x180063f60
   18001cff2:	48 8b cb             	mov    rcx,rbx
   18001cff5:	e8 46 4e 03 00       	call   0x180051e40
   18001cffa:	4c 8b 87 e8 00 00 00 	mov    r8,QWORD PTR [rdi+0xe8]
   18001d001:	48 8d 15 70 6f 04 00 	lea    rdx,[rip+0x46f70]        # 0x180063f78
   18001d008:	48 8b cb             	mov    rcx,rbx
   18001d00b:	e8 30 4e 03 00       	call   0x180051e40
   18001d010:	4c 8b 87 f0 00 00 00 	mov    r8,QWORD PTR [rdi+0xf0]
   18001d017:	48 8d 15 72 6f 04 00 	lea    rdx,[rip+0x46f72]        # 0x180063f90
   18001d01e:	48 8b cb             	mov    rcx,rbx
   18001d021:	e8 1a 4e 03 00       	call   0x180051e40
   18001d026:	4c 8b 87 f8 00 00 00 	mov    r8,QWORD PTR [rdi+0xf8]
   18001d02d:	48 8d 15 74 6f 04 00 	lea    rdx,[rip+0x46f74]        # 0x180063fa8
   18001d034:	48 8b cb             	mov    rcx,rbx
   18001d037:	e8 04 4e 03 00       	call   0x180051e40
   18001d03c:	4c 8b 87 00 01 00 00 	mov    r8,QWORD PTR [rdi+0x100]
   18001d043:	48 8d 15 76 6f 04 00 	lea    rdx,[rip+0x46f76]        # 0x180063fc0
   18001d04a:	48 8b cb             	mov    rcx,rbx
   18001d04d:	e8 ee 4d 03 00       	call   0x180051e40
   18001d052:	4c 8b 87 08 01 00 00 	mov    r8,QWORD PTR [rdi+0x108]
   18001d059:	48 8d 15 78 6f 04 00 	lea    rdx,[rip+0x46f78]        # 0x180063fd8
   18001d060:	48 8b cb             	mov    rcx,rbx
   18001d063:	e8 d8 4d 03 00       	call   0x180051e40
   18001d068:	4c 8b 87 10 01 00 00 	mov    r8,QWORD PTR [rdi+0x110]
   18001d06f:	48 8d 15 7a 6f 04 00 	lea    rdx,[rip+0x46f7a]        # 0x180063ff0
   18001d076:	48 8b cb             	mov    rcx,rbx
   18001d079:	e8 c2 4d 03 00       	call   0x180051e40
   18001d07e:	4c 8b 87 18 01 00 00 	mov    r8,QWORD PTR [rdi+0x118]
   18001d085:	48 8d 15 7c 6f 04 00 	lea    rdx,[rip+0x46f7c]        # 0x180064008
   18001d08c:	48 8b cb             	mov    rcx,rbx
   18001d08f:	e8 ac 4d 03 00       	call   0x180051e40
   18001d094:	44 8b 87 20 01 00 00 	mov    r8d,DWORD PTR [rdi+0x120]
   18001d09b:	48 8d 15 7e 6f 04 00 	lea    rdx,[rip+0x46f7e]        # 0x180064020
   18001d0a2:	48 8b cb             	mov    rcx,rbx
   18001d0a5:	e8 a6 4e 03 00       	call   0x180051f50
   18001d0aa:	4c 8b 87 28 01 00 00 	mov    r8,QWORD PTR [rdi+0x128]
   18001d0b1:	48 8d 15 78 6f 04 00 	lea    rdx,[rip+0x46f78]        # 0x180064030
   18001d0b8:	48 8b cb             	mov    rcx,rbx
   18001d0bb:	e8 80 4d 03 00       	call   0x180051e40
   18001d0c0:	4c 8b 87 30 01 00 00 	mov    r8,QWORD PTR [rdi+0x130]
   18001d0c7:	48 8d 15 72 6f 04 00 	lea    rdx,[rip+0x46f72]        # 0x180064040
   18001d0ce:	48 8b cb             	mov    rcx,rbx
   18001d0d1:	e8 6a 4d 03 00       	call   0x180051e40
   18001d0d6:	4c 8b 87 38 01 00 00 	mov    r8,QWORD PTR [rdi+0x138]
   18001d0dd:	48 8d 15 6c 6f 04 00 	lea    rdx,[rip+0x46f6c]        # 0x180064050
   18001d0e4:	48 8b cb             	mov    rcx,rbx
   18001d0e7:	e8 54 4d 03 00       	call   0x180051e40
   18001d0ec:	4c 8b 87 40 01 00 00 	mov    r8,QWORD PTR [rdi+0x140]
   18001d0f3:	48 8d 15 6e 6f 04 00 	lea    rdx,[rip+0x46f6e]        # 0x180064068
   18001d0fa:	48 8b cb             	mov    rcx,rbx
   18001d0fd:	e8 3e 4d 03 00       	call   0x180051e40
   18001d102:	4c 8b 87 48 01 00 00 	mov    r8,QWORD PTR [rdi+0x148]
   18001d109:	48 8d 15 68 6f 04 00 	lea    rdx,[rip+0x46f68]        # 0x180064078
   18001d110:	48 8b cb             	mov    rcx,rbx
   18001d113:	e8 28 4d 03 00       	call   0x180051e40
   18001d118:	f3 0f 10 97 50 01 00 	movss  xmm2,DWORD PTR [rdi+0x150]
   18001d11f:	00 
   18001d120:	48 8d 15 69 6f 04 00 	lea    rdx,[rip+0x46f69]        # 0x180064090
   18001d127:	48 8b cb             	mov    rcx,rbx
   18001d12a:	e8 71 4d 03 00       	call   0x180051ea0
   18001d12f:	4c 8b 87 58 01 00 00 	mov    r8,QWORD PTR [rdi+0x158]
   18001d136:	48 8d 15 6b 6f 04 00 	lea    rdx,[rip+0x46f6b]        # 0x1800640a8
   18001d13d:	48 8b cb             	mov    rcx,rbx
   18001d140:	e8 fb 4c 03 00       	call   0x180051e40
   18001d145:	4c 8b 87 60 01 00 00 	mov    r8,QWORD PTR [rdi+0x160]
   18001d14c:	48 8d 15 6d 6f 04 00 	lea    rdx,[rip+0x46f6d]        # 0x1800640c0
   18001d153:	48 8b cb             	mov    rcx,rbx
   18001d156:	e8 e5 4c 03 00       	call   0x180051e40
   18001d15b:	44 8b 47 60          	mov    r8d,DWORD PTR [rdi+0x60]
   18001d15f:	48 8d 15 72 6f 04 00 	lea    rdx,[rip+0x46f72]        # 0x1800640d8
   18001d166:	48 8b cb             	mov    rcx,rbx
   18001d169:	e8 e2 4d 03 00       	call   0x180051f50
   18001d16e:	44 8b 47 64          	mov    r8d,DWORD PTR [rdi+0x64]
   18001d172:	48 8d 15 7f 6f 04 00 	lea    rdx,[rip+0x46f7f]        # 0x1800640f8
   18001d179:	48 8b cb             	mov    rcx,rbx
   18001d17c:	e8 cf 4d 03 00       	call   0x180051f50
   18001d181:	44 8b 47 68          	mov    r8d,DWORD PTR [rdi+0x68]
   18001d185:	48 8d 15 8c 6f 04 00 	lea    rdx,[rip+0x46f8c]        # 0x180064118
   18001d18c:	48 8b cb             	mov    rcx,rbx
   18001d18f:	e8 bc 4d 03 00       	call   0x180051f50
   18001d194:	44 8b 47 6c          	mov    r8d,DWORD PTR [rdi+0x6c]
   18001d198:	48 8d 15 99 6f 04 00 	lea    rdx,[rip+0x46f99]        # 0x180064138
   18001d19f:	48 8b cb             	mov    rcx,rbx
   18001d1a2:	e8 a9 4d 03 00       	call   0x180051f50
   18001d1a7:	44 8b 47 70          	mov    r8d,DWORD PTR [rdi+0x70]
   18001d1ab:	48 8d 15 a6 6f 04 00 	lea    rdx,[rip+0x46fa6]        # 0x180064158
   18001d1b2:	48 8b cb             	mov    rcx,rbx
   18001d1b5:	e8 96 4d 03 00       	call   0x180051f50
   18001d1ba:	44 8b 47 74          	mov    r8d,DWORD PTR [rdi+0x74]
   18001d1be:	48 8d 15 b3 6f 04 00 	lea    rdx,[rip+0x46fb3]        # 0x180064178
   18001d1c5:	48 8b cb             	mov    rcx,rbx
   18001d1c8:	e8 83 4d 03 00       	call   0x180051f50
   18001d1cd:	44 8b 47 78          	mov    r8d,DWORD PTR [rdi+0x78]
   18001d1d1:	48 8d 15 c0 6f 04 00 	lea    rdx,[rip+0x46fc0]        # 0x180064198
   18001d1d8:	48 8b cb             	mov    rcx,rbx
   18001d1db:	e8 70 4d 03 00       	call   0x180051f50
   18001d1e0:	44 8b 47 7c          	mov    r8d,DWORD PTR [rdi+0x7c]
   18001d1e4:	48 8d 15 d5 6f 04 00 	lea    rdx,[rip+0x46fd5]        # 0x1800641c0
   18001d1eb:	48 8b cb             	mov    rcx,rbx
   18001d1ee:	e8 5d 4d 03 00       	call   0x180051f50
   18001d1f3:	44 8b 87 80 00 00 00 	mov    r8d,DWORD PTR [rdi+0x80]
   18001d1fa:	48 8d 15 e7 6f 04 00 	lea    rdx,[rip+0x46fe7]        # 0x1800641e8
   18001d201:	48 8b cb             	mov    rcx,rbx
   18001d204:	e8 47 4d 03 00       	call   0x180051f50
   18001d209:	44 8b 87 84 00 00 00 	mov    r8d,DWORD PTR [rdi+0x84]
   18001d210:	48 8d 15 01 70 04 00 	lea    rdx,[rip+0x47001]        # 0x180064218
   18001d217:	48 8b cb             	mov    rcx,rbx
   18001d21a:	e8 31 4d 03 00       	call   0x180051f50
   18001d21f:	44 8b 87 88 00 00 00 	mov    r8d,DWORD PTR [rdi+0x88]
   18001d226:	48 8d 15 1b 70 04 00 	lea    rdx,[rip+0x4701b]        # 0x180064248
   18001d22d:	48 8b cb             	mov    rcx,rbx
   18001d230:	e8 1b 4d 03 00       	call   0x180051f50
   18001d235:	44 8b 87 8c 00 00 00 	mov    r8d,DWORD PTR [rdi+0x8c]
   18001d23c:	48 8d 15 25 70 04 00 	lea    rdx,[rip+0x47025]        # 0x180064268
   18001d243:	48 8b cb             	mov    rcx,rbx
   18001d246:	e8 05 4d 03 00       	call   0x180051f50
   18001d24b:	44 8b 47 30          	mov    r8d,DWORD PTR [rdi+0x30]
   18001d24f:	48 8d 15 32 70 04 00 	lea    rdx,[rip+0x47032]        # 0x180064288
   18001d256:	48 8b cb             	mov    rcx,rbx
   18001d259:	e8 f2 4c 03 00       	call   0x180051f50
   18001d25e:	44 8b 47 34          	mov    r8d,DWORD PTR [rdi+0x34]
   18001d262:	48 8d 15 47 70 04 00 	lea    rdx,[rip+0x47047]        # 0x1800642b0
   18001d269:	48 8b cb             	mov    rcx,rbx
   18001d26c:	e8 df 4c 03 00       	call   0x180051f50
   18001d271:	f3 0f 10 97 90 00 00 	movss  xmm2,DWORD PTR [rdi+0x90]
   18001d278:	00 
   18001d279:	0f 2e d7             	ucomiss xmm2,xmm7
   18001d27c:	7a 05                	jp     0x18001d283
   18001d27e:	75 03                	jne    0x18001d283
   18001d280:	0f 28 d6             	movaps xmm2,xmm6
   18001d283:	48 8d 15 4e 70 04 00 	lea    rdx,[rip+0x4704e]        # 0x1800642d8
   18001d28a:	48 8b cb             	mov    rcx,rbx
   18001d28d:	e8 0e 4c 03 00       	call   0x180051ea0
   18001d292:	f3 0f 10 87 94 00 00 	movss  xmm0,DWORD PTR [rdi+0x94]
   18001d299:	00 
   18001d29a:	0f 2e c7             	ucomiss xmm0,xmm7
   18001d29d:	7a 02                	jp     0x18001d2a1
   18001d29f:	74 03                	je     0x18001d2a4
   18001d2a1:	0f 28 f0             	movaps xmm6,xmm0
   18001d2a4:	0f 28 d6             	movaps xmm2,xmm6
   18001d2a7:	48 8d 15 42 70 04 00 	lea    rdx,[rip+0x47042]        # 0x1800642f0
   18001d2ae:	48 8b cb             	mov    rcx,rbx
   18001d2b1:	e8 ea 4b 03 00       	call   0x180051ea0
   18001d2b6:	44 8b 87 98 00 00 00 	mov    r8d,DWORD PTR [rdi+0x98]
   18001d2bd:	48 8d 15 44 70 04 00 	lea    rdx,[rip+0x47044]        # 0x180064308
   18001d2c4:	48 8b cb             	mov    rcx,rbx
   18001d2c7:	e8 24 4c 03 00       	call   0x180051ef0
   18001d2cc:	44 8b 87 9c 00 00 00 	mov    r8d,DWORD PTR [rdi+0x9c]
   18001d2d3:	48 8d 15 4e 70 04 00 	lea    rdx,[rip+0x4704e]        # 0x180064328
   18001d2da:	48 8b cb             	mov    rcx,rbx
   18001d2dd:	e8 0e 4c 03 00       	call   0x180051ef0
   18001d2e2:	45 33 c9             	xor    r9d,r9d
   18001d2e5:	4c 8b c3             	mov    r8,rbx
   18001d2e8:	48 8b d6             	mov    rdx,rsi
   18001d2eb:	48 8b cd             	mov    rcx,rbp
   18001d2ee:	48 8b 5c 24 50       	mov    rbx,QWORD PTR [rsp+0x50]
   18001d2f3:	48 8b 6c 24 58       	mov    rbp,QWORD PTR [rsp+0x58]
   18001d2f8:	48 8b 74 24 60       	mov    rsi,QWORD PTR [rsp+0x60]
   18001d2fd:	0f 28 74 24 30       	movaps xmm6,XMMWORD PTR [rsp+0x30]
   18001d302:	0f 28 7c 24 20       	movaps xmm7,XMMWORD PTR [rsp+0x20]
   18001d307:	48 83 c4 40          	add    rsp,0x40
   18001d30b:	5f                   	pop    rdi
   18001d30c:	e9 0f 57 03 00       	jmp    0x180052a20
