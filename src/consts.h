/* ===== consts.h =========================================
 * Program constants and enumerations.
 * Among various constants, includes the OpId enum, which associates
 * between operation names and their opcode & funct constants.
 */
#ifndef CONSTS_H
#define CONSTS_H


/* ----- limits --------------------------------- */
#define MAX_PROG_LINES  2048  /* maximum lines of code in the input assembly program */
#define MAX_OPNAME_LEN  4
#define MAX_LABEL_LEN   32
#define MAX_LINE_LEN    80
#define LINE_BUFFER_SIZE (MAX_LINE_LEN*10)
#define MAX_PROG_MEMORY  (MAX_PROG_LINES * MAX_LINE_LEN / 2)

/* ----- syntax --------------------------------- */
#define COMMENT_CHAR ';'

/* ----- encoding ------------------------------- */
#define INITIAL_IC  100

/* ----- tokenization --------------------------- */
#define WSPACE_CHARS " \f\n\r\t\v"

/* ----- launguage encoding specifications ------ */
#define ENC_OPCODE_POS      (26)
#define ENC_REG_RD_POS      (11)
#define ENC_REG_RT_POS      (16)
#define ENC_REG_RS_POS      (21)
#define ENC_RTYPE_FUNCT_POS (6)
#define ENC_IOP_IMMED_MASK  (0xFFFF)
#define ENC_JOP_ADDR_MASK   (0x1FFFFFF)
#define ENC_JOP_REG_POS     (25)


/* ----- operation names and types -------------- */
#define FUNCT_SHIFT 6
#define OPCODE_MASK ((1 << FUNCT_SHIFT) - 1)
#define OPID_TO_OPTYPE(opid)     ((opid) >> FUNCT_SHIFT ? OPTYPE_R : ((opid) > OP_SH ? OPTYPE_J : OPTYPE_I))
#define OPCODE_TO_OPTYPE(opcode) (((opcode) == 0 || (opcode) == 1) ? OPTYPE_R : ((opcode) > OP_SH ? OPTYPE_J : OPTYPE_I))
#define IS_BRANCH_OP(opcode) (OP_BNE <= (opcode) && (opcode) <= OP_BGT)


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


#endif
