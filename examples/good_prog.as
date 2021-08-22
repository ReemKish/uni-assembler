; ===== good_prog.as ======================================
; this file contains valid assembly code.
; - even though it may not seem like it at first...

; ===== Syntax ============================================
; The following statements are successfuly parsed even
; though they have unusual syntax.

     beq     $31,$3    ,SomeeA
    di1:    or    $5  ,$0,   $1
     .dh    1 ,    2  ,1 ,    3
     .dh    0   ,  6  ,2   ,  3
     .dh    61   ,17  ,34   ,19
    enTry:   .asciz    "SomeeA"

SomeeA: ori    $1,-10,$2
 call       SomeLabelDefinedLater
   addi  $1  ,  +10,$5
 bne $1,  $31,Label1
    Heyo:           stop
Label1: subi $1  , -01, $10


; ===== R-type operations =================================
          ; arithmetic operaations
L1:       add   $14,    $14,    $12
          nor   $7,     $7,     $7
          or    $13,    $1,     $6
LABEL2:   and   $14,    $15,    $0
          sub   $6,     $8,     $9
          ; move operations  
          move  $1,     $2
          mvhi  $9,     $29
          mvlo  $5,     $8


; ===== I-type operations =================================:
          ; arithmetic operations
addI:     addi  $31,    -16758, $0
          subi  $31,    4758,   $0
          andi  $31,    0,      $0
FUNC:     ori   $31,    +564,   $0
          nori  $31,    -32768, $0
          ; load/store operations
          lh    $9,     32767,  $2
          lb    $0,     -128,   $2
          lw    $31,    -2147483648,  $0
          sw    $9,     2147483647,   $2
          sb    $0,     +127,   $2
          sh    $31,    -32768, $0
          ; branch operations
ascIz:    bne   $1,     $7,     FUNC
          bgt   $0,     $0,     ascIz
self:     blt   $1,     $31,    self
          beq   $0,     $0,     addI


; ===== Directives ========================================
STR1:     .asciz "Hello World"
ARR1:     .dh 124,  456, -345, -61, 123
BigARR:   .dw 3245, -546, 1,   0
ARR2:     .db 1
MiniSTR:  .asciz """
          .entry    MiniSTR
          .entry    addI
          .extern   HughumugousStr
          .extern   StrOfMedianLength
          .entry    STR1
          .entry    MiniSTR
          .extern   ExternalLabel


; ===== J-type operations =================================:
main:     jmp   addI
          jmp   $0
selfi:    jmp selfi
          call  main
          la    ARR1
          la    HughumugousStr
          la    MiniSTR
          stop
SomeLabelDefinedLater: jmp   ExternalLabel

