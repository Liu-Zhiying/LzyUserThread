.XMM
.Model Small
.Code
@SaveContext@4 Proc

test dword ptr [ecx + 0c0h], 1h
jnz back

mov [ecx + 18h], edi
mov [ecx + 1ch], esi
mov [ecx + 20h], ebx
mov [ecx + 24h], edx
mov [ecx + 28h], ecx
mov [ecx + 2ch], eax
mov [ecx + 30h], ebp

mov eax, [esp]
mov [ecx + 34h], eax

mov eax, esp
add eax, 4h
mov [ecx + 38h], eax

pushfd
pop eax
mov [ecx + 3ch], eax

movups xmmword ptr [ecx + 40h], xmm0
movups xmmword ptr [ecx + 50h], xmm1
movups xmmword ptr [ecx + 60h], xmm2
movups xmmword ptr [ecx + 70h], xmm3
movups xmmword ptr [ecx + 80h], xmm4
movups xmmword ptr [ecx + 90h], xmm5
movups xmmword ptr [ecx + 0a0h], xmm6
movups xmmword ptr [ecx + 0b0h], xmm7

mov [ecx + 0h], ss
mov [ecx + 4h], gs
mov [ecx + 8h], fs
mov [ecx + 0ch], es
mov [ecx + 10h], ds
mov [ecx + 14h], cs

or dword ptr [ecx + 0c0h], 1h

xor eax, eax
xor eax, 1
ret

back:
xor eax, eax
ret

@SaveContext@4 Endp

@RestoreContext@4 Proc

test dword ptr [ecx + 0c0h], 1h
jz back

mov esp, [ecx + 38h]

sub esp, 0ch
mov eax, [ecx + 14h]
mov [esp + 8h], eax
mov eax, [ecx + 34h]
mov [esp + 4h], eax
mov eax, [ecx + 3ch]
mov [esp], eax

mov edi, [ecx + 18h]
mov esi, [ecx + 1ch]
mov ebx, [ecx + 20h]
mov edx, [ecx + 24h]
mov eax, [ecx + 2ch]
mov ebp, [ecx + 30h]

movups xmm0, xmmword ptr [ecx + 40h]
movups xmm1, xmmword ptr [ecx + 50h]
movups xmm2, xmmword ptr [ecx + 60h]
movups xmm3, xmmword ptr [ecx + 70h]
movups xmm4, xmmword ptr [ecx + 80h]
movups xmm5, xmmword ptr [ecx + 90h]
movups xmm6, xmmword ptr [ecx + 0a0h]
movups xmm7, xmmword ptr [ecx + 0b0h]

mov ss, [ecx + 0h]
mov gs, [ecx + 4h]
mov fs, [ecx + 8h]
mov es, [ecx + 0ch]
mov ds, [ecx + 10h]

and dword ptr [ecx + 0c0h], 0fffffffeh

mov ecx, [ecx + 28h]

popfd
retf

back:
xor eax, eax
ret

@RestoreContext@4 Endp

End