/*
 * Created by v1tr10l7 on 03.03.2025.
 * Copyright (c) 2024-2025, Szymon Zemke <v1tr10l7@proton.me>
 *
 * SPDX-License-Identifier: GPL-3
 */
.intel_syntax noprefix
.code64

.section .text

# @brief set @count bytes to @value
# @param rdi - dest
# @param rsi - value
# @param rdx - count
#
# @return dest
.global memset
memset:
    # save return value
    push rdi

    mov rax, rsi
    mov rcx, rdx
    rep stosb

    pop rax
    ret

# @brief copy @count bytes from @src to @dest
# @param rdi - dest
# @param rsi - src
# @param rdx - count
# @return pointer to @dest
.global memcpy
memcpy:
    mov rax, rdi
    mov rcx, rdx
    rep movsb
    ret

# @brief copy @count bytes from @src to @dest,
# memory regions may overlap
# @param rdi - dest
# @param rsi - src
# @param rdx - count
# @return pointer to @dest
.global memmove
memmove:
    mov rcx, rdx
    mov rax, rdi
    
    cmp rdi, rsi
    ja memmove.continue

    rep movsb
    jmp memmove.done

memmove.continue:
    lea rdi, [rdi + rcx - 1]
    lea rsi, [rsi + rcx - 1]
    std
    rep movsb
    cld
memmove.done:
    ret

.global memcmp
# @brief compare @count bytes of @s1 and @s2
# @param rdi - s1
# @param rsi - s2
# @param rdx - count
# @return 0 if regions are equal, 
#  negative value if s1 < s2, and positive if s1 > s2
memcmp:
    test rdx, rdx
    # count is 0, so just return
    je memcmp.done

    xor eax, eax
    # alignment
    nop word ptr [rax + rax]
memcmp.loop:
    movzx ecx, byte ptr [rsi + rax]
    cmp byte ptr [rdi + rax], cl
    jne memcmp.fail
    
    inc rax
    cmp rdx, rax
    jne memcmp.loop
memcmp.done:
    xor eax, eax
    ret
memcmp.fail:
    # compute the return value
    setae al
    movzx eax, al
    add eax, eax
    add eax, -1
    ret
