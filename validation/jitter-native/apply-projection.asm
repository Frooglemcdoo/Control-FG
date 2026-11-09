
upload/renderer_rmdwin10_f(4).dll:     file format pei-x86-64


Disassembly of section .text:

0000000180180940 <.text+0x17f940>:
   180180940:	48 89 5c 24 08       	mov    QWORD PTR [rsp+0x8],rbx
   180180945:	48 89 74 24 10       	mov    QWORD PTR [rsp+0x10],rsi
   18018094a:	48 89 7c 24 18       	mov    QWORD PTR [rsp+0x18],rdi
   18018094f:	4c 89 74 24 20       	mov    QWORD PTR [rsp+0x20],r14
   180180954:	55                   	push   rbp
   180180955:	48 8d 6c 24 80       	lea    rbp,[rsp-0x80]
   18018095a:	48 81 ec 80 01 00 00 	sub    rsp,0x180
   180180961:	0f 10 82 e0 00 00 00 	movups xmm0,XMMWORD PTR [rdx+0xe0]
   180180968:	48 8b da             	mov    rbx,rdx
   18018096b:	48 8b f9             	mov    rdi,rcx
   18018096e:	48 8d 4d 00          	lea    rcx,[rbp+0x0]
   180180972:	4d 8b f1             	mov    r14,r9
   180180975:	0f 29 45 00          	movaps XMMWORD PTR [rbp+0x0],xmm0
   180180979:	49 8b f0             	mov    rsi,r8
   18018097c:	0f 10 82 f0 00 00 00 	movups xmm0,XMMWORD PTR [rdx+0xf0]
   180180983:	0f 29 45 10          	movaps XMMWORD PTR [rbp+0x10],xmm0
   180180987:	0f 10 82 00 01 00 00 	movups xmm0,XMMWORD PTR [rdx+0x100]
   18018098e:	0f 29 45 20          	movaps XMMWORD PTR [rbp+0x20],xmm0
   180180992:	0f 10 82 10 01 00 00 	movups xmm0,XMMWORD PTR [rdx+0x110]
   180180999:	0f 29 45 30          	movaps XMMWORD PTR [rbp+0x30],xmm0
   18018099d:	0f 10 82 20 01 00 00 	movups xmm0,XMMWORD PTR [rdx+0x120]
   1801809a4:	0f 29 45 40          	movaps XMMWORD PTR [rbp+0x40],xmm0
   1801809a8:	0f 10 82 30 01 00 00 	movups xmm0,XMMWORD PTR [rdx+0x130]
   1801809af:	0f 29 45 50          	movaps XMMWORD PTR [rbp+0x50],xmm0
   1801809b3:	0f 10 82 40 01 00 00 	movups xmm0,XMMWORD PTR [rdx+0x140]
   1801809ba:	0f 29 45 60          	movaps XMMWORD PTR [rbp+0x60],xmm0
   1801809be:	0f 10 82 50 01 00 00 	movups xmm0,XMMWORD PTR [rdx+0x150]
   1801809c5:	49 8b d0             	mov    rdx,r8
   1801809c8:	0f 29 45 70          	movaps XMMWORD PTR [rbp+0x70],xmm0
   1801809cc:	e8 ef 77 e9 ff       	call   0x1800181c0
   1801809d1:	80 bb b0 04 00 00 00 	cmp    BYTE PTR [rbx+0x4b0],0x0
   1801809d8:	48 8d 53 20          	lea    rdx,[rbx+0x20]
   1801809dc:	0f 10 02             	movups xmm0,XMMWORD PTR [rdx]
   1801809df:	48 8b cf             	mov    rcx,rdi
   1801809e2:	0f 29 44 24 20       	movaps XMMWORD PTR [rsp+0x20],xmm0
   1801809e7:	0f 10 42 10          	movups xmm0,XMMWORD PTR [rdx+0x10]
   1801809eb:	0f 29 44 24 30       	movaps XMMWORD PTR [rsp+0x30],xmm0
   1801809f0:	0f 10 42 20          	movups xmm0,XMMWORD PTR [rdx+0x20]
   1801809f4:	0f 29 44 24 40       	movaps XMMWORD PTR [rsp+0x40],xmm0
   1801809f9:	0f 10 42 30          	movups xmm0,XMMWORD PTR [rdx+0x30]
   1801809fd:	0f 29 44 24 50       	movaps XMMWORD PTR [rsp+0x50],xmm0
   180180a02:	0f 10 42 40          	movups xmm0,XMMWORD PTR [rdx+0x40]
   180180a06:	0f 29 44 24 60       	movaps XMMWORD PTR [rsp+0x60],xmm0
   180180a0b:	0f 10 42 50          	movups xmm0,XMMWORD PTR [rdx+0x50]
   180180a0f:	0f 29 44 24 70       	movaps XMMWORD PTR [rsp+0x70],xmm0
   180180a14:	74 65                	je     0x180180a7b
   180180a16:	48 8d 93 70 03 00 00 	lea    rdx,[rbx+0x370]
   180180a1d:	e8 de 36 e9 ff       	call   0x180014100
   180180a22:	0f 10 83 f0 02 00 00 	movups xmm0,XMMWORD PTR [rbx+0x2f0]
   180180a29:	49 8b d6             	mov    rdx,r14
   180180a2c:	0f 29 45 80          	movaps XMMWORD PTR [rbp-0x80],xmm0
   180180a30:	0f 10 83 00 03 00 00 	movups xmm0,XMMWORD PTR [rbx+0x300]
   180180a37:	0f 29 45 90          	movaps XMMWORD PTR [rbp-0x70],xmm0
   180180a3b:	0f 10 83 10 03 00 00 	movups xmm0,XMMWORD PTR [rbx+0x310]
   180180a42:	0f 29 45 a0          	movaps XMMWORD PTR [rbp-0x60],xmm0
   180180a46:	0f 10 83 20 03 00 00 	movups xmm0,XMMWORD PTR [rbx+0x320]
   180180a4d:	0f 29 45 b0          	movaps XMMWORD PTR [rbp-0x50],xmm0
   180180a51:	0f 10 83 30 03 00 00 	movups xmm0,XMMWORD PTR [rbx+0x330]
   180180a58:	0f 29 45 c0          	movaps XMMWORD PTR [rbp-0x40],xmm0
   180180a5c:	0f 10 83 40 03 00 00 	movups xmm0,XMMWORD PTR [rbx+0x340]
   180180a63:	0f 29 45 d0          	movaps XMMWORD PTR [rbp-0x30],xmm0
   180180a67:	0f 10 83 50 03 00 00 	movups xmm0,XMMWORD PTR [rbx+0x350]
   180180a6e:	0f 29 45 e0          	movaps XMMWORD PTR [rbp-0x20],xmm0
   180180a72:	0f 10 83 60 03 00 00 	movups xmm0,XMMWORD PTR [rbx+0x360]
   180180a79:	eb 5c                	jmp    0x180180ad7
   180180a7b:	e8 80 36 e9 ff       	call   0x180014100
   180180a80:	0f 10 83 e0 00 00 00 	movups xmm0,XMMWORD PTR [rbx+0xe0]
   180180a87:	48 8b d6             	mov    rdx,rsi
   180180a8a:	0f 29 45 80          	movaps XMMWORD PTR [rbp-0x80],xmm0
   180180a8e:	0f 10 83 f0 00 00 00 	movups xmm0,XMMWORD PTR [rbx+0xf0]
   180180a95:	0f 29 45 90          	movaps XMMWORD PTR [rbp-0x70],xmm0
   180180a99:	0f 10 83 00 01 00 00 	movups xmm0,XMMWORD PTR [rbx+0x100]
   180180aa0:	0f 29 45 a0          	movaps XMMWORD PTR [rbp-0x60],xmm0
   180180aa4:	0f 10 83 10 01 00 00 	movups xmm0,XMMWORD PTR [rbx+0x110]
   180180aab:	0f 29 45 b0          	movaps XMMWORD PTR [rbp-0x50],xmm0
   180180aaf:	0f 10 83 20 01 00 00 	movups xmm0,XMMWORD PTR [rbx+0x120]
   180180ab6:	0f 29 45 c0          	movaps XMMWORD PTR [rbp-0x40],xmm0
   180180aba:	0f 10 83 30 01 00 00 	movups xmm0,XMMWORD PTR [rbx+0x130]
   180180ac1:	0f 29 45 d0          	movaps XMMWORD PTR [rbp-0x30],xmm0
   180180ac5:	0f 10 83 40 01 00 00 	movups xmm0,XMMWORD PTR [rbx+0x140]
   180180acc:	0f 29 45 e0          	movaps XMMWORD PTR [rbp-0x20],xmm0
   180180ad0:	0f 10 83 50 01 00 00 	movups xmm0,XMMWORD PTR [rbx+0x150]
   180180ad7:	48 8d 4d 80          	lea    rcx,[rbp-0x80]
   180180adb:	0f 29 45 f0          	movaps XMMWORD PTR [rbp-0x10],xmm0
   180180adf:	e8 dc 76 e9 ff       	call   0x1800181c0
   180180ae4:	48 8d 55 80          	lea    rdx,[rbp-0x80]
   180180ae8:	48 8b cf             	mov    rcx,rdi
   180180aeb:	e8 60 3f e9 ff       	call   0x180014a50
   180180af0:	48 8b cf             	mov    rcx,rdi
   180180af3:	e8 08 fc ff ff       	call   0x180180700
   180180af8:	0f b6 83 b0 04 00 00 	movzx  eax,BYTE PTR [rbx+0x4b0]
   180180aff:	48 8d 54 24 20       	lea    rdx,[rsp+0x20]
   180180b04:	48 8b cf             	mov    rcx,rdi
   180180b07:	88 87 b0 04 00 00    	mov    BYTE PTR [rdi+0x4b0],al
   180180b0d:	e8 ee 35 e9 ff       	call   0x180014100
   180180b12:	48 8d 55 00          	lea    rdx,[rbp+0x0]
   180180b16:	48 8b cf             	mov    rcx,rdi
   180180b19:	e8 32 3f e9 ff       	call   0x180014a50
   180180b1e:	48 8d 97 e0 00 00 00 	lea    rdx,[rdi+0xe0]
   180180b25:	48 8d 4c 24 20       	lea    rcx,[rsp+0x20]
   180180b2a:	e8 21 7f e9 ff       	call   0x180018a50
   180180b2f:	8b                   	.byte 0x8b
