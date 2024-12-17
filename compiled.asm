PUSH 0
POP  [0]     ; x
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
PUSH 8
POP  [2]     ; y
HLT
