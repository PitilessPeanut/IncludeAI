; TODO: convert this to feedforawrd/feedbackward/etc later

    .globl is_prime
    .text

; int is_prime(int n)
is_prime:
    push rbp
    mov rbp, rsp
    mov DWORD PTR [rbp-4], edi        ; store n into local var

    ; if (n < 2) return 0;
    cmp DWORD PTR [rbp-4], 2
    jl .not_prime

    ; i = 2
    mov DWORD PTR [rbp-8], 2

.loop:
    mov eax, DWORD PTR [rbp-8]        ; eax = i
    imul eax, eax                     ; eax = i * i
    cmp eax, DWORD PTR [rbp-4]        ; if (i*i > n) break
    jg .is_prime

    mov eax, DWORD PTR [rbp-4]        ; eax = n
    xor edx, edx
    mov ecx, DWORD PTR [rbp-8]        ; ecx = i
    idiv ecx                          ; eax=n/i, edx=n%i

    cmp edx, 0                        ; if (n % i == 0)
    je .not_prime

    add DWORD PTR [rbp-8], 1          ; i++
    jmp .loop

.is_prime:
    mov eax, 1                        ; return 1 (true)
    pop rbp
    ret

.not_prime:
    mov eax, 0                        ; return 0 (false)
    pop rbp
    ret

.section .note.GNU-stack,"",@progbits
