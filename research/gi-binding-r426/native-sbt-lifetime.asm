RVA 00026360, bytes 594, sha256 b41f762f6590f3f8a7a373028e421ee8b0805aa4cd406f1d07fcd0902e29ddcf
00026360 mov qword ptr [rsp + 8], rbx
00026365 push rbp
00026366 push rsi
00026367 push rdi
00026368 push r12
0002636a push r13
0002636c push r14
0002636e push r15
00026370 lea rbp, [rsp - 0x27]
00026375 sub rsp, 0xe0
0002637c mov rax, qword ptr [rip + 0xd086d]
00026383 xor rax, rsp
00026386 mov qword ptr [rbp + 0x1f], rax
0002638a add ecx, 0xff
00026390 movsxd rdi, edx
00026393 and ecx, 0xffffff00
00026399 xor r12d, r12d
0002639c mov r15d, r9d
0002639f mov rsi, r8
000263a2 cmp edi, 2
000263a5 jbe 0x263af
000263a7 mov eax, r12d
000263aa cmp edi, 5
000263ad jne 0x263b4
000263af mov eax, 4
000263b4 mov edx, 1
000263b9 mov dword ptr [rbp - 0x41], eax
000263bc mov qword ptr [rbp - 0x55], 0x10001
000263c4 lea r13, [rip - 0x263cb]
000263cb mov qword ptr [rbp - 0x2d], r12
000263cf cmp edi, 6
000263d2 mov dword ptr [rbp - 0x25], edx
000263d5 mov eax, 3
000263da mov dword ptr [rbp - 0x71], edx
000263dd cmovne eax, edx
000263e0 mov dword ptr [rbp - 0x31], eax
000263e3 mov ebx, r12d
000263e6 mov qword ptr [rbp - 0x69], r12
000263ea movups xmm0, xmmword ptr [rbp - 0x71]
000263ee movsxd r14, ecx
000263f1 mov qword ptr [rbp - 0x4d], rdx
000263f5 movups xmmword ptr [rbp - 0x19], xmm0
000263f9 mov dword ptr [rbp - 0x45], edx
000263fc movups xmm0, xmmword ptr [rbp - 0x51]
00026400 mov qword ptr [rbp - 0x61], r14
00026404 mov dword ptr [rbp - 0x59], edx
00026407 movups xmm1, xmmword ptr [rbp - 0x61]
0002640b mov dword ptr [rbp - 0x21], edx
0002640e movups xmmword ptr [rbp + 7], xmm0
00026412 movups xmm0, xmmword ptr [rbp - 0x31]
00026416 movups xmmword ptr [rbp - 9], xmm1
0002641a movsd xmm1, qword ptr [rbp - 0x41]
0002641f movups xmmword ptr [rbp - 0x31], xmm0
00026423 movsd qword ptr [rbp + 0x17], xmm1
00026428 ja 0x26451
0002642a mov ecx, dword ptr [r13 + rdi*4 + 0x265b4]
00026432 add rcx, r13
00026435 jmp rcx
00026437 mov ebx, 0x400000
0002643c jmp 0x26451
0002643e mov ebx, 8
00026443 jmp 0x26451
00026445 mov ebx, 0x40
0002644a jmp 0x26451
0002644c mov ebx, 0x400
00026451 mov rcx, qword ptr [rip + 0xeb7b0]
00026458 lea rdx, [rbp - 0x79]
0002645c mov qword ptr [rsp + 0x38], rdx
00026461 lea r9, [rbp - 0x19]
00026465 lea rdx, [rip + 0x3c794]
0002646c mov qword ptr [rbp - 0x79], r12
00026470 mov qword ptr [rsp + 0x30], rdx
00026475 xor r8d, r8d
00026478 mov rax, qword ptr [rcx]
0002647b lea rdx, [rbp - 0x31]
0002647f mov qword ptr [rsp + 0x28], r12
00026484 mov dword ptr [rsp + 0x20], ebx
00026488 call qword ptr [rax + 0xd8]
0002648e cmp eax, 0x8007000e
00026493 je 0x264a0
00026495 lea ecx, [rax + 0x7785fffb]
0002649b cmp ecx, 2
0002649e ja 0x264ae
000264a0 mov edx, eax
000264a2 lea rcx, [rip + 0x3c6f7]
000264a9 call 0x23660
000264ae cmp edi, 6
000264b1 ja 0x264ff
000264b3 mov ecx, dword ptr [r13 + rdi*4 + 0x265d0]
000264bb add rcx, r13
000264be jmp rcx
000264c0 lea rdx, [rip + 0x3f501]
000264c7 jmp 0x26506
000264c9 lea rdx, [rip + 0x3f518]
000264d0 jmp 0x26506
000264d2 lea rdx, [rip + 0x3f52f]
000264d9 jmp 0x26506
000264db lea rdx, [rip + 0x3f54e]
000264e2 jmp 0x26506
000264e4 lea rdx, [rip + 0x3f575]
000264eb jmp 0x26506
000264ed lea rdx, [rip + 0x3f59c]
000264f4 jmp 0x26506
000264f6 lea rdx, [rip + 0x3f5c3]
000264fd jmp 0x26506
000264ff lea rdx, [rip + 0x3f5e2]
00026506 mov rcx, qword ptr [rbp - 0x79]
0002650a mov rax, qword ptr [rcx]
0002650d call qword ptr [rax + 0x30]
00026510 mov rcx, qword ptr [rbp - 0x79]
00026514 mov dword ptr [rsi + 0x20], ebx
00026517 mov qword ptr [rsi], rcx
0002651a test rcx, rcx
0002651d je 0x26527
0002651f mov rax, qword ptr [rcx]
00026522 call qword ptr [rax + 0x58]
00026525 jmp 0x2652a
00026527 mov rax, r12
0002652a mov qword ptr [rsi + 0x48], rax
0002652e mov qword ptr [rsi + 0x30], r14
00026532 mov dword ptr [rsi + 0x38], r15d
00026536 cmp edi, 1
00026539 jne 0x2658b
0002653b mov rcx, qword ptr [rip + 0xeb676]
00026542 lea rdx, [rbp - 0x39]
00026546 xorps xmm0, xmm0
00026549 mov qword ptr [rbp - 0x69], 0x1688
00026551 movdqu xmmword ptr [rbp - 0x59], xmm0
00026556 mov dword ptr [rbp - 0x71], r12d
0002655a mov dword ptr [rbp - 0x6d], 0xb
00026561 mov qword ptr [rbp - 0x61], rax
00026565 call 0x1b040
0002656a mov rcx, qword ptr [rip + 0xeb697]
00026571 lea r8, [rbp - 0x71]
00026575 mov rbx, qword ptr [rbp - 0x39]
00026579 xor edx, edx
0002657b mov r9, rbx
0002657e mov rax, qword ptr [rcx]
00026581 call qword ptr [rax + 0x90]
00026587 mov qword ptr [rsi + 0x10], rbx
0002658b mov rcx, qword ptr [rbp + 0x1f]
0002658f xor rcx, rsp
00026592 call 0x57070
00026597 mov rbx, qword ptr [rsp + 0x120]
0002659f add rsp, 0xe0
000265a6 pop r15
000265a8 pop r14
000265aa pop r13
000265ac pop r12
000265ae pop rdi
000265af pop rsi
000265b0 pop rbp
000265b1 ret 
RVA 00033a40, bytes 375, sha256 f3622edc689f6b696c6162db488e6af43f8358336faf15419264d74128ef714b
00033a40 push rbp
00033a42 push rsi
00033a43 push rdi
00033a44 push r12
00033a46 push r13
00033a48 push r14
00033a4a push r15
00033a4c lea rbp, [rsp - 0x180]
00033a54 sub rsp, 0x280
00033a5b mov qword ptr [rbp + 0x28], 0xfffffffffffffffe
00033a63 mov qword ptr [rsp + 0x2c8], rbx
00033a6b mov rax, qword ptr [rip + 0xc317e]
00033a72 xor rax, rsp
00033a75 mov qword ptr [rbp + 0x170], rax
00033a7c movsxd r14, r9d
00033a7f mov r15, r8
00033a82 mov qword ptr [rbp - 0x18], r8
00033a86 mov rdi, rdx
00033a89 mov qword ptr [rbp - 0x20], rdx
00033a8d mov dword ptr [rbp - 0x28], ecx
00033a90 mov rax, qword ptr [rbp + 0x1f0]
00033a97 mov qword ptr [rbp - 0x38], rax
00033a9b mov edx, 8
00033aa0 mov r8d, 0x10
00033aa6 xor r13d, r13d
00033aa9 mov qword ptr [rbp + 0x150], r13
00033ab0 test ecx, ecx
00033ab2 js 0x33ada
00033ab4 mov dword ptr [rbp + 0x158], r13d
00033abb mov rax, qword ptr [rip + 0xde166]
00033ac2 mov qword ptr [rbp + 0x160], rax
00033ac9 lea rcx, [rbp + 0x150]
00033ad0 call 0x42cc0
00033ad5 jmp 0x33bb7
00033ada mov rax, qword ptr [rdi]
00033add mov qword ptr [rbp + 0x168], 0x400
00033ae8 mov qword ptr [rbp + 0x158], rax
00033aef mov dword ptr [rbp + 0x164], 0x40
00033af9 mov dword ptr [rbp + 0x160], 0xffffffff
00033b03 movups xmm0, xmmword ptr [rbp + 0x150]
00033b0a movups xmmword ptr [rbp - 0x10], xmm0
00033b0e movups xmm1, xmmword ptr [rbp + 0x160]
00033b15 movups xmmword ptr [rbp], xmm1
00033b19 mov ecx, dword ptr [rip + 0xddadd]
00033b1f mov rax, qword ptr gs:[0x58]
00033b28 mov rbx, qword ptr [rax + rcx*8]
00033b2c mov esi, edx
00033b2e add rsi, rbx
00033b31 mov rdx, qword ptr [rsi]
00033b34 test rdx, rdx
00033b37 je 0x33b3f
00033b39 mov rbx, qword ptr [r8 + rbx]
00033b3d jmp 0x33b46
00033b3f mov rbx, qword ptr [rip + 0xde0da]
00033b46 mov qword ptr [rsp + 0x38], rbx
00033b4b test rbx, rbx
00033b4e je 0x33b73
00033b50 call qword ptr [rip + 0x2965a]
00033b56 mov edi, eax
00033b58 cmp dword ptr [rbx + 8], eax
00033b5b je 0x33b69
00033b5d mov rcx, rbx
00033b60 call qword ptr [rip + 0x29652]
00033b66 mov dword ptr [rbx + 8], edi
00033b69 inc dword ptr [rbx + 0xc]
00033b6c mov rdx, qword ptr [rsi]
00033b6f mov rdi, qword ptr [rbp - 0x20]
00033b73 mov rcx, qword ptr [rip + 0xde09e]
00033b7a test rdx, rdx
00033b7d cmovne rcx, rdx
00033b81 inc qword ptr [rcx + 8]
00033b85 mov rcx, qword ptr [rcx + 0x18]
00033b89 mov rax, qword ptr [rcx]
00033b8c lea r8, [rbp - 0x10]
00033b90 mov edx, 1
00033b95 call qword ptr [rax + 0xd0]
00033b9b nop 
00033b9c test rbx, rbx
00033b9f je 0x33bb7
00033ba1 add dword ptr [rbx + 0xc], -1
00033ba5 jne 0x33bb7
00033ba7 mov dword ptr [rbx + 8], 0xffffffff
00033bae mov rcx, rbx
00033bb1 call qword ptr [rip + 0x295f1]
RVA 000343f3, bytes 323, sha256 c3241cb8b43de931d6a33874ebc26024878f8a6cacefdb510d16211b7661f6de
000343f3 mov qword ptr [rsp + 0x40], r13
000343f8 movsxd rbx, dword ptr [rbp - 0x28]
000343fc test ebx, ebx
000343fe js 0x3441c
00034400 call 0x42ee0
00034405 mov rcx, rbx
00034408 shl rcx, 6
0003440c lea rdx, [rip + 0x102955]
00034413 mov qword ptr [rcx + rdx], rax
00034417 jmp 0x3450c
0003441c mov rax, qword ptr [rbp - 0x20]
00034420 mov rax, qword ptr [rax]
00034423 mov qword ptr [rbp + 0x168], 0x40
0003442e mov qword ptr [rbp + 0x150], 0
00034439 mov qword ptr [rbp + 0x158], rax
00034440 mov dword ptr [rbp + 0x164], 0x400
0003444a mov dword ptr [rbp + 0x160], 0xffffffff
00034454 movups xmm0, xmmword ptr [rbp + 0x150]
0003445b movups xmmword ptr [rbp - 0x10], xmm0
0003445f movups xmm1, xmmword ptr [rbp + 0x160]
00034466 movups xmmword ptr [rbp], xmm1
0003446a mov ecx, dword ptr [rip + 0xdd18c]
00034470 mov rax, qword ptr gs:[0x58]
00034479 mov rbx, qword ptr [rax + rcx*8]
0003447d mov esi, 8
00034482 add rsi, rbx
00034485 mov rdx, qword ptr [rsi]
00034488 test rdx, rdx
0003448b je 0x34498
0003448d mov eax, 0x10
00034492 mov rbx, qword ptr [rax + rbx]
00034496 jmp 0x3449f
00034498 mov rbx, qword ptr [rip + 0xdd781]
0003449f mov qword ptr [rbp - 0x38], rbx
000344a3 test rbx, rbx
000344a6 je 0x344c7
000344a8 call qword ptr [rip + 0x28d02]
000344ae mov edi, eax
000344b0 cmp dword ptr [rbx + 8], eax
000344b3 je 0x344c1
000344b5 mov rcx, rbx
000344b8 call qword ptr [rip + 0x28cfa]
000344be mov dword ptr [rbx + 8], edi
000344c1 inc dword ptr [rbx + 0xc]
000344c4 mov rdx, qword ptr [rsi]
000344c7 mov rcx, qword ptr [rip + 0xdd74a]
000344ce test rdx, rdx
000344d1 cmovne rcx, rdx
000344d5 inc qword ptr [rcx + 8]
000344d9 mov rcx, qword ptr [rcx + 0x18]
000344dd mov rax, qword ptr [rcx]
000344e0 lea r8, [rbp - 0x10]
000344e4 mov edx, 1
000344e9 call qword ptr [rax + 0xd0]
000344ef nop 
000344f0 test rbx, rbx
000344f3 je 0x3450c
000344f5 add dword ptr [rbx + 0xc], -1
000344f9 jne 0x3450c
000344fb mov dword ptr [rbx + 8], 0xffffffff
00034502 mov rcx, rbx
00034505 call qword ptr [rip + 0x28c9d]
0003450b nop 
0003450c mov rcx, qword ptr [rbp + 0x170]
00034513 xor rcx, rsp
00034516 call 0x57070
0003451b mov rbx, qword ptr [rsp + 0x2c8]
00034523 add rsp, 0x280
0003452a pop r15
0003452c pop r14
0003452e pop r13
00034530 pop r12
00034532 pop rdi
00034533 pop rsi
00034534 pop rbp
00034535 ret 
RVA 00035701, bytes 133, sha256 0e84e9fe1b1ad02e1c1df471e7610bfdeca774ce3a5c6e3740ac2ec9f6ddf72c
00035701 mov rax, qword ptr [r8 + 0x220]
00035708 mov rax, qword ptr [rax + 0xa8]
0003570f inc qword ptr [rax]
00035712 mov rdx, qword ptr [r8 + 0x220]
00035719 mov rax, qword ptr [rdx + 0xa8]
00035720 mov rcx, qword ptr [rax]
00035723 and ecx, 1
00035726 lea r14, [rcx + rcx*4]
0003572a shl r14, 4
0003572e add r14, rdx
00035731 mov edi, dword ptr [r14 + 0x30]
00035735 mov eax, dword ptr [r8 + 0x228]
0003573c cmp edi, eax
0003573e jge 0x35746
00035740 add edi, edi
00035742 cmp edi, eax
00035744 jl 0x35740
00035746 movsxd rax, edi
00035749 cmp rax, qword ptr [r14 + 0x30]
0003574d je 0x35782
0003574f mov rcx, r14
00035752 call 0x25f00
00035757 xor r9d, r9d
0003575a mov r8, r14
0003575d lea edx, [r9 + 3]
00035761 mov ecx, edi
00035763 call 0x26360
00035768 mov r9d, 2
0003576e xor r8d, r8d
00035771 xor edx, edx
00035773 mov rcx, r14
00035776 call 0x26600
0003577b mov r8, qword ptr [rip + 0x1015a6]
00035782 mov rdi, qword ptr [r14 + 0x48]
RVA 00024430, bytes 202, sha256 a4d278a58905da74be5feb827349f182b8d040aa74805f8790839eb07f38cd88
00024430 test rcx, rcx
00024433 je 0x244f9
00024439 push rdi
0002443a sub rsp, 0x30
0002443e mov qword ptr [rsp + 0x20], 0xfffffffffffffffe
00024447 mov qword ptr [rsp + 0x48], rbx
0002444c mov qword ptr [rsp + 0x50], rsi
00024451 mov rdi, rcx
00024454 lea rsi, [rip + 0xdb965]
0002445b mov qword ptr [rsp + 0x40], rsi
00024460 call qword ptr [rip + 0x38d4a]
00024466 mov ebx, eax
00024468 cmp dword ptr [rip + 0xdb95a], eax
0002446e je 0x2447f
00024470 mov rcx, rsi
00024473 call qword ptr [rip + 0x38d3f]
00024479 mov dword ptr [rip + 0xdb949], ebx
0002447f inc dword ptr [rip + 0xdb947]
00024485 mov rax, qword ptr [rdi]
00024488 mov rcx, rdi
0002448b call qword ptr [rax + 8]
0002448e mov rbx, qword ptr [rip + 0xed76b]
00024495 mov edx, dword ptr [rbx + 0x10]
00024498 inc edx
0002449a mov rcx, rbx
0002449d call 0x24ea0
000244a2 mov eax, dword ptr [rbx + 0xc]
000244a5 shl rax, 4
000244a9 add rax, qword ptr [rbx]
000244ac mov qword ptr [rax], rdi
000244af lea rcx, [rip - 0x96]
000244b6 mov qword ptr [rax + 8], rcx
000244ba mov r8d, dword ptr [rbx + 0x14]
000244be mov eax, dword ptr [rbx + 0xc]
000244c1 inc eax
000244c3 xor edx, edx
000244c5 div r8
000244c8 mov dword ptr [rbx + 0xc], edx
000244cb inc dword ptr [rbx + 0x10]
000244ce add dword ptr [rip + 0xdb8f7], -1
000244d5 jne 0x244ea
000244d7 mov dword ptr [rip + 0xdb8e7], 0xffffffff
000244e1 mov rcx, rsi
000244e4 call qword ptr [rip + 0x38cbe]
000244ea mov rbx, qword ptr [rsp + 0x48]
000244ef mov rsi, qword ptr [rsp + 0x50]
000244f4 add rsp, 0x30
000244f8 pop rdi
000244f9 ret 
RVA 000245c0, bytes 477, sha256 4abfea7384ccab7dadac3107b5fd2f2fa0e1673fc7b5873393e0f9e14454308d
000245c0 push rdi
000245c2 sub rsp, 0x30
000245c6 mov qword ptr [rsp + 0x20], 0xfffffffffffffffe
000245cf mov qword ptr [rsp + 0x50], rbx
000245d4 mov qword ptr [rsp + 0x58], rsi
000245d9 lea rsi, [rip + 0xdb7e0]
000245e0 mov qword ptr [rsp + 0x40], rsi
000245e5 call qword ptr [rip + 0x38bc5]
000245eb mov ebx, eax
000245ed cmp dword ptr [rip + 0xdb7d5], eax
000245f3 je 0x24604
000245f5 mov rcx, rsi
000245f8 call qword ptr [rip + 0x38bba]
000245fe mov dword ptr [rip + 0xdb7c4], ebx
00024604 inc dword ptr [rip + 0xdb7c2]
0002460a mov rdx, qword ptr [rip + 0xed5ef]
00024611 test rdx, rdx
00024614 je 0x24684
00024616 mov rax, qword ptr [rip + 0x11270b]
0002461d mov rcx, qword ptr [rax + 0x88]
00024624 movups xmm0, xmmword ptr [rcx]
00024627 movups xmmword ptr [rdx + 0x18], xmm0
0002462b mov edx, dword ptr [rip + 0xede77]
00024631 inc edx
00024633 lea rcx, [rip + 0xede5e]
0002463a call 0x24df0
0002463f mov ecx, dword ptr [rip + 0xede5f]
00024645 mov rax, qword ptr [rip + 0xede4c]
0002464c lea rdx, [rax + rcx*8]
00024650 mov rax, qword ptr [rip + 0xed5a9]
00024657 mov qword ptr [rdx], rax
0002465a mov eax, dword ptr [rip + 0xede44]
00024660 inc eax
00024662 mov ecx, dword ptr [rip + 0xede44]
00024668 xor edx, edx
0002466a div rcx
0002466d mov dword ptr [rip + 0xede31], edx
00024673 inc dword ptr [rip + 0xede2f]
00024679 mov qword ptr [rip + 0xed57c], 0
00024684 mov rbx, qword ptr [rip + 0xeda8d]
0002468b mov qword ptr [rsp + 0x48], rbx
00024690 call qword ptr [rip + 0x38b1a]
00024696 mov edi, eax
00024698 cmp dword ptr [rbx + 8], eax
0002469b je 0x246a9
0002469d mov rcx, rbx
000246a0 call qword ptr [rip + 0x38b12]
000246a6 mov dword ptr [rbx + 8], edi
000246a9 inc dword ptr [rbx + 0xc]
000246ac mov ecx, dword ptr [rip + 0xeda7e]
000246b2 test ecx, ecx
000246b4 jne 0x24714
000246b6 call 0x24dc0
000246bb mov rdi, rax
000246be mov edx, dword ptr [rip + 0xeda6c]
000246c4 inc edx
000246c6 lea rcx, [rip + 0xeda53]
000246cd call 0x24df0
000246d2 mov edx, dword ptr [rip + 0xeda54]
000246d8 mov rcx, qword ptr [rip + 0xeda41]
000246df mov qword ptr [rcx + rdx*8], rdi
000246e3 mov eax, dword ptr [rip + 0xeda43]
000246e9 inc eax
000246eb mov r8d, dword ptr [rip + 0xeda42]
000246f2 xor edx, edx
000246f4 div r8
000246f7 mov dword ptr [rip + 0xeda2f], edx
000246fd mov ecx, dword ptr [rip + 0xeda2d]
00024703 inc ecx
00024705 mov dword ptr [rip + 0xeda25], ecx
0002470b inc qword ptr [rip + 0xeda26]
00024712 jmp 0x2471b
00024714 mov r8d, dword ptr [rip + 0xeda19]
0002471b mov r9d, r8d
0002471e mov r8d, dword ptr [rip + 0xeda03]
00024725 mov eax, r8d
00024728 xor edx, edx
0002472a div r9
0002472d mov rax, qword ptr [rip + 0xed9ec]
00024734 mov rdi, qword ptr [rax + rdx*8]
00024738 test ecx, ecx
0002473a je 0x24754
0002473c lea eax, [r8 + 1]
00024740 xor edx, edx
00024742 div r9
00024745 mov dword ptr [rip + 0xed9dd], edx
0002474b lea eax, [rcx - 1]
0002474e mov dword ptr [rip + 0xed9dc], eax
00024754 add dword ptr [rbx + 0xc], -1
00024758 jne 0x2476a
0002475a mov dword ptr [rbx + 8], 0xffffffff
00024761 mov rcx, rbx
00024764 call qword ptr [rip + 0x38a3e]
0002476a mov qword ptr [rip + 0xed48f], rdi
00024771 add dword ptr [rip + 0xdb654], -1
00024778 jne 0x2478d
0002477a mov dword ptr [rip + 0xdb644], 0xffffffff
00024784 mov rcx, rsi
00024787 call qword ptr [rip + 0x38a1b]
0002478d mov rbx, qword ptr [rsp + 0x50]
00024792 mov rsi, qword ptr [rsp + 0x58]
00024797 add rsp, 0x30
0002479b pop rdi
0002479c ret 
RVA 00024992, bytes 77, sha256 0cfd0e7f040bbc2f781c97d2efab3e010bcd0047146b31a6ad65adcd4bd0ca75
00024992 mov ecx, dword ptr [rip + 0xedb14]
00024998 mov eax, dword ptr [rip + 0xedb02]
0002499e xor edx, edx
000249a0 div rcx
000249a3 mov rax, qword ptr [rip + 0xedaee]
000249aa mov rdi, qword ptr [rax + rdx*8]
000249ae test r13b, r13b
000249b1 jne 0x249df
000249b3 movups xmm0, xmmword ptr [rdi + 0x18]
000249b7 movaps xmmword ptr [rbp - 0x29], xmm0
000249bb psrldq xmm0, 8
000249c0 movq rcx, xmm0
000249c5 test rcx, rcx
000249c8 je 0x249df
000249ca mov rax, qword ptr [rcx]
000249cd call qword ptr [rax + 0x40]
000249d0 cmp rax, qword ptr [rbp - 0x29]
000249d4 setb al
000249d7 test al, al
000249d9 jne 0x24ca7
RVA 000417b0, bytes 129, sha256 bbd6697c4626c8436db734c5870773b8fd7e847f64896feb59ef912194858e54
000417b0 sub edx, 2
000417b3 je 0x41811
000417b5 sub edx, 1
000417b8 je 0x417f1
000417ba cmp edx, 2
000417bd je 0x417d1
000417bf mov rax, qword ptr [rip + 0xd03f2]
000417c6 mov rdx, qword ptr [rax + 0x90]
000417cd mov qword ptr [rcx], rdx
000417d0 ret 
000417d1 test r8, r8
000417d4 je 0x417df
000417d6 mov rax, qword ptr [r8 + 0x18]
000417da test rax, rax
000417dd jne 0x4182d
000417df mov rax, qword ptr [rip + 0xd03d2]
000417e6 mov rax, qword ptr [rax + 0x98]
000417ed mov qword ptr [rcx], rax
000417f0 ret 
000417f1 test r8, r8
000417f4 je 0x417ff
000417f6 mov rax, qword ptr [r8 + 0x10]
000417fa test rax, rax
000417fd jne 0x4182d
000417ff mov rax, qword ptr [rip + 0xd03b2]
00041806 mov rax, qword ptr [rax + 0x90]
0004180d mov qword ptr [rcx], rax
00041810 ret 
00041811 test r8, r8
00041814 je 0x4181f
00041816 mov rax, qword ptr [r8 + 8]
0004181a test rax, rax
0004181d jne 0x4182d
0004181f mov rax, qword ptr [rip + 0xd0392]
00041826 mov rax, qword ptr [rax + 0xa0]
0004182d mov qword ptr [rcx], rax
00041830 ret 
