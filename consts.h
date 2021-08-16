#ifndef CONSTS_H
#define CONSTS_H

/* ----- limits --------------------------------- */
#define MAX_LINE_LEN    80
#define MAX_OPNAME_LEN  4
#define MAX_LABEL_LEN   32
#define MAX_PROG_LINES  2048  /* maximum lines of code in the input assembly program */
#define MAX_PROG_MEMORY (MAX_PROG_LINES * MAX_LINE_LEN / 2)

/* ----- syntax --------------------------------- */
#define COMMENT_CHAR ';'

/* ----- encoding ------------------------------- */
#define INITIAL_IC  100

/* ----- tokenizing ----------------------------- */
#define WSPACE_CHARS " \f\n\r\t\v"


/* ----- operation names and types -------------- */
#define FUNCT_SHIFT 6
#define OPCODE_MASK ((1 << FUNCT_SHIFT) - 1)
#define OPID_TO_OPTYPE(opid)     ((opid) >> FUNCT_SHIFT ? OPTYPE_R : ((opid) > 24 ? OPTYPE_J : OPTYPE_I))
#define OPCODE_TO_OPTYPE(opcode) (((opcode) == 0 || (opcode) == 1) ? OPTYPE_R : ((opcode) > 24 ? OPTYPE_J : OPTYPE_I))
#define IS_BRANCH_OP(opcode) (15 <= (opcode) && (opcode) <= 18)

enum OpType {
  OPTYPE_R,
  OPTYPE_I,
  OPTYPE_J
};

enum OpId {
  /* R-type */
  /* (x << FUNCT_SHIFT | y) means funct x & opcode y */
  OP_RTYPE = 0,
  OP_ADD  = (1 << FUNCT_SHIFT | 0),
  OP_SUB  = (2 << FUNCT_SHIFT | 0),
  OP_AND  = (3 << FUNCT_SHIFT | 0),
  OP_OR   = (4 << FUNCT_SHIFT | 0),
  OP_NOR  = (5 << FUNCT_SHIFT | 0),
  OP_MOVE = (1 << FUNCT_SHIFT | 1),
  OP_MVHI = (2 << FUNCT_SHIFT | 1),
  OP_MVLO = (3 << FUNCT_SHIFT | 1),
  /* I-type */
  OP_ADDI = 10,
  OP_SUBI = 11,
  OP_ANDI = 12,
  OP_ORI  = 13,
  OP_NORI = 14,
  OP_BNE  = 15,
  OP_BEQ  = 16,
  OP_BLT  = 17,
  OP_BGT  = 18,
  OP_LB   = 19,
  OP_SB   = 20,
  OP_LW   = 21,
  OP_SW   = 22,
  OP_LH   = 23,
  OP_SH   = 24,
  /* J-type */
  OP_JMP  = 30,
  OP_LA   = 31,
  OP_CALL = 32,
  OP_STOP = 63
};

enum DirId {
  DIR_INVALID,
  DIR_DB,
  DIR_DW,
  DIR_DH,
  DIR_ASCIZ,
  DIR_ENTRY,
  DIR_EXTERN
};


#endif
