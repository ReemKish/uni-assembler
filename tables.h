#ifndef TABLES_H
#define TABLES_H


#include <stdint.h>


#define SYM_REQUIRED (1 << 0) 
#define SYM_EXTERN   (1 << 1)
#define SYM_ENTRY    (1 << 2)
#define SYM_CODE     (1 << 3)
#define SYM_DATA     (1 << 4)

typedef struct SymbolEntry {
  char *name;
  int32_t offset;
  int attr;       /* bitwise-OR of SYM_DATA, SYM_CODE, SYM_ENTRY, SYM_EXTERNAL */
} SymbolEntry_t;

int search_op(char *tok);
int search_dir(char *tok);

SymbolEntry_t* search_symbol(char *name);
void add_symbol(SymbolEntry_t symbol);


#endif
