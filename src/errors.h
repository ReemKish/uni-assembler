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

/* printable token names */
#define LABELDEF_TOK_NAME "label definition"
#define OP_TOK_NAME       "operation"
#define DIR_TOK_NAME      "directive"
#define REG_TOK_NAME      "register"
#define IMMED_TOK_NAME    "numeric literal"
#define STRING_TOK_NAME   "string"
#define LABEL_TOK_NAME    "label"

#define ERR_TABLE(X) \
	X(  	EUNKNOWN_TOK,          "unknown token") \
	X(  	EUNEXPECTED_TOK,       "unexpected token") \
	X(		EUNEXPECTED_EOL,       "unexpected end of line") \
	X(		EINVAL_DIR,            "unrecognized directive") \
	X(		EINVAL_REG,            "invalid register") \
	X(		EINVAL_IMMED,          "numeric literal out of bounds") \
	X(		EINVAL_LABEL,          "label name is a reserved word") \
	X(		ELONG_LABEL,           "label name exceeds character limit") \
	X(		ELONG_LINE,            "line exceeds character limit") \
	X(		ELABEL_UNDEFINED,      "refrence to undefined label") \
	X(		ELABEL_SCOPE_MISMATCH, "label defined as both external and an entry") \
	X(		ELABEL_EXT_DEF,        "label declared external but defined in file") \
	X(		ELABEL_DOUBLE_DEF,     "label defined more than once") \
	X(		ELABEL_EXP_DATA,       "expected a data symbol") \
	X(		ELABEL_EXP_CODE,       "expected a code label") \
	X(		ELABEL_ENT_UNDEF,      "label declared entry but not defined in file") \
	X(		ELABEL_UNEXP_EXT,      "external label operand to branch operation") \
  X(    ___WARNINGS___,        "") \
	X(		WLABEL_JMP2DATA,       "attempted jump to data symbol") \
	X(		WLABEL_DEF_ENTRY,      "redundant label definition on .entry statement") \
	X(		WLABEL_DEF_EXTERN,     "redundant label definition on .extern statement")


#define IS_SYNTAX_ERR(errid) ((errid) < ELABEL_UNDEFINED ? 1 : 0)
enum ErrId {
  __ErrStart__,/* enum starts from 1 since return value 0 is regarded as "no errors" */
  #define ERR_ID(errid, errstr) errid,
  ERR_TABLE(ERR_ID)
  #undef ERR_ID
  __ErrEnd__
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
