renderer_rmdwin10_f.dll SHA256 ebfb5b47cebc2d4482e912b8090bd343f717bab8a178b5a6385dec9105e7c433
0012c621 lea rcx, [rip + 0x7e23d0]
0012c628 call 0x133a0
0012c62d mov rcx, rax
0012c630 call qword ptr [rip + 0x4b36fa]
0012c636 mov rcx, rax
0012c639 call qword ptr [rip + 0x4b3931]
0012c63f mov eax, dword ptr [rsi + rdi]
0012c642 cmp dword ptr [rip + 0x116ad88], eax
0012c648 jle 0x12c68b
0012c64a lea rcx, [rip + 0x116ad7f]
0012c651 call 0x587e64
0012c656 cmp dword ptr [rip + 0x116ad73], -1
0012c65d jne 0x12c68b
0012c65f lea rdx, [rip + 0x503d22]
0012c666 lea rcx, [rip + 0x116ad6b]
0012c66d call 0x130f0
0012c672 lea rcx, [rip + 0x4a1d07]
0012c679 call 0x587bc4
0012c67e nop 
0012c67f lea rcx, [rip + 0x116ad4a]
0012c686 call 0x587e04
0012c68b lea rcx, [rip + 0x7e2366]
0012c692 call 0x133a0
0012c697 test rax, rax
0012c69a je 0x12c6aa
0012c69c mov rcx, rax
0012c69f call qword ptr [rip + 0x4b368b]
0012c6a5 mov rbx, rax
0012c6a8 jmp 0x12c6ac
0012c6aa xor ebx, ebx
0012c6ac lea rcx, [rip + 0x116ad25]
0012c6b3 call 0x10d50
0012c6b8 mov rcx, rax
0012c6bb call 0x10c90
0012c6c0 mov ecx, eax
0012c6c2 call qword ptr [rip + 0x4b35c0]
0012c6c8 mov edx, eax
0012c6ca mov r8, rbx
0012c6cd lea rcx, [rbp + 0x150]
0012c6d4 call qword ptr [rip + 0x4b35a6]
0012c6da lea rcx, [rip + 0x116acf7]
0012c6e1 call 0x10d50
0012c6e6 mov rcx, rax
0012c6e9 call 0x10c90
0012c6ee mov ecx, eax
0012c6f0 xor r8d, r8d
0012c6f3 lea rdx, [rbp + 0x150]
0012c6fa call qword ptr [rip + 0x4b3890]
0012c700 lea rcx, [rip + 0x116acd1]
0012c707 call 0x10d50
0012c70c mov rcx, rax
0012c70f call 0x10c90
0012c714 mov ecx, eax
0012c716 call qword ptr [rip + 0x4b356c]
0012c71c mov edx, eax
0012c71e mov r8, rbx
0012c721 lea rcx, [rip + 0x116acc0]
0012c728 call qword ptr [rip + 0x4b3562]
0012c72e test r12b, r12b
0012c731 jne 0x12c930
0012c737 test r14b, r14b
d3d_rmdwin10_f.dll SHA256 cceb99cbd9c019af907c24c44701a53c8d230965c1fb31c325233e4c80214aa5
000416a0 mov qword ptr [rcx + 8], r8
000416a4 test edx, edx
000416a6 je 0x416ea
000416a8 cmp edx, 4
000416ab je 0x416c2
000416ad mov rax, qword ptr [rip + 0xd0504]
000416b4 mov rdx, qword ptr [rax + 0x80]
000416bb mov rax, rcx
000416be mov qword ptr [rcx], rdx
000416c1 ret 
000416c2 test r8, r8
000416c5 je 0x416d5
000416c7 mov rax, qword ptr [r8 + 0xa8]
000416ce mov qword ptr [rcx], rax
000416d1 mov rax, rcx
000416d4 ret 
000416d5 mov rax, qword ptr [rip + 0xd04dc]
000416dc mov rax, qword ptr [rax + 0x88]
000416e3 mov qword ptr [rcx], rax
000416e6 mov rax, rcx
000416e9 ret 
000416ea test r8, r8
000416ed je 0x416fd
000416ef mov rax, qword ptr [r8 + 0x90]
000416f6 mov qword ptr [rcx], rax
000416f9 mov rax, rcx
000416fc ret 
000416fd mov rax, qword ptr [rip + 0xd04b4]
00041704 mov rax, qword ptr [rax + 0x80]
0004170b mov qword ptr [rcx], rax
0004170e mov rax, rcx
00041711 ret 
00041712 int3 
00041713 int3 
00041714 int3 
00041715 int3 
00041716 int3 
00041717 int3 
00041718 int3 
00041719 int3 
0004171a int3 
0004171b int3 
0004171c int3 
0004171d int3 
0004171e int3 
0004171f int3 
0003a0c0 mov rax, rsp
0003a0c3 push rdi
0003a0c4 sub rsp, 0xb0
0003a0cb mov qword ptr [rax - 0x60], 0xfffffffffffffffe
0003a0d3 mov qword ptr [rax + 0x10], rbx
0003a0d7 mov qword ptr [rax + 0x18], rbp
0003a0db mov qword ptr [rax + 0x20], rsi
0003a0df mov rax, qword ptr [rip + 0xbcb0a]
0003a0e6 xor rax, rsp
0003a0e9 mov qword ptr [rsp + 0xa0], rax
0003a0f1 movzx ebp, r9b
0003a0f5 mov ebx, r8d
0003a0f8 mov rsi, rcx
0003a0fb call 0x2c850
0003a100 mov rdi, rax
0003a103 lea rcx, [rax + 0xa0]
0003a10a lea rdx, [rax + 0xa8]
0003a111 lea r9, [rax + 0x98]
0003a118 lea r8, [rax + 0x90]
0003a11f mov byte ptr [rsp + 0x48], bpl
0003a124 mov dword ptr [rsp + 0x40], ebx
0003a128 xor ebx, ebx
0003a12a mov dword ptr [rsp + 0x38], ebx
0003a12e mov dword ptr [rsp + 0x30], ebx
0003a132 mov qword ptr [rsp + 0x28], rcx
0003a137 mov qword ptr [rsp + 0x20], rdx
0003a13c lea rdx, [rsp + 0x50]
0003a141 mov rcx, rsi
0003a144 call 0x39b60
00039b60 push rbp
00039b62 push rbx
00039b63 push rsi
00039b64 push rdi
00039b65 push r12
00039b67 push r13
00039b69 push r14
00039b6b push r15
00039b6d lea rbp, [rsp - 7]
00039b72 sub rsp, 0xd8
00039b79 mov qword ptr [rbp - 0x49], 0xfffffffffffffffe
00039b81 mov rax, qword ptr [rip + 0xbd068]
00039b88 xor rax, rsp
00039b8b mov qword ptr [rbp - 9], rax
00039b8f mov qword ptr [rbp - 0x69], r9
00039b93 mov r13, r8
00039b96 mov qword ptr [rbp - 0x71], r8
00039b9a mov rbx, rdx
00039b9d mov r15, rcx
00039ba0 mov rax, qword ptr [rbp + 0x6f]
00039ba4 mov qword ptr [rbp - 0x61], rax
00039ba8 mov rax, qword ptr [rbp + 0x77]
00039bac mov qword ptr [rbp - 0x51], rax
00039bb0 mov rax, qword ptr [rcx]
00039bb3 lea rdx, [rbp - 0x41]
00039bb7 call qword ptr [rax + 0x50]
00039bba mov ecx, dword ptr [rbp - 0x41]
00039bbd mov dword ptr [rbx], ecx
00039bbf sub ecx, 1
00039bc2 je 0x3a099
00039bc8 sub ecx, 2
00039bcb je 0x39d42
00039bd1 cmp ecx, 1
00039bd4 jne 0x3a099
00039bda xor edi, edi
00039bdc mov qword ptr [rsp + 0x38], 0x1688
00039be5 xorps xmm0, xmm0
00039be8 movdqu xmmword ptr [rsp + 0x48], xmm0
00039bee mov dword ptr [rsp + 0x34], 8
00039bf6 mov esi, dword ptr [rbp + 0x7f]
00039bf9 mov dword ptr [rsp + 0x40], esi
00039bfd movzx eax, word ptr [rbp - 0x23]
00039c01 sub eax, esi
00039c03 mov dword ptr [rsp + 0x44], eax
00039c07 movsx rax, byte ptr [rbp + 0x97]
00039c0f lea r14, [rax + rax*4]
00039c13 lea r12, [rip + 0x28806]
00039c1a mov eax, dword ptr [r12 + r14*4]
00039c1e mov dword ptr [rsp + 0x30], eax
00039c22 test byte ptr [rbp - 0x11], 2
00039c26 je 0x39c3d
00039c28 mov eax, 0x2e
00039c2d mov ecx, 0x29
00039c32 cmp dword ptr [rbp - 0x21], 0x27
00039c36 cmove eax, ecx
00039c39 mov dword ptr [rsp + 0x30], eax
00039c3d lea rdx, [rbp - 0x79]
00039c41 mov rcx, qword ptr [rip + 0xd7f70]
00039c48 call 0x1b040
00039c4d mov rcx, qword ptr [rip + 0xd7fb4]
00039c54 mov rax, qword ptr [rcx]
00039c57 mov rbx, qword ptr [rbp - 0x79]
00039c5b mov r9, rbx
00039c5e lea r8, [rsp + 0x30]
00039c63 mov rdx, r15
00039c66 call qword ptr [rax + 0x90]
00039c6c mov qword ptr [r13], rbx
00039c70 mov eax, dword ptr [rbp - 0x11]
00039c73 test al, 1
00039c75 je 0x39ccd
00039c77 mov dword ptr [rsp + 0x44], edi
00039c7b mov eax, dword ptr [r12 + r14*4]
00039c7f mov dword ptr [rsp + 0x30], eax
00039c83 mov dword ptr [rsp + 0x34], 8
00039c8b mov dword ptr [rsp + 0x38], esi
00039c8f mov dword ptr [rsp + 0x3c], edi
00039c93 mov dword ptr [rsp + 0x40], 0xffffffff
00039c9b lea rdx, [rbp - 0x79]
00039c9f call 0x1b200
00039ca4 mov rcx, qword ptr [rip + 0xd7f5d]
00039cab mov rax, qword ptr [rcx]
00039cae mov rbx, qword ptr [rbp - 0x79]
00039cb2 mov r9, rbx
00039cb5 lea r8, [rsp + 0x30]
00039cba mov rdx, r15
00039cbd call qword ptr [rax + 0xa0]
00039cc3 mov rax, qword ptr [rbp - 0x69]
00039cc7 mov qword ptr [rax], rbx
00039cca mov eax, dword ptr [rbp - 0x11]
00039ccd test al, 4
00039ccf je 0x3a099
00039cd5 xorps xmm0, xmm0
00039cd8 movdqu xmmword ptr [rsp + 0x44], xmm0
00039cde mov dword ptr [rbp - 0x7d], edi
00039ce1 mov eax, dword ptr [r12 + r14*4 + 8]
00039ce6 mov dword ptr [rsp + 0x30], eax
00039cea mov dword ptr [rsp + 0x34], 8
00039cf2 mov dword ptr [rsp + 0x38], esi
00039cf6 mov dword ptr [rsp + 0x3c], edi
00039cfa mov dword ptr [rsp + 0x40], 0xffffffff
00039d02 lea rdx, [rbp - 0x79]
00039d06 mov rcx, qword ptr [rip + 0xd7eab]
00039d0d call 0x1b2f0
00039d12 mov rcx, qword ptr [rip + 0xd7eef]
00039d19 mov rax, qword ptr [rcx]
00039d1c mov rbx, qword ptr [rbp - 0x79]
00039d20 mov qword ptr [rsp + 0x20], rbx
00039d25 lea r9, [rsp + 0x30]
00039d2a xor r8d, r8d
00039d2d mov rdx, r15
00039d30 call qword ptr [rax + 0x98]
00039d36 mov rax, qword ptr [rbp - 0x61]
00039d3a mov qword ptr [rax], rbx
00039d3d jmp 0x3a099
00039d42 movzx edx, word ptr [rbp - 0x25]
00039d46 mov r8d, edx
00039d49 xor edi, edi
00039d4b mov qword ptr [rsp + 0x38], 0x1688
00039d54 mov qword ptr [rsp + 0x4c], rdi
00039d59 mov dword ptr [rbp - 0x7d], edi
00039d5c movsx rax, byte ptr [rbp + 0x97]
00039d64 lea rax, [rax + rax*4]
00039d68 mov qword ptr [rbp - 0x79], rax
00039d6c lea r12, [rip + 0x286ad]
00039d73 mov eax, dword ptr [r12 + rax*4]
00039d77 mov dword ptr [rsp + 0x30], eax
00039d7b test byte ptr [rbp - 0x11], 2
00039d7f je 0x39d96
00039d81 mov eax, 0x2e
00039d86 mov ecx, 0x29
00039d8b cmp dword ptr [rbp - 0x21], 0x27
00039d8f cmove eax, ecx
00039d92 mov dword ptr [rsp + 0x30], eax
00039d96 mov ecx, 6
00039d9b lea eax, [rcx - 1]
00039d9e lea r14d, [rcx - 2]
00039da2 lea r9d, [rcx + 1]
00039da6 mov r13d, dword ptr [rbp + 0x87]
00039dad mov esi, dword ptr [rbp + 0x7f]
00039db0 xorps xmm0, xmm0
00039db3 mov dword ptr [rsp + 0x40], esi
00039db7 cmp dword ptr [rbp + 0x8f], 1
00039dbe jne 0x39dfa
00039dc0 cmp dx, cx
00039dc3 jbe 0x39ded
00039dc5 lea ecx, [rax + 5]
00039dc8 mov eax, 0xaaaaaaab
00039dcd mul r8d
00039dd0 shr edx, 2
00039dd3 movzx eax, word ptr [rbp - 0x23]
00039dd7 sub eax, esi
00039dd9 mov dword ptr [rsp + 0x48], r13d
00039dde sub edx, r13d
00039de1 mov dword ptr [rsp + 0x4c], edx
00039de5 movss dword ptr [rsp + 0x50], xmm0
00039deb jmp 0x39e3c
00039ded mov ecx, 9
00039df2 movss dword ptr [rsp + 0x48], xmm0
00039df8 jmp 0x39e36
00039dfa cmp dx, 1
00039dfe jbe 0x39e24
00039e00 mov ecx, r9d
00039e03 cmp dword ptr [rbp - 0x1d], 1
00039e07 cmovbe ecx, eax
00039e0a movzx eax, word ptr [rbp - 0x23]
00039e0e sub eax, esi
00039e10 mov dword ptr [rsp + 0x48], r13d
00039e15 sub r8d, r13d
00039e18 mov dword ptr [rsp + 0x4c], r8d
00039e1d movss dword ptr [rbp - 0x7d], xmm0
00039e22 jmp 0x39e3c
00039e24 cmp dword ptr [rbp - 0x1d], 1
00039e28 cmovbe ecx, r14d
00039e2c mov dword ptr [rsp + 0x48], edi
00039e30 movss dword ptr [rsp + 0x4c], xmm0
00039e36 movzx eax, word ptr [rbp - 0x23]
00039e3a sub eax, esi
00039e3c mov dword ptr [rsp + 0x44], eax
00039e40 mov dword ptr [rsp + 0x34], ecx
00039e44 lea rdx, [rbp - 0x59]
00039e48 mov rcx, qword ptr [rip + 0xd7d69]
00039e4f call 0x1b040
00039e54 mov rcx, qword ptr [rip + 0xd7dad]
00039e5b mov rax, qword ptr [rcx]
00039e5e mov rbx, qword ptr [rbp - 0x59]
00039e62 mov r9, rbx
00039e65 lea r8, [rsp + 0x30]
00039e6a mov rdx, r15
00039e6d call qword ptr [rax + 0x90]
00039e73 mov rax, qword ptr [rbp - 0x71]
00039e77 mov qword ptr [rax], rbx
00039e7a mov eax, dword ptr [rbp - 0x11]
00039e7d test al, 1
00039e7f je 0x39efd
00039e81 movzx edx, word ptr [rbp - 0x25]
00039e85 cmp dx, 1
00039e89 jbe 0x39e95
00039e8b mov ecx, 5
00039e90 lea eax, [rcx + 2]
00039e93 jmp 0x39e9d
00039e95 mov ecx, r14d
00039e98 mov eax, 6
00039e9d cmp dword ptr [rbp - 0x1d], 1
00039ea1 cmova ecx, eax
00039ea4 mov dword ptr [rsp + 0x44], edi
00039ea8 mov rax, qword ptr [rbp - 0x79]
00039eac mov eax, dword ptr [r12 + rax*4]
00039eb0 mov dword ptr [rsp + 0x30], eax
00039eb4 mov dword ptr [rsp + 0x34], ecx
00039eb8 movzx eax, dx
00039ebb sub eax, r13d
00039ebe mov dword ptr [rsp + 0x40], eax
00039ec2 mov dword ptr [rsp + 0x3c], r13d
00039ec7 mov dword ptr [rsp + 0x38], esi
00039ecb lea rdx, [rbp - 0x71]
00039ecf call 0x1b200
00039ed4 mov rcx, qword ptr [rip + 0xd7d2d]
00039edb mov rax, qword ptr [rcx]
00039ede mov rbx, qword ptr [rbp - 0x71]
00039ee2 mov r9, rbx
00039ee5 lea r8, [rsp + 0x30]
00039eea mov rdx, r15
00039eed call qword ptr [rax + 0xa0]
00039ef3 mov rax, qword ptr [rbp - 0x69]
00039ef7 mov qword ptr [rax], rbx
00039efa mov eax, dword ptr [rbp - 0x11]
00039efd mov r13d, 5
00039f03 test r14b, al
00039f06 je 0x39f86
00039f08 xorps xmm0, xmm0
00039f0b movdqu xmmword ptr [rsp + 0x40], xmm0
00039f11 mov qword ptr [rsp + 0x50], rdi
00039f16 movzx eax, word ptr [rbp - 0x25]
00039f1a mov dword ptr [rsp + 0x3c], edi
00039f1e mov dword ptr [rsp + 0x38], esi
00039f22 cmp ax, 1
00039f26 jbe 0x39f36
00039f28 mov dword ptr [rsp + 0x34], r13d
00039f2d movzx eax, ax
00039f30 mov dword ptr [rsp + 0x40], eax
00039f34 jmp 0x39f3b
00039f36 mov dword ptr [rsp + 0x34], r14d
00039f3b mov rax, qword ptr [rbp - 0x79]
00039f3f mov eax, dword ptr [r12 + rax*4 + 8]
00039f44 mov dword ptr [rsp + 0x30], eax
00039f48 lea rdx, [rbp - 0x71]
00039f4c mov rcx, qword ptr [rip + 0xd7c65]
00039f53 call 0x1b2f0
00039f58 mov rcx, qword ptr [rip + 0xd7ca9]
00039f5f mov rax, qword ptr [rcx]
00039f62 mov rbx, qword ptr [rbp - 0x71]
00039f66 mov qword ptr [rsp + 0x20], rbx
00039f6b lea r9, [rsp + 0x30]
00039f70 xor r8d, r8d
00039f73 mov rdx, r15
00039f76 call qword ptr [rax + 0x98]
00039f7c mov rax, qword ptr [rbp - 0x61]
00039f80 mov qword ptr [rax], rbx
