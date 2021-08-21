/* ===== tables.h =========================================
 * Header file for "tables.c".
 * Defines the SymbolEntry_t type for symbol entries in the symbol table.
 * Exposes the following:
 *  add_symbol, search_symbol functions of the symbol table.
 *  search_op, search_dir functions of the operation and directive tables.
 */
#ifndef TABLES_H
#define TABLES_H


#include <stdlib.h>
#include <stdint.h>

#define SYM_REQUIRED (1 << 0)  /* symbol is an operand of some operation, e.g: call Func  */
#define SYM_EXTERN   (1 << 1)  /* symbol is declared external                             */
#define SYM_ENTRY    (1 << 2)  /* symbol is declared as an entry                          */
#define SYM_CODE     (1 << 3)  /* symbol is defined at an operation statement             */
#define SYM_DATA     (1 << 4)  /* symbol is defined at a directive statement              */

typedef struct SymbolEntry {
  char *name;
  int32_t offset;
  int attr;       /* bitwise-OR of SYM_DATA, SYM_CODE, SYM_ENTRY, SYM_EXTERN, SYM_REQUIRED */
} SymbolEntry_t;
void init_symtable();
void cleanup_symtable();
void add_symbol(SymbolEntry_t symbol);
SymbolEntry_t* search_symbol(char *name);

int search_op(char *tok);
int search_dir(char *tok);


#endif
