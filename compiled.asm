PUSH 100
POP  [0]     ; x
PUSH 0
POP  [1]     ; y
IN
POP  [1]     ; y
WHILE_BEGIN_0:
PUSH [0]     ; x
PUSH [1]     ; y
SUB
PUSH 0
JE WHILE_END_0:
PUSH [1]     ; y
PUSH 1
ADD
POP  [1]     ; y
PUSH [1]     ; y
OUT
JMP WHILE_BEGIN_0:
WHILE_END_0:
HLT
