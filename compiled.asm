; AUTOGENERATED CODE
IN
POP  [0]     ; x
; call
; giving args
PUSH [0]     ; x
POP  [1]     ; 0 + 1
; ended giving args
; pushing old base pointer (RBX)
PUSH RBX
; shifting base pointer (RBX)
PUSH 1
POP  RBX
; ended shifting base pointer (RBX)
CALL factorial:
POP  RBX
PUSH RAX
; call ended
OUT
JMP END_OF_FUNC_factorial: ;skipping func body
factorial:
PUSH [RBX 0] ; number (local)
PUSH 1
SUB
PUSH 0
JE IF_END_0:
; call
; giving args
PUSH [RBX 0] ; number (local)
PUSH 1
SUB
POP  [RBX 2]     ; 0 + 2
; ended giving args
; pushing old base pointer (RBX)
PUSH RBX
; shifting base pointer (RBX)
PUSH RBX 2
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
