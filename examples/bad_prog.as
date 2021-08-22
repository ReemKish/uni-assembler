; ===== file bad_prog.as ==================================
; this file contains very erroneous assembly code.
; in fact, every single implemented error occurs in this file.



; "unexpected token"
    move  $1,   mov
    call  "STRING"
_L: stop
    jmp   42
  . asciz "Hello World!"

; "unexpected end of line"
    add   $1,   $2

; "unrecognized directive"
ValidLabel: .invaldir 12, 35
    .Db   12,   35

; "invalid register"
    add   $1,   $+2,  $5
    add   $-1,  $2,   $5
    add   $+0,  $2.0, $+5*7
    add   $1e1, $2e0, $5
    add   $1    $2,   $3

; "numeric literal out of bounds"
    .db   0,  1, 128
    .dh   0, -32769, 6
    .dw   47358345934, 130 ,5
    addi  $1, 4353464562, $5

; "label name is a reserved word"
    addi:   addi $1,10,$1
    asciz:  stop
    dh:     jmp ValidLabel

; "label name exceeds character limit"
SuperCaliFragelisticExpialyDoeuicousLavl: stop

; "line exceeds character limit"
    .dw 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 213, 31415, 345, 2134, -123, -124, 124435, 123, -1324

; "refrence to undefined label"
    jmp UndeFinedLabel

; "label defined as both external and an entry"
; "label declared entry but not defined in file"
    .entry    ConfusedLabel
    .extern   ConfusedLabel

; "label declared external but defined in file"
    .extern     externalLabel
externalLabel:  stop

; "label defined more than once"
Label:  .asciz "I was already defined above"
Label:  .asciz "I was already defined above"

; "external label operand to branch operation"
    .extern     reallyExternalLabel
    bne $1, $2, reallyExternalLabel

; Warning - "attempted jump to data symbol"
You:  .asciz "You is a data symbol"
      jmp You

; Warning - "redundant label definition on .entry statement"
redundantLabel2:  .entry    You

; Warning - "redundant label definition on .extern statement"
redundantLabel:   .extern   Me
