#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include "consts.h"

/* ===== Terminology: Statements & Instructions ============
 * A statement is one line of assmebly source code,
 * it consists of an instruction and optionally a label.
 * An instruction can either be a directive instruction
 * or an operation instruction - as defined in the language specifications.
 * e.g: in the following source code line,
 *      Loop: MOV $1 $2
 * "MOV $1 $2" is an operation instruction and "Loop: MOV $1 $2" is a statement.
 * In addition, a directive/operation statement is a statement that
 * containts a directive/operation instruction.
 */

/* define statement types */
enum StmType {
  STATEMENT_ERROR,
  STATEMENT_OPERATION,
  STATEMENT_DIRECTIVE,
  STATEMENT_IGNORE,
  STATEMENT_END
};

/* ===== Tokens =========================================== */
enum TokType {
  TOK_ERR,
  TOK_EMPTY,
  TOK_LABELDEF,
  TOK_OP,
  TOK_DIR,
  TOK_REG,
  TOK_IMMED,
  TOK_STRING,
  TOK_LABEL,
  TOK_COMMENT,
  TOK_END
};

typedef union TokVal {
  enum OpId opid;
  enum DirId dirid;
  char *str;
  int immed;
  int reg;
  char *label;
} TokVal_t;

typedef struct Token {
  enum TokType type;
  int ind;
  TokVal_t value;
} Token_t;


/* ----- Operation instruction ---------------------------- */
struct RtypeOp {
  uint8_t funct;
  uint8_t rd;
  uint8_t rt;
  uint8_t rs;
};

struct ItypeOp {
  char *label;  /* for branch ops */
  int16_t immed;
  uint8_t rt;
  uint8_t rs;
};

struct JtypeOp {
  char *label;
  int32_t addr;
  uint8_t reg;
};

typedef union Op {
  struct RtypeOp Rop;
  struct ItypeOp Iop;
  struct JtypeOp Jop;
} Op_t;

typedef struct OpInstruction {
  enum OpId opcode;
  Op_t op;
} OpInstruction_t;


/* ----- Directive instruction ---------------------------- */

/* Array type directives are .dh, .dw or .db */
struct AtypeDir {
  uint8_t argc;
  void *argv;
};

/* Single (or String) type directives are .asciz, .entry or .extern */
union StypeDir {
  char *str;
  char *label;
};

typedef union Dir {
  struct AtypeDir Adir;
  union StypeDir Sdir;
} Dir_t;

typedef struct DirInstruction {
  enum DirId dirid;
  Dir_t dir;
} DirInstruction_t;


/* ----- Generic statement & instruction ------------------ */

typedef union Instruction {
  OpInstruction_t op_inst;
  DirInstruction_t di_inst;
} Instruction_t;

typedef struct Statement {
  enum StmType type;
  char *label;
  Instruction_t inst;
  } Statement_t;






#endif
