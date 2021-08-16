#ifndef ERRORS_H
#define ERRORS_H


#include "types.h"

/* error strings */
#define EMSG_UNKNOWN_TOK    "unknown token"
#define EMSG_UNEXPECTED_TOK "unexpected token"
#define EMSG_UNEXPECTED_EOL "unexpected end of line"
#define EMSG_INVAL_DIR      "unrecognized directive"
#define EMSG_INVAL_REG      "invalid register"
#define EMSG_INVAL_IMMED    "numeric literal out of bounds"
#define EMSG_INVAL_LABEL    "d"

/* printable token names */
#define LABELDEF_TOK_NAME "label definition"
#define OP_TOK_NAME       "operation"
#define DIR_TOK_NAME      "directive"
#define REG_TOK_NAME      "register"
#define IMMED_TOK_NAME    "numeric literal"
#define STRING_TOK_NAME   "string"
#define LABEL_TOK_NAME    "label"

enum ErrId {
  EUNKNOWN_TOK = 1,
  EUNEXPECTED_TOK,
  EUNEXPECTED_EOL,
  EINVAL_DIR,
  EINVAL_REG,
  EINVAL_IMMED,
  EINVAL_LABEL
};

typedef struct Error {
  enum ErrId errid;
  Token_t tok;
  char *line;
  long flags;
} Error_t;


void print_error(Error_t error, int line_ind);

#endif
