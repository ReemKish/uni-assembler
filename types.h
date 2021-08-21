/* ===== types.h ==========================================
 * Contains definitions of most of the program's datatypes.
 * Datatypes defined:
 *
 * ----- Operation instruction -------------
 * NOTE: enum OpId is defined in "consts.h" and not here since the value of its members must conform
 *       to the launguage specifications and therefore make use of various constants.
 * enum OpType      - Enumeration of the three operation types: R, I, J (see language specs).
 * OpInstruction_t  - Generic operation instruction. 
 * Op_t             - Generic operation parameters, implemented as a union.  
 * struct RtypeOp/ItypeOp/JtypeOp
 *                  - Optype-specific parameters.
 *
 * ----- Directive instruction -------------
 * enum DirId       - Enumeration of the directives.
 * DirInstruction_t - Generic directive instruction.
 * Dir_t            - Generic directive parameters, implemented as a union.
 * struct AtypeDir  - Paramaters of array-type directives (.dh, .dw, .db).
 * struct StypeDir  - Paramaters of single(or string)-type directives (.asciz, .entry, .extern).
 *
 * ----- Generic statement & instruction ---
 * Instruction_t  - Generic instruction, implemented as a union.
 * enum StmType   - Enumeration of the different types of statements.
 * Statement_t    - Generic statement.
 *
 * ----- Token -----------------------------
 * enum TokType - Enumeration of the different types of tokens.
 * TokVal_t     - Generic value of a token, implemented as a union.
 * Token_t      - Generic token, includes the token's type, value and index in line.
 */
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

/* ===== Statement & Instruction ========================== */
/* ----- Operation instruction ---------------------------- */
enum OpType {
  OPTYPE_R,
  OPTYPE_I,
  OPTYPE_J
};

struct RtypeOp {
  uint8_t funct;  /* funct field as in the launguage specs */
  uint8_t rd;     /* rd register id */
  uint8_t rt;     /* rt register id */
  uint8_t rs;     /* rs register id */
};

struct ItypeOp {
  char *label;    /* for branch ops */
  int16_t immed;  /* immed field as the launguage specs */
  uint8_t rt;     /* rt register id */
  uint8_t rs;     /* rs register id */
};

struct JtypeOp {
  char *label;    /* assigned during tokenization: label name for operations that require a label */
  int32_t addr;   /* assigned during parsing: address/id of the required label/register */
  uint8_t reg;    /* 1 if jmp operation requires a register, else 0. */
};

typedef union Op {
  struct RtypeOp Rop;
  struct ItypeOp Iop;
  struct JtypeOp Jop;
} Op_t;

typedef struct OpInstruction {
  enum OpId opcode;   /* opcode of the operation as described in the language specs */
  Op_t op;            /* operation-specific parameters */
} OpInstruction_t;


/* ----- Directive instruction ---------------------------- */

/* Array type directives are .dh, .dw or .db */
struct AtypeDir {
  uint8_t argc;   /* count of arguments of the directive */
  void *argv;     /* pointer to the array of arguments */
};

/* Single (or String) type directives are .asciz, .entry or .extern */
union StypeDir {
  /* NOTE: Here *str and *label point to the same value, the union merely offers syntactic sugar. */
  char *str;      /* the string argument of a .asciz directive */
  char *label;    /* the label argument of a .entry/.extern directive */
};

typedef union Dir {
  struct AtypeDir Adir;
  union StypeDir Sdir;
} Dir_t;

enum DirId {
  DIR_INVALID,
  DIR_DB,
  DIR_DW,
  DIR_DH,
  DIR_ASCIZ,
  DIR_ENTRY,
  DIR_EXTERN
};
typedef struct DirInstruction {
  enum DirId dirid;   /* id of the directive as defined in consts.h */
  Dir_t dir;          /* directive-specific parameters */
} DirInstruction_t;


/* ----- Generic statement & instruction ------------------ */
typedef union Instruction {
  OpInstruction_t op_inst;
  DirInstruction_t di_inst;
} Instruction_t;

enum StmType /* statement types */  {
  STATEMENT_ERROR,      /* statement's instruction is erroneous */
  STATEMENT_OPERATION,  /* statement's instruction is an operation */
  STATEMENT_DIRECTIVE,  /* statement's instruction is a directive */
  STATEMENT_IGNORE,     /* blank line or comment line */
  STATEMENT_END
};
typedef struct Statement {
  enum StmType type;    /* type of the statement                            */
  int line_ind;         /* index of the statement's line                    */
  char *label;          /* name of the label associated with the statement
                           (defined in the beginning of the line).
                           NULL if irrelevant.                              */
  Instruction_t inst;   /* instruction parameters                           */
} Statement_t;


/* ===== Token ============================================ */
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
  enum OpId opid;     /* corresponds to type TOK_OP */
  enum DirId dirid;		/* corresponds to type TOK_DIR */
  char *str;      		/* corresponds to type TOK_STRING */
  int immed;		      /* corresponds to type TOK_IMMED */
  int reg;        		/* corresponds to type TOK_REG */
  char *label;	    	/* corresponds to type TOK_LABEL or TOK_LABELDEF */
} TokVal_t;

typedef struct Token {
  enum TokType type;  /* type of the token */
  int ind;            /* index of the token in line */
  TokVal_t value;     /* value the token holds */
} Token_t;


#endif
