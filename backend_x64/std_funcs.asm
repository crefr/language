section .text

; ================================================================
; -----------------------------------------
; Standard function for reading numbers
; Entry:
;   no args
; Return:
;   rax = number given
; -----------------------------------------
__in_standard_func_please_do_not_name_your_funcs_this_name__:
        push rbp
        mov rbp, rsp

        sub rsp, 32        ; [rbp - 32] is start of the buffer

        xor r12, r12            ; index = 0

    ; --- checking first symbol for '-' ---
        ; --- reading ---
        mov rax, 0
        mov rdi, 0
        lea rsi, [-32 + rbp + r12*1]
        mov rdx, 1

        syscall
        ; ---------------
        inc r12

        cmp BYTE [rsi], '-'
        jne .reading_symbol

        dec r12
        mov r13, 1
    ; -------------------------------------

    .reading_symbol:
        ; --- reading ---
        mov rax, 0
        mov rdi, 0
        lea rsi, [-32 + rbp + r12*1]
        mov rdx, 1

        syscall
        ; ---------------

        inc r12                 ; index++

        cmp BYTE [rsi], 0x0A    ; do while byte != '\n'
        jne .reading_symbol

        dec r12

        xor rax, rax            ; num = 0
        mov r8, 1               ; decimal multiplicator
    .calc_loop:
        dec r12

        xor rcx, rcx
        mov cl, [-32 + rbp + r12*1]
        sub cl, '0'
        imul rcx, r8        ; r9 = (char - '0') * 10^n
        imul r8, r8, 10

        add rax, rcx

        test r12, r12
        jnz .calc_loop

    ; if it was negative:
        test r13, r13
        jz .the_end

        neg rax

    .the_end:

        mov rsp, rbp
        pop rbp

    ret
; ================================================================



; ================================================================
; -----------------------------------------
; Standard function for printing numbers
; Entry:
;   first arg on the stack - number to print
; -----------------------------------------
__out_standard_func_please_do_not_name_your_funcs_this_name__:
        push rbp
        mov rbp, rsp

        sub rsp, 32            ; [rbp - BUF_LEN] is start of the buffer

        xor r12, r12                ; index = 0

        mov rax, [rbp + 16]         ; rax = first arg

        mov r8, QWORD 0x7000000000000000
        xor rcx, rcx

        test rax, r8
        jz .print_unsigned

        mov BYTE [-32 + rbp], '-'
        inc rcx

        neg rax

    .print_unsigned:
        sub rsp, 32
        mov rsi, rsp                ; rsi = &num_buf

        xor r11, r11
        mov rdi, 10                 ; divisor
    .num_loop:
        xor rdx, rdx                ; edx = 0 (for div)

        div rdi                     ; dl = num % 10
        add dl, '0'                 ; dl = ascii digit
        mov BYTE [rsi + r11], dl          ; into reversed buffer
        inc r11

        cmp rax, 0                  ; until number is 0
        jne .num_loop

        dec rcx
    .reverse_loop:
        inc rcx
        dec r11
        mov dl, BYTE [rsi + r11]        ; from reversed buffer
        mov BYTE [-32 + rbp + rcx], dl        ; to normal buffer

        cmp r11, 0
        jne .reverse_loop

        inc rcx
        mov BYTE [-32 + rbp + rcx], 0x0a        ; \n

        inc rcx

    ; --- writing buffer ---
        mov rax, 1
        mov rdi, 1
        lea rsi, [-32 + rbp]
        mov rdx, rcx
        syscall
    ; ----------------------

        mov rsp, rbp
        pop rbp
    ret
; ================================================================

