/* ===== scan.c ===========================================
 * This module is responsible for scanning the assembly statements (which
 * were parsed from the source file by "tokenizer.c" and "parser.c") and assembling
 * the symbol table and memory & instruction images - which are then used to write the 
 * ".ob", ".ent", ".ext" output files.
 */

/* ===== Includes ========================================= */
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "tables.h"
#include "types.h"
#include "errors.h"
#include "consts.h"

/* ===== Declarations ===================================== */
/* defined in "assembler.c" */
extern char inst_img[], mem_img[];
extern long IC, DC, ICF;

/* defined in "tables.c" */
extern int symtable_size;
extern SymbolEntry_t *symtable;

extern int error_occurred;  /* defined in "errors.c" */

/* ----- prototypes --------------------------------------- */
void write_memory_image(Statement_t *statements);
void write_instruction_image(Statement_t *statements);

static int32_t encode_op_stm(OpInstruction_t op_inst);
static int log_label(char *label, int attr, int line_ind);
static void write_instruction(int32_t inst_enc);
static int perform_directive(Statement_t stm);
static int check_symtable_integrity(Error_t *error);
static int handle_branch_op(Op_t *op);
static int handle_la_op(Op_t *op);
static int handle_jmp_op(Op_t *op);

/* ===== Code =============================================*/

/*
 * Returns the 4-byte encoding of an assembly operation instruction.
 * Makes use of various constants defined in "consts.h".
 */
static int32_t  /* the 4-byte encoding */
encode_op_stm(OpInstruction_t op_inst)
{
  int opcode = op_inst.opcode;
  struct RtypeOp Rop = op_inst.op.Rop;
  struct ItypeOp Iop = op_inst.op.Iop;
  struct JtypeOp Jop = op_inst.op.Jop;

  switch(OPCODE_TO_OPTYPE(op_inst.opcode)) {
    case OPTYPE_R:
      return ( Rop.funct << ENC_RTYPE_FUNCT_POS )
           | ( Rop.rd    << ENC_REG_RD_POS      )
           | ( Rop.rt    << ENC_REG_RT_POS      )
           | ( Rop.rs    << ENC_REG_RS_POS      )
           | ( opcode    << ENC_OPCODE_POS      );
    case OPTYPE_I:
      return ( Iop.immed  & ENC_IOP_IMMED_MASK  )
           | ( Iop.rt    << ENC_REG_RT_POS      )
           | ( Iop.rs    << ENC_REG_RS_POS      )
           | ( opcode    << ENC_OPCODE_POS      );
    case OPTYPE_J:
      return ( Jop.addr   & ENC_JOP_ADDR_MASK   )
           | ( Jop.reg   << ENC_JOP_REG_POS     )
           | ( opcode    << ENC_OPCODE_POS      );
  }

  return 0;
}


/*
 * Logs a label into the symbol table.
 * attr is the attribute of the label (bitwise-OR of SYM_DATA, SYM_CODE, SYM_ENTRY, SYM_EXTERN, SYM_REQUIRED).
 * On success, the symbol entry is either updated or added to the symbol table and 0 is returned.
 * On failure, the error id is returned.
 * Errors:
 *  ELABEL_SCOPE_MISMATCH - mismatch between entry/external attributes of current and previous encounter of the label.
 *  ELABEL_DOUBLE_DEF     - the label was already defined.
 *  ELABEL_EXT_DEF        - previously declared-external label is now defined.
 * Errors are printed to stdout using print_err, with line index line_ind.
 */
static int  /* nonzero on failure */
log_label(char *label, int attr, int line_ind)
{
  SymbolEntry_t *symbolp;
  SymbolEntry_t symbol;
  long offset = (attr & SYM_CODE) ? IC : (attr & SYM_DATA) ? DC : -1 * line_ind;
  Error_t error;
  error.errid = 0;
  error.line = NULL;
  error.tok.ind = -1;
  error.line_ind = line_ind;

  /* label already exists in symbol table */
  if(NULL != ( symbolp = search_symbol(label))) {
    if((symbolp->offset >= 0) && ((attr & SYM_CODE) || (attr & SYM_DATA))) {
      /* error - attempted label definition but label was already defined */
      error.errid = ELABEL_DOUBLE_DEF;
    } else {
      symbolp->attr |= attr;
      if( attr & SYM_CODE || attr & SYM_DATA
        || (symbolp->offset < 0 && attr & SYM_ENTRY)) {
        symbolp->offset = offset;
      }
    }
    if(symbolp->attr & SYM_EXTERN) {
      if(symbolp->attr & SYM_ENTRY) {
        /* error - entry/extern mismatch. */
        error.errid = ELABEL_SCOPE_MISMATCH;
      } else if(symbolp->attr & SYM_DATA || symbolp->attr & SYM_CODE) {
        /* error - external label defined. */
        error.errid = ELABEL_EXT_DEF;
      }
    }
  }
  /* label not encountered before */
  else {
    symbol.name = label;
    symbol.attr = attr;
    symbol.offset = offset;
    add_symbol(symbol);
  }

  if(error.errid != 0) {
    print_error(error);
    error_occurred = 1;
  }
  return error.errid;
}


/* 
 * Writes an instruction encoding into the instruction image and advances IC.
 * inst_enc - the encoded instruction to write.
 */
static void
write_instruction(int32_t inst_enc)
{
  int i=0;
  for(i=0; i<4; i++) {
    inst_img[IC++] = (inst_enc >> 8*i) & 0xFF;
  }
}


/* 
 * Writes a chunk of data into the memory image and advances DC.
 * data  - pointer to the data that would be written.
 * count - number of data chunks to write.
 * size  - size of one chunk of data.
 */
static void
write_memory(char *data, int count, int size)
{
  memcpy(&mem_img[DC], data, size*count);
  DC += size*count;
}


/*
 * Performs a directive.
 * For .entry or .extern directives:
 *  Logs the label into the symbol table with tha appropriate attributes.
 * For .asciz, .db, .dh or .dw directives:
 *  Writes the data into the memory image.
 */
static int  /* nonzero on failure */
perform_directive(Statement_t stm)
{
  Dir_t dir = stm.inst.di_inst.dir;
  char *label = dir.Sdir.label;
  switch(stm.inst.di_inst.dirid) {
    case DIR_ENTRY:
      return log_label(label, SYM_ENTRY, stm.line_ind);
    case DIR_EXTERN:
      return log_label(label, SYM_EXTERN, stm.line_ind);
    case DIR_ASCIZ:
      write_memory(dir.Sdir.str, strlen(dir.Sdir.str)+1, 1);
      break;
    case DIR_DB:
      write_memory(dir.Adir.argv, dir.Adir.argc, 1);
      break;
    case DIR_DH:
      write_memory(dir.Adir.argv, dir.Adir.argc, 2);
      break;
    case DIR_DW:
      write_memory(dir.Adir.argv, dir.Adir.argc, 4);
      break;
    default:
      break;
  }
  return 0;
}


/* 
 * Scans the array of assembly statements and handles all directives
 * and label definitions. As a results, both the program's memory image
 * and it's symbol table are assembled.
 * returns -1 if an error occured, else 0.
 */
void
write_memory_image(Statement_t *statements)
{
  int i;
  Statement_t stm;
  for(i=0; stm.type != STATEMENT_END; stm = statements[i++]) {
    switch(stm.type) {
      case STATEMENT_OPERATION:
        if(stm.label != NULL) /* statement contains label definition */
          log_label(stm.label, SYM_CODE, stm.line_ind);
        IC += 4;
        break;
      case STATEMENT_DIRECTIVE:
        if(stm.label != NULL
           && stm.inst.di_inst.dirid != DIR_ENTRY
           && stm.inst.di_inst.dirid != DIR_EXTERN)
          /* statement contains label definition */
          log_label(stm.label, SYM_DATA, stm.line_ind);
        perform_directive(stm);
      case STATEMENT_ERROR:
      default:
        break;
    }
  }
}

/*
 * Scans the array of assembly statements and handles all operations.
 * As a results, the program's instruction image is completed.
 * Prerequisite: the symbol table should be assembled beforehand.
 */
void
write_instruction_image(Statement_t *statements)
{
  int i;
  int32_t inst_enc;  /* instruction encoding */
  Statement_t stm = *statements;
  Op_t *op;
  Error_t error;
  error.errid = 0;
  error.line = NULL;
  error.tok.ind = -1;
  for(i=0; stm.type != STATEMENT_END; stm = statements[i++]) {
    op = &stm.inst.op_inst.op;
    if(stm.type == STATEMENT_OPERATION) {
      /* set label adderss for operations that require labels */
      switch(stm.inst.op_inst.opcode) {
        case OP_BNE:
        case OP_BEQ:
        case OP_BGT:
        case OP_BLT:
          error.errid = handle_branch_op(op);
          break;
        case OP_LA:
          error.errid = handle_la_op(op);
          break;
        case OP_JMP:
        case OP_CALL:
          error.errid = handle_jmp_op(op);
        default:
          break;
      }
      if(error.errid != 0) {
        error.line_ind = stm.line_ind;
        print_error(error);
      }
      inst_enc = encode_op_stm(stm.inst.op_inst);
      write_instruction(inst_enc);
    }
  }
  /* iterate over symbol table and check for  */
  check_symtable_integrity(&error);
  if(error.errid > 0)  /* errid is not a warning */
    error_occurred = 1;
}

/*
 * Iterates over the symbol table in search of symbols that were declared 
 * an entry but never defined - prints an error message for each.
 * Modifies the error pointer with the the error parameters if found.
 * Returns 0 if none symbols found, else ELABEL_ENT_UNDEF.
 */
static int  /* 1 if passed integrity check */
check_symtable_integrity(Error_t *error)
{
  int i;
  for(i=0; i<symtable_size; i++) {
    if((symtable[i].attr & SYM_ENTRY) && (symtable[i].offset < 0)) {
      error->errid = ELABEL_ENT_UNDEF;
      error->line_ind = symtable[i].offset * -1;
      print_error(*error);
    }
  }
  return error->errid;
}


/*
 * Handles symbol refrences that are part of branch operations (bne, beq, bgt, blt).
 */
static int  /* error id - nonzero on failure */
handle_branch_op(Op_t *op)
{
  SymbolEntry_t *symbol, symbol_req;
  symbol = search_symbol(op->Iop.label);
  if (symbol == NULL) {
    /* error - undefined label */
    return ELABEL_UNDEFINED;
  } else if (symbol->attr & SYM_EXTERN) {
    return ELABEL_UNEXP_EXT;
  } else if(symbol->attr & SYM_DATA) {
    /* error - branch to data label */
    return WLABEL_JMP2DATA;
  } else {
    op->Iop.immed = symbol->offset - IC;
    symbol_req.name = op->Iop.label;
    symbol_req.attr = symbol->attr | SYM_REQUIRED;
    symbol_req.offset = IC;
    add_symbol(symbol_req);
  }
  return 0;
}


/*
 * Handles symbol refrences that are part of a load adress (la) operation.
 */
static int  /* error id - nonzero on failure */
handle_la_op(Op_t *op)
{
  SymbolEntry_t *symbol, symbol_req;
  if(op->Jop.label == NULL) {
    return 0;
  }
  symbol = search_symbol(op->Jop.label);
  if (symbol == NULL) {
    /* error - undefined label */
    return ELABEL_UNDEFINED;
  } else if (!(symbol->attr & SYM_EXTERN) && !(symbol->attr & SYM_DATA)) {
    /* error - expected a data symbol */
    return ELABEL_EXP_DATA;
  } else {
    op->Jop.addr = symbol->attr & SYM_EXTERN ? 0 : symbol->offset + ICF + INITIAL_IC;
    symbol_req.name = op->Jop.label;
    symbol_req.attr = symbol->attr | SYM_REQUIRED;
    symbol_req.offset = IC;
    add_symbol(symbol_req);
  }
  return 0;
}

/*
 * Handles symbol refrences that are part of a jmp/call operation.
 */
static int  /* error id - nonzero on failure */
handle_jmp_op(Op_t *op)
{
  SymbolEntry_t *symbol, symbol_req;
  if(op->Jop.label == NULL) {
    return 0;
  }
  symbol = search_symbol(op->Jop.label);
  if (symbol == NULL) {
    /* error - undefined label */
    return ELABEL_UNDEFINED;
  } else if ((symbol->attr & SYM_DATA)) {
    /* error - expected code label */
    return WLABEL_JMP2DATA;
  } else {
    op->Jop.addr = symbol->attr & SYM_EXTERN ? 0 : symbol->offset + INITIAL_IC;
    symbol_req.name = op->Jop.label;
    symbol_req.attr = symbol->attr | SYM_REQUIRED;
    symbol_req.offset = IC;
    add_symbol(symbol_req);
  }
  return 0;
}
