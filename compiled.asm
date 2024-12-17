PUSH 15
POP  [0]     ; x
PUSH 0
POP  [1]     ; y
IN
POP  [1]     ; y
PUSH 1
POP  [2]     ; answer
PUSH [0]     ; x
PUSH [1]     ; y
SUB
PUSH 0
JE IF_END_108782931674880:
PUSH 0
POP  [2]     ; answer
IF_END_108782931674880:
PUSH [2]     ; answer
OUT
HLT
