PUSH 1
POP  [0]     ; x
PUSH 2
POP  [1]     ; y
PUSH 228
COS
POP  [2]     ; b
PUSH [0]     ; x
PUSH [1]     ; y
MUL
SIN
PUSH [2]     ; b
ADD
POP  [3]     ; z
HLT
