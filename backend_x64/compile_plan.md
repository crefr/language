# Compile plan

EVERYTHING IS ON THE STACK!!!

The only type is `int64`

For anything we potentially know its address (on the stack)

rvalue is on stack

## Values

If we get to the node of variable or number:

```asm
    push val
```

## Var

var a;

```asm
    ; get space for a
    sub rsp, 8
```

## Assignment

a = b

ASSIGN a, b

```asm
    ;  rvalue is on the stack

    pop [a]
```

## Simple math

### MUL (DIV)

MUL a, b

res = a * b

```asm
    xor rdx, rdx

    pop rcx     ; rcx = b
    pop rax     ; rax = a

    ;imul rsi
    idiv rcx

    push rax
```

### ADD (SUB)

SUB a, b

res = a - b

```asm
    pop rcx     ; rcx = b
    pop rax     ; rax = a

    ;add rax, rcx
    sub rax, rcx

    push rax
```

## Functions

### Call

CALL func(arg1, arg2, ..., argN)

```asm
    ; --- Pushing args ---
    ; handling with args from right to left
    ; --------------------

    call func       ; rax = ret value
    add rsp, N * 8

    push rax        ; pushing
```

### Return

RETURN value

```asm
    pop rax

    ; --- 1 variant ---
    mov rsp, rbp
    pop rbp
    ; --- 2 variant ---
    leave
    ; -----------------

    ret
```

### Func decl

FUNC_DECL func(arg1, arg2, ..., argN)

```asm
    ; --- 1 variant ---
    mov rbp, rsp
    push rbp
    ; --- 2 variant ---
    enter
```

## Global

### Start

```asm
    mov rbp, rsp
    push rbp
```

### Exit

```asm
    mov rax, 0x3c
    mov rdi, 0x01
    syscall
```

## In/Out

### In

in(var)

```asm
    call __in_standard_func_please_do_not_name_your_funcs_this_name__
    push rax
```

### Out

OUT out(var)

```asm
    push [var]
    call __out_standard_func_please_do_not_name_your_funcs_this_name__
    add rsp, 8
```

## If/else

### If without else

```asm
    ; condition result is on the stack
    pop rsi

    test rsi, rsi
    jz __IF_XXX_END

        ; ... IF body ...

__IF_XXX_END:
```

### If with else

```asm
    ; condition result is on the stack
    pop rsi

    test rsi, rsi
jz __IF_XXX_ELSE

    ; ... IF body ...

jmp __IF_XXX_END

__IF_XXX_ELSE:

    ; ... ELSE body

__IF_XXX_END:
```

## While

```asm
__WHILE_XXX_COND_CHECK:
    ; condition result is on the stack
    pop rsi

    test rsi, rsi
    jz __WHILE_XXX_END

    ; ... WHILE body ...

    jmp __WHILE_XXX_COND_CHECK
__WHILE_XXX_END:
```
