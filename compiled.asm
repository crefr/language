PUSH 0
POP  [0]     ; x
PUSH 2
CALL foo:
PUSH RAX
MUL
POP  [0]     ; x
PUSH [0]     ; x
OUT
JMP END_OF_FUNC_foo: ;skipping func body
foo:
PUSH [0]     ; x
PUSH 1
ADD
POP  [0]     ; x
PUSH [0]     ; x
POP  RAX
RET
END_OF_FUNC_foo:
HLT
