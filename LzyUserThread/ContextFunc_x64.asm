.code

SaveContext Proc

test dword ptr [rcx + 1b8h], 1h
jnz back

mov [rcx + 0h], rax
mov [rcx + 8h], rbx
mov [rcx + 10h], rcx
mov [rcx + 18h], rdx
mov [rcx + 20h], rsi
mov [rcx + 28h], rdi
mov [rcx + 30h], r8
mov [rcx + 38h], r9
mov [rcx + 40h], r10
mov [rcx + 48h], r11
mov [rcx + 50h], r12
mov [rcx + 58h], r13
mov [rcx + 60h], r14
mov [rcx + 68h], r15

mov rax, rsp
add rax, 8h
mov [rcx + 70h], rax

mov rax, [rsp]
mov [rcx + 78h], rax

pushfq
pop rax
mov [rcx + 80h], rax 

movups xmmword ptr [rcx + 90h], xmm0
movups xmmword ptr [rcx + 0a0h], xmm1
movups xmmword ptr [rcx + 0b0h], xmm2
movups xmmword ptr [rcx + 0c0h], xmm3
movups xmmword ptr [rcx + 0d0h], xmm4
movups xmmword ptr [rcx + 0e0h], xmm5
movups xmmword ptr [rcx + 0f0h], xmm6
movups xmmword ptr [rcx + 100h], xmm7
movups xmmword ptr [rcx + 110h], xmm8
movups xmmword ptr [rcx + 120h], xmm9
movups xmmword ptr [rcx + 130h], xmm10
movups xmmword ptr [rcx + 140h], xmm11
movups xmmword ptr [rcx + 150h], xmm12
movups xmmword ptr [rcx + 160h], xmm13
movups xmmword ptr [rcx + 170h], xmm14
movups xmmword ptr [rcx + 180h], xmm15

mov [rcx + 1a0h], ss
mov [rcx + 1a4h], gs
mov [rcx + 1a8h], fs
mov [rcx + 1b0h], es
mov [rcx + 1b4h], ds
mov [rcx + 1b8h], cs

or dword ptr [rcx + 1b8h], 1h

xor eax, eax
xor eax, 1
ret

back:
xor eax, eax
ret

SaveContext Endp

RestoreContext Proc

test dword ptr [rcx + 1b8h], 1h
jz back

mov rsp, [rcx + 70h]

sub rsp, 10h
;mov eax, 0
;mov [rsp + 14h], eax
;mov eax, [rcx + 1b8h]
;mov [rsp + 10h], eax
mov rax, [rcx + 78h]
mov [rsp + 8h], rax
mov rax, [rcx + 80h]
mov [rsp], rax

mov rax, [rcx + 0h]
mov rbx, [rcx + 8h]
mov rdx, [rcx + 18h]
mov rsi, [rcx + 20h]
mov rdi, [rcx + 28h]
mov r8, [rcx + 30h]
mov r9, [rcx + 38h]
mov r10, [rcx + 40h]
mov r11, [rcx + 48h]
mov r12, [rcx + 50h]
mov r13, [rcx + 58h]
mov r14, [rcx + 60h]
mov r15, [rcx + 68h]

movups xmm0, xmmword ptr [rcx + 90h]
movups xmm1, xmmword ptr [rcx + 0a0h]
movups xmm2, xmmword ptr [rcx + 0b0h]
movups xmm3, xmmword ptr [rcx + 0c0h]
movups xmm4, xmmword ptr [rcx + 0d0h]
movups xmm5, xmmword ptr [rcx + 0e0h]
movups xmm6, xmmword ptr [rcx + 0f0h]
movups xmm7, xmmword ptr [rcx + 100h]
movups xmm8, xmmword ptr [rcx + 110h]
movups xmm9, xmmword ptr [rcx + 120h]
movups xmm10, xmmword ptr [rcx + 130h]
movups xmm11, xmmword ptr [rcx + 140h]
movups xmm12, xmmword ptr [rcx + 150h]
movups xmm13, xmmword ptr [rcx + 160h]
movups xmm14, xmmword ptr [rcx + 170h]
movups xmm15, xmmword ptr [rcx + 180h]

mov ss, [rcx + 1a0h]
mov gs, [rcx + 1a4h]
mov fs, [rcx + 1a8h]
mov es, [rcx + 1b0h]
mov ds, [rcx + 1b4h]

and dword ptr [rcx + 1b8h], 0fffffffeh

mov rcx, [rcx + 10h]

popfq
ret

back:
xor eax, eax
ret

RestoreContext Endp

End