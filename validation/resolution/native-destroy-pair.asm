
/workspace/scratch/65fb49337344/upload/d3d_rmdwin10_f(4).dll:     file format pei-x86-64


Disassembly of section .text:

0000000180029882 <.text+0x28882>:
   180029882:	12 cb                	adc    cl,bl
   180029884:	03 00                	add    eax,DWORD PTR [rax]
   180029886:	48 89 01             	mov    QWORD PTR [rcx],rax
   180029889:	48 8b 59 08          	mov    rbx,QWORD PTR [rcx+0x8]
   18002988d:	48 85 db             	test   rbx,rbx
   180029890:	74 13                	je     0x1800298a5
   180029892:	48 8b cb             	mov    rcx,rbx
   180029895:	e8 46 0b 01 00       	call   0x18003a3e0
   18002989a:	90                   	nop
   18002989b:	48 8b cb             	mov    rcx,rbx
   18002989e:	ff 15 dc 3f 03 00    	call   QWORD PTR [rip+0x33fdc]        # 0x18005d880
   1800298a4:	90                   	nop
   1800298a5:	48 83 c4 30          	add    rsp,0x30
   1800298a9:	5b                   	pop    rbx
