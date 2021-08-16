#ifndef PARSER_H
#define PARSER_H


#include "types.h"
#include <stdio.h>

/* ===== CPP Definitions ================================== */
/* ----- flags -------------------------------------------- */
#define EXP_END       (1 << 1)
#define EXP_COMMENT   (1 << 2)
#define EXP_LABELDEF  (1 << 3)
#define EXP_LABEL     (1 << 4)
#define EXP_OP        (1 << 5)
#define EXP_DIR       (1 << 6)
#define EXP_REG       (1 << 7)
#define EXP_IMMED     (1 << 8)
#define EXP_STRING    (1 << 9)
#define REG_RS        (1 << 10)
#define REG_RT        (1 << 11)
#define REG_RD        (1 << 12)

Statement_t* parse_file(FILE *file);


#endif
