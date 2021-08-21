/* ===== errors.h =========================================
 * Header file for "errors.c".
 * Exposes the module's main function: print_error.
 * Defines the following:
 *  - Error strings of the various errors.
 *  - Printable token names for the various token types.
 *  - enum ErrId - an enumeration of the various error ids.
 *  - Error_t - the type used to describe a general error
 *             (wheter a syntax error, parsing error, etc.).
 */
#ifndef ERRORS_H
#define ERRORS_H


#include "types.h"

/* error strings */
#define EMSG_UNKNOWN_TOK          "unknown token"
#define EMSG_UNEXPECTED_TOK       "unexpected token"
#define EMSG_UNEXPECTED_EOL       "unexpected end of line"
#define EMSG_INVAL_DIR            "unrecognized directive"
#define EMSG_INVAL_REG            "invalid register"
#define EMSG_INVAL_IMMED          "numeric literal out of bounds"
#define EMSG_INVAL_LABEL          "label name is a reserved word"
#define EMSG_LONG_LABEL           "label name exceeds character limit"
#define EMSG_LONG_LINE            "line exceeds character limit"
#define EMSG_LABEL_UNDEFINED      "refrence to undefined label"
#define EMSG_LABEL_SCOPE_MISMATCH "label defined as both external and an entry"
#define EMSG_LABEL_EXT_DEF        "label declared external but defined in file"
#define EMSG_LABEL_DOUBLE_DEF     "label defined more than once"
#define EMSG_LABEL_EXP_DATA       "expected a data symbol"
#define EMSG_LABEL_EXP_CODE       "expected a code label"
#define EMSG_LABEL_ENT_UNDEF      "label declared entry but not defined in file"
#define EMSG_LABEL_UNEXP_EXT      "external label operand to branch operation"
#define WMSG_LABEL_JMP2DATA       "attempted jump to data symbol"

/* printable token names */
#define LABELDEF_TOK_NAME "label definition"
#define OP_TOK_NAME       "operation"
#define DIR_TOK_NAME      "directive"
#define REG_TOK_NAME      "register"
#define IMMED_TOK_NAME    "numeric literal"
#define STRING_TOK_NAME   "string"
#define LABEL_TOK_NAME    "label"

#define IS_SYNTAX_ERR(errid) ((errid) < ELABEL_UNDEFINED ? 1 : 0)
enum ErrId {
  /* enum starts from 1 since return value 0 is regarded as "no errors" */
  EUNKNOWN_TOK = 1,       /* unknown token          */
  EUNEXPECTED_TOK,        /* unexpected token       */
  EUNEXPECTED_EOL,        /* unexpected end of line */
  EINVAL_DIR,             /* invalid directive */
  EINVAL_REG,             /* invalid register  */
  EINVAL_IMMED,           /* invalid immediate */
  EINVAL_LABEL,           /* label name is a reserved word */
  ELONG_LABEL,            /* label name is too long */
  ELONG_LINE,             /* line is too long */
  ELABEL_UNDEFINED,       /* a label is refrenced yet undefined */
  ELABEL_SCOPE_MISMATCH,  /* a label is declared both entry and external   */
  ELABEL_EXT_DEF,         /* a label is both defined and declared external */
  ELABEL_DOUBLE_DEF,      /* a label is defined more than once             */
  ELABEL_EXP_DATA,        /* expected a data symbol                        */
  ELABEL_EXP_CODE,        /* expected a code label                         */
  ELABEL_ENT_UNDEF,       /* label declared entry but not defined in file  */
  ELABEL_UNEXP_EXT,       /* external label operand to branch operation    */
  WLABEL_JMP2DATA = -1    /* attempted jmp/branch to a data label          */
};
typedef struct Error {
  enum ErrId errid;  /* ID of the error            */
  Token_t tok;       /* the erroneous token        */
  char *line;        /* the erroneous line         */
  int line_ind;      /* the erroneous line's index */
  long flags;        /* error-specific flags       */
} Error_t;


void print_error(Error_t error);

#endif
