/* ===== Includes ========================================= */
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>
#include "tokenizer.h"
#include "parser.h"
#include "types.h"
#include "consts.h"
#include "tables.h"
#include "errors.h"


/* ===== Declarations ===================================== */
static Error_t error;  /* like errno for statement errors */
int parse_line(char *line, Statement_t *statement);
int parse_token(Token_t token, Statement_t *statement, long *flags);


/* ===== Code ============================================= */
int parse_op(Token_t tok, Statement_t *stm, long *flags)
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


int parse_dir(Token_t tok, Statement_t *stm, long *flags)
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

int parse_labeldef(Token_t tok, Statement_t *stm, long *flags)
{
  char** labelp = &stm->label;
  *labelp = tok.value.label;
  *flags = EXP_OP | EXP_DIR;
  return 0;
}

int parse_label(Token_t tok, Statement_t *stm, long *flags)
{
  char **labelp;
  labelp = OPCODE_TO_OPTYPE(stm->inst.op_inst.opcode) == OPTYPE_I ?
           &stm->inst.op_inst.op.Iop.label :  /* branch op */
           &stm->inst.op_inst.op.Jop.label;
  *labelp = tok.value.label;
  *flags = EXP_END;
  return 0;
}

int parse_reg(Token_t tok, Statement_t *stm, long *flags)
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

int parse_immed(Token_t tok, Statement_t *stm, long *flags)
{
  struct AtypeDir *Adir = &(stm->inst.di_inst.dir.Adir);
  int size=0, i;
  int immed = tok.value.immed;
  if(immed > 32767 || immed < -32768) {
    return EINVAL_IMMED;
  }
  /* determine which operand corresponds to the register */
  if (stm->type == STATEMENT_OPERATION) {
    stm->inst.op_inst.op.Iop.immed = immed;
    *flags = EXP_REG | REG_RT;
  } else {
    switch(stm->inst.di_inst.dirid) {
      case DIR_DB: size = 1; break;
      case DIR_DH: size = 2; break;
      case DIR_DW: size = 4; break;
      default: break;
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

int parse_string(Token_t tok, Statement_t *stm, long *flags)
{
  stm->inst.di_inst.dir.Sdir.str = tok.value.str;
  *flags = EXP_END;
  return 0;
}

/* parses the assembly source code in file into an array of statements.
 **/
Statement_t* parse_file(FILE *file)
{
  enum ErrId errid;
  Statement_t *statements = malloc(sizeof(Statement_t) * (MAX_PROG_LINES+1));
  char* line = malloc(MAX_LINE_LEN+2);  /* two extra bytes for newline and null terminator */
  int i = 0;
  while (NULL != fgets(line, MAX_LINE_LEN+2, file)) {
    if (0 != (errid = parse_line(line, &statements[i++]))) {  /* error occured */
      print_error(error, i);
    }
  }
  statements[i].type = STATEMENT_END;
  return statements;
}


/* parses the assembly source code in line into a statement.
 **/
int parse_line(char *line, Statement_t *statement)
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
  for (token = next_token(line); ; token = next_token(NULL)) {
    if(0 != (errid = parse_token(token, statement, &flags))) {  /* error occured */
      statement->type = STATEMENT_ERROR;
      free(error.line);
      error.errid = errid;
      error.line = line_cpy;
      error.tok = token;
      error.flags = flags;
      return errid;
    }
    if(statement->type == STATEMENT_IGNORE || token.type == TOK_END) break;
  }
  return 0;
}

int parse_token(Token_t tok, Statement_t *stm, long *flags)
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
