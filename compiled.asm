; AUTOGENERATED CODE
PUSH 0
POP  [0]     ; x
WHILE_BEGIN_0:
PUSH [0]     ; x
PUSH 200
CALL __LESS_OP__:
PUSH 0
JE WHILE_END_0:
PUSH 0
POP  [1]     ; y
WHILE_BEGIN_1:
PUSH [1]     ; y
PUSH 10000
CALL __LESS_OP__:
PUSH 0
JE WHILE_END_1:
; call
; giving args
PUSH 15
POP  [3]     ; 0 + 3
; ended giving args
; pushing old base pointer (RBX)
PUSH RBX
; shifting base pointer (RBX)
PUSH 3
POP  RBX
; ended shifting base pointer (RBX)
CALL factorial:
POP  RBX
PUSH RAX
; call ended
POP  [2]     ; fact_result
PUSH [1]     ; y
PUSH 1
ADD
POP  [1]     ; y
JMP WHILE_BEGIN_1:
WHILE_END_1:
PUSH [0]     ; x
PUSH 1
ADD
POP  [0]     ; x
JMP WHILE_BEGIN_0:
WHILE_END_0:
JMP END_OF_FUNC_factorial: ;skipping func body
factorial:
PUSH [RBX 0] ; number (local)
PUSH 1
CALL __GREATER_OP__:
PUSH 0
JE IF_END_0:
PUSH [RBX 0] ; number (local)
PUSH 1
SUB
POP  [RBX 2] ; new_number (local)
; call
; giving args
PUSH [RBX 2] ; new_number (local)
POP  [RBX 3]     ; 0 + 3
; ended giving args
; pushing old base pointer (RBX)
PUSH RBX
; shifting base pointer (RBX)
PUSH RBX 3
POP  RBX
; ended shifting base pointer (RBX)
CALL factorial:
POP  RBX
PUSH RAX
; call ended
PUSH [RBX 0] ; number (local)
MUL
POP  [RBX 1] ; answer (local)
PUSH [RBX 1] ; answer (local)
POP  RAX
RET
IF_END_0:
PUSH 1
POP  RAX
RET
END_OF_FUNC_factorial:
HLT


;==========FUNCS FOR MACHINE ONLY USING==========
__GREATER_OP__:
JA __RETURN_TRUE__:
JMP __RETURN_FALSE__:

__LESS_OP__:
JB __RETURN_TRUE__:
JMP __RETURN_FALSE__:

__GREATER_EQ_OP__:
JAE __RETURN_TRUE__:
JMP __RETURN_FALSE__:

__LESS_EQ_OP__:
JBE __RETURN_TRUE__:
JMP __RETURN_FALSE__:

__EQUAL_OP__:
JE __RETURN_TRUE__:
JMP __RETURN_FALSE__:

__N_EQUAL_OP__:
JNE __RETURN_TRUE__:
JMP __RETURN_FALSE__:

__RETURN_FALSE__:
PUSH 0
RET

__RETURN_TRUE__:
PUSH 1
RET

