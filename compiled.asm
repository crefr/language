; AUTOGENERATED CODE
IN
POP  [0]     ; x
PUSH [0]     ; x
PUSH 5
CALL __GREATER_EQ_OP__:
PUSH 0
JE IF_END_0:
PUSH 52
OUT
IF_END_0:
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

