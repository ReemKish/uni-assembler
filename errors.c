/* ===== errors.c =========================================
 * This module contains methods for fomratting and outputting
 * the various errors into stdout.
 * All error ids are definited in the header file "errors.h".
 */

/* ===== Includes ========================================= */
#include <stdio.h>
#include <ctype.h>
#include "errors.h"
#include "parser.h"

/* ===== CPP definitons =================================== */
/* ANSI color escape sequences */
#define COLOR_RED_B     "\033[1;31m"  /* bold red      */
#define COLOR_WHITE_B   "\033[1;37m"  /* bold white    */
#define COLOR_PURPLE    "\033[0;35m"  /* purple        */
#define COLOR_PURPLE_B  "\033[1;35m"  /* bold purple        */
#define COLOR_RESET     "\033[0m"     /* default color */

/* padding macros to determine count of whitespace characters when printing errors */
#define PADDING1(x)   ((x) < 10 ? 2 : (x) < 100 ? 1 : 0)
#define PADDING2(x,y) (PADDING1(x) + PADDING1(y) - 1)

/* ===== Declarations ===================================== */
extern char *filename;  /* defined in "assembler.c" */
int error_occurred;     /* 0 iff no errors occured */

/* ----- prototypes --------------------------------------- */
void print_error(Error_t error);
static void print_errstr_unexpectedtok(long flags);
static void print_errstr(Error_t error);

/* ===== Code ============================================= */

/*
 * Pretty prints the unexpected token error to stdout.
 * The flags parameter is the flags field of the Error_t struct.
 * In this context, it is a bitwise-OR of the EXP_* flags defined in "parser.h"
 * and determines the info text that is provided in the error message.
 * e.g: for flags = EXP_REG | EXP_LABEL, the following error message is printed:
 *  "prog.as:1:4 error: expected a label or register"
 */
void
print_errstr_unexpectedtok(long flags)
{
  int i = 0;
  char *expected_toks[10], c;
  if(flags == EXP_END) {
    printf("unexpected token");
    return;
  }
  printf("expected ");
  if(flags & EXP_LABELDEF)
    expected_toks[i++] = LABELDEF_TOK_NAME;
  if(flags & EXP_REG)
    expected_toks[i++] = REG_TOK_NAME;
  if(flags & EXP_IMMED)
    expected_toks[i++] = IMMED_TOK_NAME;
  if(flags & EXP_STRING)
    expected_toks[i++] = STRING_TOK_NAME;
  if(flags & EXP_LABEL)
    expected_toks[i++] = LABEL_TOK_NAME;
  if(flags & EXP_DIR)
    expected_toks[i++] = DIR_TOK_NAME;
  if(flags & EXP_OP)
    expected_toks[i++] = OP_TOK_NAME;

  /* print 'a' or 'an' with respect to the first character of the next word
   * (wheter it is a vowel or not). Yes, probably very unnecessary. */
  c = tolower(expected_toks[--i][0]);
  (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u') ? printf("an ") : printf("a ");

  /* print the list of expected tokens */
  for(; i > 1; i--)
    printf("%s, ", (expected_toks[i]));
  if (i>0)
    printf("%s or ", (expected_toks[1]));
  printf("%s", (expected_toks[0]));
}

/*
 * Prints the error information string of error.
 * This is a part of the general error print (which includes the filename, position, etc.)
 */
void
print_errstr(Error_t error)
{
  enum ErrId errid = error.errid;
  switch(errid) {
    /* syntax errors */
    case EUNEXPECTED_EOL:
      printf(EMSG_UNEXPECTED_EOL "; ");
    case EUNKNOWN_TOK:
    case EUNEXPECTED_TOK:
      print_errstr_unexpectedtok(error.flags);
      break;
    case EINVAL_DIR:
            printf(EMSG_INVAL_DIR);
            break;
    case EINVAL_REG:
            printf(EMSG_INVAL_REG);
            break;
    case EINVAL_IMMED:
            printf(EMSG_INVAL_IMMED);
            break;
    case EINVAL_LABEL:
            printf(EMSG_INVAL_LABEL);
            break;
    case ELONG_LABEL:
            printf(EMSG_LONG_LABEL);
            break;
    case ELONG_LINE:
            printf(EMSG_LONG_LINE);
            break;
    /* parsing errors */
    case ELABEL_UNDEFINED:
            printf(EMSG_LABEL_UNDEFINED);
            break;
    case ELABEL_SCOPE_MISMATCH:
            printf(EMSG_LABEL_SCOPE_MISMATCH);
            break;
    case ELABEL_EXT_DEF:
            printf(EMSG_LABEL_EXT_DEF);
            break;
    case ELABEL_DOUBLE_DEF:
            printf(EMSG_LABEL_DOUBLE_DEF);
            break;
    case ELABEL_EXP_DATA:
            printf(EMSG_LABEL_EXP_DATA);
            break;
    case ELABEL_EXP_CODE:
            printf(EMSG_LABEL_EXP_CODE);
            break;
    case ELABEL_ENT_UNDEF:
            printf(EMSG_LABEL_ENT_UNDEF);
            break;
    case ELABEL_UNEXP_EXT:
            printf(EMSG_LABEL_UNEXP_EXT);
            break;
    case WLABEL_JMP2DATA:
            printf(WMSG_LABEL_JMP2DATA);
            break;
    }
}



/*
 * Pretty-prints the syntax error to stdout.
 * Includes the filename, erroneous line number, position in line,
 * the erroneous line itself and an error information message.
 * Also makes use of ANSI color escape sequences for colored output.
 */
void
print_error(Error_t error)
{
  int i;
  int padding;    /* used to align all error messages */
  int has_tok     /* 1 iff the error specifies an erroneous token       */
    = error.tok.ind >= 0;
  int has_line    /* 1 iff the error includes the erroneous line string */ 
    = error.line != NULL;
  int is_warning  /* 1 iff the error is non critical (a warning) */
    = error.errid < 0;

  /* error base (same for all errors) */
  printf(COLOR_WHITE_B"%s:%d:", filename, error.line_ind);

  /* decide padding */
  if(has_tok) {
    /* the error specifies the erroneous token */
    printf("%d:", error.tok.ind);
    padding = PADDING2(error.line_ind, error.tok.ind);
  } else { 
    /* the error does not specify the erroneous token */
    padding = PADDING1(error.line_ind);
  }
  for(i=0; i<padding; i++) printf(" ");

  /* print error/warning appropriatley */
  if(is_warning)
    printf(COLOR_PURPLE_B" warning:"COLOR_RESET" ");
  else
    printf(COLOR_RED_B" error:"COLOR_RESET" ");
  print_errstr(error);

  /* if provided, include the erroneous line */
  if(has_line) {
    printf("\n%4d | \t%s", error.line_ind, error.line);
    printf("     | \t");
  }

  /* if provided, specifiey the erroneous token */
  if(has_line && has_tok) {
    for (i=0; i<error.tok.ind-1; i++) printf(error.line[i] == '\t' ? "\t" : " ");
    printf(COLOR_PURPLE"^^^"COLOR_RESET);  
  }
  printf("\n");
}
