/* ===== parser.h =========================================
 * Header file for "parser.c".
 * Contains definition for EXP_* flags used both in "parser.c" and in "errors.c".
 * Exposes the parser's main function: parse_file
 */
#ifndef PARSER_H
#define PARSER_H


#include <stdio.h>
#include "types.h"

/* ===== CPP Definitions ================================== */
/* ----- flags -------------------------------------------- */
#define EXP_END       (1 << 1)    /* expect end of line               */
#define EXP_COMMENT   (1 << 2)    /* expect a comment                 */
#define EXP_LABELDEF  (1 << 3)    /* expect a label definition        */
#define EXP_LABEL     (1 << 4)    /* expect a label reference         */
#define EXP_OP        (1 << 5)    /* expect an operation              */
#define EXP_DIR       (1 << 6)    /* expect a directive               */
#define EXP_REG       (1 << 7)    /* expect a register                */
#define EXP_IMMED     (1 << 8)    /* expect an immediate              */
#define EXP_STRING    (1 << 9)    /* expect a string immediate        */
#define REG_RS        (1 << 10)   /* currently parsed register is RS  */
#define REG_RT        (1 << 11)   /* currently parsed register is RT  */
#define REG_RD        (1 << 12)   /* currently parsed register is RD  */

Statement_t* parse_file(FILE *file);


#endif
