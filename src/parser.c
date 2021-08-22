/* ===== parser.c ======================================
 * This module is responsible for processing the raw input (lines of the source file)
 * into meaningful tokens. The way this is done is via the main function next_token which
 * receives a line of assembly source code and outputs the next token in it with each
 * successive call.
 * Each token has a type, and stores a value corresponding to that type.
 * e.g: a token of type REG_TOK stores the id of a register.
 * The Token_t data structure and the enumeration of the token types are defined in "types.h".
 */

/* ===== Includes ========================================= */
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>
#include "parser.h"
#include "tokenizer.h"
#include "tables.h"
#include "errors.h"
#include "types.h"
#include "consts.h"

/* ===== CPP Definitions ================================== */
/* evaluates to 1 iff x is in bounds of signed int with n bits */
#define IN_BOUNDS(x,n) ((~0 << ((n)-1) <= (x)) && ((x) <= ~(~0 << ((n)-1))))

/* ===== Declarations ===================================== */
static Error_t error;   /* errno for statement errors */
extern int error_occurred;  /* defined in "errors.c" */

/* ----- prototypes --------------------------------------- */
Statement_t* parse_file(FILE *file);
static void free_statement(Statement_t stm);
static int parse_token   (Token_t tok, Statement_t *stm, long *flags);
static int parse_op      (Token_t tok, Statement_t *stm, long *flags);
static int parse_dir     (Token_t tok, Statement_t *stm, long *flags);
static int parse_labeldef(Token_t tok, Statement_t *stm, long *flags);
static int parse_label   (Token_t tok, Statement_t *stm, long *flags);
static int parse_reg     (Token_t tok, Statement_t *stm, long *flags);
static int parse_immed   (Token_t tok, Statement_t *stm, long *flags);
static int parse_string  (Token_t tok, Statement_t *stm, long *flags);
static int parse_line(char *line, Statement_t *statement);
static int immed_in_bounds(Statement_t stm, long immed);

/* ===== Code ============================================= */

/*
 * Parses tok as an operation token and updates stm and flags accordingly.
 * Sets the statement type and instruction opcode, and in case
 * of a R-type op, the funct field.
 * Always returns 0 to indicate no errors.
 */
static int  /* always 0 */
parse_op(Token_t tok, Statement_t *stm, long *flags)
{
  enum OpId opid = tok.value.opid;
  enum OpType optype = OPID_TO_OPTYPE(opid);
  OpInstruction_t *op_inst = &stm->inst.op_inst;
  stm->type = STATEMENT_OPERATION;
  op_inst->opcode = opid & OPCODE_MASK;

  if(optype == OPTYPE_R) {
    op_inst->op.Rop.funct = opid >> FUNCT_SHIFT;
  }

  switch(opid) {
    case OP_LA:
    case OP_CALL:
      *flags = EXP_LABEL;
      break;
    case OP_JMP: 
      *flags = EXP_REG | REG_RS | EXP_LABEL;
      break;
    case OP_STOP:
      *flags = EXP_END;
      break;
    default:
      *flags = EXP_REG | REG_RS;
  }
  return 0;
}


/*
 * Parses tok as a directive token and updates stm and flags accordingly.
 * Sets the statement type and directive id.
 * If the token's dirid is invalid, returns EINVAL_DIR,
 * else 0 to indicate no erros.
 */
static int  /* nonzero on failure */
parse_dir(Token_t tok, Statement_t *stm, long *flags)
{
  enum DirId dirid = tok.value.dirid;
  DirInstruction_t *di_inst = &stm->inst.di_inst;
  if(dirid == DIR_INVALID) {
    return EINVAL_DIR;
  }
  stm->type = STATEMENT_DIRECTIVE;
  di_inst->dirid = dirid;

  switch(dirid) {
    case DIR_ASCIZ:
      *flags = EXP_STRING;
      break;
    case DIR_ENTRY:
    case DIR_EXTERN:
      *flags = EXP_LABEL;
      break;
    default:  /* .dh, .dw or .db*/
      *flags = EXP_IMMED;
  };
  return 0;
}


/*
 * Parses tok as a label definiton token and updates stm and flags accordingly.
 * Sets the statement label.
 * If the label name is a reserved keyword returns EINVAL_LABEL.
 * If the label name is longer than MAX_LABEL_LEN, returns ELONG_LABEL.
 * Else 0 to indicate no erros.
 */
static int  /* always 0 */
parse_labeldef(Token_t tok, Statement_t *stm, long *flags)
{
  char** labelp = &stm->label;
  if(  -1 != search_op(tok.value.label)
    || -1 != search_dir(tok.value.label)) {
    free(tok.value.label);
    return EINVAL_LABEL;
  }
  if(strlen(tok.value.label) > MAX_LABEL_LEN) {
    free(tok.value.label);
    return ELONG_LABEL;
  }
  *labelp = tok.value.label;
  *flags = EXP_OP | EXP_DIR;
  return 0;
}


/*
 * Parses tok as a label token and updates stm and flags accordingly.
 * Sets the Iop/Jop instruction label.
 * Always returns 0 to indicate no erros.
 */
static int  /* always 0 */
parse_label(Token_t tok, Statement_t *stm, long *flags)
{
  char **labelp;
  labelp = OPCODE_TO_OPTYPE(stm->inst.op_inst.opcode) == OPTYPE_I ?
           &stm->inst.op_inst.op.Iop.label :  /* branch op */
           &stm->inst.op_inst.op.Jop.label;
  *labelp = tok.value.label;
  *flags = EXP_END;
  return 0;
}


/*
 * Parses tok as a register token and updates stm and flags accordingly.
 * If the currently parsed operation is of type:
 *  R-type or I-type:
 *    sets the appropriate register field according to the set flag (REG_RS, REG_RD or REG_RT)
 *  J-type:
 *    sets the reg field to 1 and the addr field to the regiser id.
 * If the register id is not a valid register, returns EINVAL_REG,
 * else returns 0 to indicate no erros.
 */
static int  /* nonzero on failure */
parse_reg(Token_t tok, Statement_t *stm, long *flags)
{
  int reg = tok.value.reg;
  OpInstruction_t *op_inst = &(stm->inst.op_inst);
  if(reg > 31 || reg < 0) {
    return EINVAL_REG;
  }
  /* determine which operand corresponds to the register */
  /* JMP operation */
  if (op_inst->opcode == OP_JMP) {
    op_inst->op.Jop.reg = 1;
    op_inst->op.Jop.addr = reg;
    *flags = EXP_END;
  }
  /* R-type operation */
  else if(OPCODE_TO_OPTYPE(op_inst->opcode) == OPTYPE_R) {
    if(*flags & REG_RS) {  /* rs register */
      op_inst->op.Rop.rs = reg;
      *flags = EXP_REG;
      *flags |= (op_inst->opcode == 1) ? REG_RD : REG_RT;
    } else if(*flags & REG_RT) {  /* rt register */
      op_inst->op.Rop.rt = reg;
      /* is a move operation (2 operands) */
      if(op_inst->opcode == 1) {
        *flags = EXP_END;
      }
      /* is a logical/arithmetic operation (3 operands)*/
      else {
        *flags = EXP_REG | REG_RD;
      }
    } else if(*flags & REG_RD) {  /* rd register */
      op_inst->op.Rop.rd = reg;
      *flags = EXP_END;
    }
  }
  /* I-type operation */
  else if(OPCODE_TO_OPTYPE(op_inst->opcode) == OPTYPE_I) {
    if (*flags & REG_RS) {
      op_inst->op.Iop.rs = reg;
      if(IS_BRANCH_OP(op_inst->opcode)) {
        *flags = EXP_REG | REG_RT;
      } else {
        *flags = EXP_IMMED;
      }

    } else if (*flags & REG_RT) {
      op_inst->op.Iop.rt = reg;
      if(IS_BRANCH_OP(op_inst->opcode)) {
        *flags = EXP_LABEL;
      } else {
        *flags = EXP_END;
      }
    }
  }
  return 0;
}


/*
 * Parses tok as an immediate token and updates stm and flags accordingly.
 * If the currently parsed instruction is of type:
 *  Operation:
 *    sets the Iop immed field to the token's immediate.
 *  Directive:
 *    appends the token's immediate to the argument array of the directive.
 * If the immediate is out of bounds for a 16-bit signed integer, returns EINVAL_IMMED,
 * else returns 0 to indicate no erros.
 */
static int  /* nonzero on failure */
parse_immed(Token_t tok, Statement_t *stm, long *flags)
{
  struct AtypeDir *Adir = &(stm->inst.di_inst.dir.Adir);
  int size=0, i;
  int immed = tok.value.immed;
  if(0 == (size = immed_in_bounds(*stm, immed)))
    return EINVAL_IMMED;
  /* determine which operand corresponds to the register */
  if (stm->type == STATEMENT_OPERATION) {
    stm->inst.op_inst.op.Iop.immed = immed;
    *flags = EXP_REG | REG_RT;
  } else {
    if(!IN_BOUNDS(immed, size*8)) {
      return EINVAL_IMMED;
    }
    Adir->argv = realloc(Adir->argv, (Adir->argc + 1) * size);
    for(i=0; i<size; i++) {
      ((char *)Adir->argv)[Adir->argc*size+i] = ((char *)&immed)[i];
    }
    Adir->argc++;
    *flags = EXP_IMMED | EXP_END;
  }
  return 0;
}


/*
 * Determines whether the immediate value is within bounds
 * given the context of the current statement.
 * e.g: 130 is within bounds in context of a lw instruction,
 *      but noy within bounds in context of a .db instruction.
 * If the immediate is within bounds, returns the size (in bytes)
 *  of an immediate in that context - a nonzero value.
 * Rlse returns 0.
 */
static int  /* size of an immediate in context of the statement */
immed_in_bounds(Statement_t stm, long immed)
{
  int size = 2;
  if (stm.type == STATEMENT_OPERATION) {
    switch(stm.inst.op_inst.opcode) {
      case OP_LB:
      case OP_SB:
        size = 1; break;
      case OP_LH:
      case OP_SH:
        size = 2; break;
      case OP_LW:
      case OP_SW:
        size = 4; break;
      default: break;
    }
  } else if(stm.type == STATEMENT_DIRECTIVE) {
    switch(stm.inst.di_inst.dirid) {
      case DIR_DB: size = 1; break;
      case DIR_DH: size = 2; break;
      case DIR_DW: size = 4; break;
      default: break;
    }
  }
  if(!IN_BOUNDS(immed, size*8))
    return 0;
  return size;
}


/*
 * Parses tok as a string token and updates stm and flags accordingly.
 * Sets the .asciz directive's string field.
 * Always returns 0 to indicate no erros.
 */
static int  /* always 0 */
parse_string(Token_t tok, Statement_t *stm, long *flags)
{
  stm->inst.di_inst.dir.Sdir.str = tok.value.str;
  *flags = EXP_END;
  return 0;
}

/*
 * Parses the assembly source code in file into an array of statements.
 */
Statement_t* parse_file(FILE *file)
{
  enum ErrId errid;
  Statement_t *statements = calloc((MAX_PROG_LINES+1), sizeof(Statement_t));
  char line[LINE_BUFFER_SIZE];
  int i = 0;
  while (NULL != fgets(line, LINE_BUFFER_SIZE, file)) {
    if (0 != (errid = parse_line(line, &statements[i]))) {  /* error occured */
      error_occurred = 1;
      error.line_ind = i+1;
      print_error(error);
      free(error.line);
    }
    statements[i].line_ind = i+1;
    i++;
  }
  statements[i].type = STATEMENT_END;
  return statements;
}


/*
 * Parses the assembly source code in line into a statement.
 * On failure, updates the static error variable and returns the error id.
 */
static int  /* error id - nonzero on failure */
parse_line(char *line, Statement_t *statement)
{
  enum ErrId errid;
  Token_t token;
  char *line_cpy = malloc(strlen(line)+1);
  /* initial flags */
  long flags =  EXP_LABELDEF |  /* line may start with a label definiton */
                EXP_COMMENT  |  /* line may be a comment */
                EXP_END      |  /* line may be empty */
                EXP_OP       |  /* line may start with an operation */
                EXP_DIR;        /* line may start with a directive */

  strcpy(line_cpy, line);
  memset(statement, 0, sizeof(*statement));
  if(strlen(line) > MAX_LINE_LEN) {
    token.ind = -1;
    errid = ELONG_LINE;
    goto Error;
  }
  for (token = next_token(line); ; token = next_token(NULL)) {
    if(0 != (errid = parse_token(token, statement, &flags))) {  /* error occured */
      /* free the token's allocated string value */
      if(  token.type == TOK_STRING
        || token.type == TOK_LABEL)
        free(token.value.str);
      goto Error;
    }
    if(statement->type == STATEMENT_IGNORE || token.type == TOK_END) break;
  }
  free(line_cpy);
  return 0;
Error:
    free_statement(*statement);
    statement->type = STATEMENT_ERROR;
    error.errid = errid;
    error.line = line_cpy;
    error.tok = token;
    error.flags = flags;
    return errid;
}

/*
 * Frees all the statement's allocated memory.
 */
static void
free_statement(Statement_t stm)
{
  free(stm.label);
  /* operation statement */
  if(stm.type == STATEMENT_OPERATION) {
    switch(OPCODE_TO_OPTYPE(stm.inst.op_inst.opcode)) {
      case OPTYPE_I:
        free(stm.inst.op_inst.op.Iop.label);
        break;
      case OPTYPE_J:
        free(stm.inst.op_inst.op.Jop.label);
      case OPTYPE_R:
        break;
    }
  }
  /* directive statement */
  else if(stm.type == STATEMENT_DIRECTIVE) {
    switch(stm.inst.di_inst.dirid) {
      case DIR_ENTRY:
      case DIR_EXTERN:
      case DIR_ASCIZ:
        free(stm.inst.di_inst.dir.Sdir.str);
        break;
      default:
        free(stm.inst.di_inst.dir.Adir.argv);
    }

  }
}

/*
 * Parses the token and updates stm accordingly.
 * On failure, returns the error id.
 */
static int  /* error id - nonzero on failure */
parse_token(Token_t tok, Statement_t *stm, long *flags)
{
  enum ErrId errid = EUNEXPECTED_TOK;
  switch(tok.type) {
		case TOK_ERR:
      errid = EUNKNOWN_TOK;
			break;
		case TOK_EMPTY:
			break;
		case TOK_LABELDEF:
      if(*flags & EXP_LABELDEF)
        errid = parse_labeldef(tok, stm, flags);
			break;
		case TOK_OP:
      if(*flags & EXP_OP)
        errid = parse_op(tok, stm, flags);
			break;
		case TOK_DIR:
      if(*flags & EXP_DIR)
        errid = parse_dir(tok, stm, flags);
			break;
		case TOK_REG:
      if(*flags & EXP_REG)
        errid = parse_reg(tok, stm, flags);
			break;
		case TOK_IMMED:
      if(*flags & EXP_IMMED)
        errid = parse_immed(tok, stm, flags);
			break;
		case TOK_STRING:
      if(*flags & EXP_STRING)
        errid = parse_string(tok, stm, flags);
			break;
		case TOK_LABEL:
      if(*flags & EXP_LABEL)
        errid = parse_label(tok, stm, flags);
			break;
		case TOK_COMMENT:
      if(*flags & EXP_COMMENT) {
        stm->type = STATEMENT_IGNORE;
        errid = 0;
      }
			break;
		case TOK_END:
      if(!(*flags & EXP_END))
        errid = EUNEXPECTED_EOL;  /* unexpected end */
      else
        errid = 0;
			break;
  }
  return errid;
}
