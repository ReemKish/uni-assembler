#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "parser.h"
#include "consts.h"
#include "types.h"
#include "tables.h"

extern char inst_img[], mem_img[];
extern long IC, DC, ICF;


int32_t encode_op_stm(Statement_t stm)
{
  OpInstruction_t op_inst = stm.inst.op_inst;
  struct RtypeOp Rop = op_inst.op.Rop;
  struct ItypeOp Iop = op_inst.op.Iop;
  struct JtypeOp Jop = op_inst.op.Jop;

  switch(OPCODE_TO_OPTYPE(op_inst.opcode)) {
    case OPTYPE_R:
      return (Rop.funct << 6) | (Rop.rd << 11) |
             (Rop.rt << 16)   | (Rop.rs << 21) |
             (op_inst.opcode << 26);
    case OPTYPE_I:
      return (Iop.immed & 0xFFFF)            |
             (Iop.rt << 16) | (Iop.rs << 21) |
             (op_inst.opcode << 26);
    case OPTYPE_J:
      return (Jop.addr & 0x1FFFFFF) |  (Jop.reg << 25) |
             (op_inst.opcode << 26);
  }

  return 0;
}


/* Logs a label into the symbol table.
 * attr is the attribute of the label and may be either SYM_CODE or SYM_DATA.
 * On success, the symbol entry is either updated or added to the symbol table and 0 is returned.
 * On failure, the error id is returned.
 * Errors:
 *  A - mismatch between entry/external attributes of current and previous encounter of the label.
 *  B - the label was already defined.
 *  C - previously declared-external label is now defined.
 **/
int log_label(char *label, int attr)
{
  SymbolEntry_t *symbolp;
  SymbolEntry_t symbol;
  long offset = (attr & SYM_CODE) ? IC : (attr & SYM_DATA) ? DC : -1;

  /* label already exists in symbol table */
  if(NULL != ( symbolp = search_symbol(label))) {
    if((symbolp->offset >= 0) && ((attr & SYM_CODE) || (attr & SYM_DATA))) {
      /* attempted label definition but label was already defined */
      /* TODO - error, multiple label definitions */
    } else { symbolp->attr |= attr;
      if(attr & SYM_CODE || attr & SYM_DATA) {
        symbolp->offset = offset;
      }
    }
    if(symbolp->attr & SYM_EXTERN) {
      if(symbolp->attr & SYM_ENTRY) {
        /* TODO - error, entry/extern mismatch. */
      }
      if(symbol.attr & SYM_DATA || symbol.attr & SYM_CODE) {
        /* TODO - error, external label defined. */
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
  return 0;
}

/* Writes an instruction encoding into the instruction image and advances IC.
 * inst_enc - the encoded instruction to write.
 * */
void write_instruction(int32_t inst_enc)
{
  int i=0;
  for(i=0; i<4; i++) {
    inst_img[IC++] = (inst_enc >> 8*i) & 0xFF;
  }
}

/* Writes a chunk of data into the memory image and advances DC.
 * data  - pointer to the data that would be written.
 * count - number of data chunks to write.
 * size  - size of one chunk of data.
 * */
void write_memory(char *data, int count, int size)
{
  memcpy(&mem_img[DC], data, size*count);
  DC += size*count;
}

int perform_directive(Statement_t stm)
{
  Dir_t dir = stm.inst.di_inst.dir;
  char *label = dir.Sdir.label;
  switch(stm.inst.di_inst.dirid) {
    case DIR_ENTRY:
      log_label(label, SYM_ENTRY);
      break;
    case DIR_EXTERN:
      log_label(label, SYM_EXTERN);
      break;
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
  
}


/* scans the array of assembly statements and handles all directives
 * and label definitions. As a results, both the program's memory image
 * and it's symbol table are assembled;
 * */
int write_memory_image(Statement_t *statements)
{
  int i;
  Statement_t stm;
  for(i=0; stm.type != STATEMENT_END; stm = statements[i++]) {
    switch(stm.type) {
      case STATEMENT_OPERATION:
        if(stm.label != NULL) /* statement contains label definition */ {
          log_label(stm.label, SYM_CODE);
        } 
        IC += 4;
        break;
      case STATEMENT_DIRECTIVE:
        if(stm.label != NULL) /* statement contains label definition */ {
          log_label(stm.label, SYM_DATA);
        } 
        perform_directive(stm);
        break;
      case STATEMENT_ERROR:
      default:
        break;
    }
  }
  return 0;
}

/* scans the array of assembly statements and handles all operations.
 * As a results, the program's instruction image is completed.
 * Prerequisite: the symbol table should be assembled beforehand.
 * */
int write_instruction_image(Statement_t *statements)
{
  int i;
  int32_t inst_enc;  /* instruction encoding */
  Statement_t stm = *statements;
  SymbolEntry_t *symbol, symbol_req;
  Op_t *op;
  for(i=0; stm.type != STATEMENT_END; stm = statements[i++]) {
    op = &stm.inst.op_inst.op;
    if(stm.type == STATEMENT_OPERATION) {
      /* set label adderss for operations that require labels */
      switch(stm.inst.op_inst.opcode) {
        case OP_BNE:
        case OP_BEQ:
        case OP_BGT:
        case OP_BLT:
          symbol = search_symbol(op->Iop.label);
          if (symbol == NULL) {
            /* TODO - error, undefined label */
          } else if(symbol->attr & SYM_EXTERN) {
            /* TODO - error, branch to external label */
          } else if(symbol->attr & SYM_DATA) {
            /* TODO - error, branch to data label */
          } else {
            op->Iop.immed = symbol->offset - IC;
            symbol_req.name = op->Iop.label;
            symbol_req.attr = symbol->attr | SYM_REQUIRED;
            symbol_req.offset = IC;
            add_symbol(symbol_req);
          }
          break;
        case OP_LA:
          if(op->Jop.label == NULL) {
            break;
          }
          symbol = search_symbol(op->Jop.label);
          if (symbol == NULL) {
            /* TODO - error, undefined label */
          } else if (!(symbol->attr & SYM_EXTERN) && !(symbol->attr & SYM_DATA)) {
            /* TODO - error, unexpected label type */
            printf("%s\n",symbol->name);
          } else {
            op->Jop.addr = symbol->attr & SYM_EXTERN ? 0 : symbol->offset + ICF + INITIAL_IC;
            symbol_req.name = op->Jop.label;
            symbol_req.attr = symbol->attr | SYM_REQUIRED;
            symbol_req.offset = IC;
            add_symbol(symbol_req);
          }
          break;
        case OP_JMP:
        case OP_CALL:
          if(op->Jop.label == NULL) {
            break;
          }
          symbol = search_symbol(op->Jop.label);
          if (symbol == NULL) {
            /* TODO - error, undefined label */
          } else if (!(symbol->attr & SYM_EXTERN) && !(symbol->attr & SYM_CODE)) {
            /* TODO - error, unexpected label type */
          } else {
            op->Jop.addr = symbol->attr & SYM_EXTERN ? 0 : symbol->offset + INITIAL_IC;
            symbol_req.name = op->Jop.label;
            symbol_req.attr = symbol->attr | SYM_REQUIRED;
            symbol_req.offset = IC;
            add_symbol(symbol_req);
          }
        default:
          break;
      }
      inst_enc = encode_op_stm(stm);
      write_instruction(inst_enc);
    }
  }
  return 0;
}
