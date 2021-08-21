/* ===== tables.c =========================================
 * This module contains definitons to 3 key data structures of the assembler as
 * well as methods for searching/modyfing them:
 * 1. operations - A static array of operation names and ids, used during tokenization
 *                 to match operation names to their id.
 * 2. directives - A static array of directive names and ids, used during tokenization
 *                 to match directive names to their id.
 * 3. symtable   - A dynamic array of symbols. Used during assembling to match
 *                 labels to addresses.
 */
/* ===== Includes ========================================= */
#include <stdlib.h>
#include <string.h>
#include "tables.h"
#include "types.h"
#include "consts.h"

/* ===== CPP definitons =================================== */
#define OPERATIONS_CNT (sizeof(operations) / sizeof(*operations))
#define DIRECTIVES_CNT (sizeof(directives) / sizeof(*directives))

/* ===== Declarations ===================================== */
/* ----- prototypes --------------------------------------- */
int search_op(char *term);
int search_dir(char *term);
void init_symtable();
void add_symbol(SymbolEntry_t symbol);
SymbolEntry_t* search_symbol(char *name);

static int op_cmp(const void *op1, const void *op2);
static int dir_cmp(const void *dir1, const void *dir2);


/* ===== Operations & directives tables =================== */
/* ----- Operations table ----------------------- */
struct op {
  char *name;
  enum OpId id;
} operations[] = {  /* sorted for efficient searching */
  {"add",   OP_ADD},
  {"addi",  OP_ADDI},
  {"and",   OP_AND},
  {"andi",  OP_ANDI},
  {"beq",   OP_BEQ},
  {"bgt",   OP_BGT},
  {"blt",   OP_BLT},
  {"bne",   OP_BNE},
  {"call",  OP_CALL},
  {"jmp",   OP_JMP},
  {"la",    OP_LA},
  {"lb",    OP_LB},
  {"lh",    OP_LH},
  {"lw",    OP_LW},
  {"move",  OP_MOVE},
  {"mvhi",  OP_MVHI},
  {"mvlo",  OP_MVLO},
  {"nor",   OP_NOR},
  {"nori",  OP_NORI},
  {"or",    OP_OR},
  {"ori",   OP_ORI},
  {"sb",    OP_SB},
  {"sh",    OP_SH},
  {"stop",  OP_STOP},
  {"sub",   OP_SUB},
  {"subi",  OP_SUBI},
  {"sw",    OP_SW}
};


/*
 * Lexicographic comparison function between operations to be used as a comparison key.
 */
static int
op_cmp(const void *op1, const void *op2)
{
  const struct op * const pop1 = op1;
  const struct op * const pop2 = op2; 
  return strcmp(pop1->name, pop2->name);
}

/*
 * Searches term in operations list,
 * if found - returns the operation id.
 * Else returns -1.
 */
int  /* the operation id */
search_op(char *term)
{
  struct op *op = bsearch(&term, operations, OPERATIONS_CNT, sizeof *operations, op_cmp);
  if (op == NULL)
    return -1;
  return op->id;
}

/* ----- Directives table ----------------------- */
struct dir {
  char *name;
  enum DirId id;
} directives[] = {  /* sorted for efficient searching */
  {"asciz",  DIR_ASCIZ},
  {"db",     DIR_DB},
  {"dh",     DIR_DH},
  {"dw",     DIR_DW},
  {"entry",  DIR_ENTRY},
  {"extern", DIR_EXTERN}
};

/*
 * Lexicographic comparison function between directives to be used as a comparison key.
 */
static int
dir_cmp(const void *dir1, const void *dir2)
{
  const struct dir * const pdir1 = dir1;
  const struct dir * const pdir2 = dir2; 
  return strcmp(pdir1->name, pdir2->name);
}

/*
 * Searches term in directives list,
 * if found - returns the directive id.
 * Else returns -1
 */
int  /* the directive id */
search_dir(char *term)
{
  struct dir *dir = bsearch(&term, directives, DIRECTIVES_CNT, sizeof *directives, dir_cmp);
  if (dir == NULL)
    return -1;
  return dir->id;
}


/* ===== Symbol table ===================================== */
static int symtable_maxsize;
SymbolEntry_t *symtable = NULL;
int symtable_size;

/*
 * Initializes the symbol table.
 * Discards previously stored symbols.
 */
void
init_symtable()
{
  symtable_maxsize = 2;
  symtable_size = 0;
  symtable = realloc(symtable, symtable_maxsize * sizeof(SymbolEntry_t));
}

/*
 * Frees up all memory used by the symbol table and symbols in it.
 */
void
cleanup_symtable()
{
  int i;
  for(i=0; i<symtable_size; i++) {
    free(symtable[i].name);
  }
  free(symtable);
}

/*
 * Adds a symbol to the symbol table.
 */
void
add_symbol(SymbolEntry_t symbol)
{
  if(symtable_size == symtable_maxsize) {
    symtable_maxsize *= 2;
    symtable = realloc(symtable, symtable_maxsize * sizeof(SymbolEntry_t));
  }
  symtable[symtable_size++] = symbol;
}

/*
 * Searches the given symbol name in the symbols table,
 * If found, returnes a pointer to it, else returns NULL.
 */
SymbolEntry_t*  /* pointer to the symbol if found */
search_symbol(char *name)
{
  int i;
  for(i=0; i<symtable_size; i++) {
    if(!(symtable[i].attr & SYM_REQUIRED)  && (strcmp(symtable[i].name, name) == 0)) {
      return &symtable[i];
    }
  }
  return NULL;
}
