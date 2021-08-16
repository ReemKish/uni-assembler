#include <stdio.h>
#include <ctype.h>
#include "errors.h"
#include "parser.h"

extern char *filename;

#define COLOR_RED_B   "\033[1;31m"
#define COLOR_WHITE_B "\033[1;37m"
#define COLOR_PURPLE  "\033[0;35m"
#define COLOR_RESET   "\033[0m"

void print_errstr_unexpectedtok(Error_t error)
{
  long flags = error.flags;
  int i = 0;
  char *expected_toks[10], c;
  if(flags == EXP_END) {
    printf("unexpected token\n");
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

  c = tolower(expected_toks[--i][0]);
  (c == 'a' || c == 'e' || c == 'i' || c == 'o' || c == 'u') ? printf("an ") : printf("a ");
  for(; i > 1; i--) {
    printf("%s, ", (expected_toks[i]));
  }
  if (i>0) {
  printf("%s or ", (expected_toks[1]));
  }
  printf("%s", (expected_toks[0]));
}

void print_errstr(Error_t error)
{
  enum ErrId errid = error.errid;
  switch(errid) {
    case EINVAL_IMMED:
      printf(EMSG_INVAL_IMMED);
      break;
    case EINVAL_LABEL:
      printf(EMSG_INVAL_LABEL);
      break;
    case EINVAL_DIR:
      printf(EMSG_INVAL_DIR);
      break;
    case EINVAL_REG:
      printf(EMSG_INVAL_REG);
      break;
    case EUNEXPECTED_EOL:
      printf(EMSG_UNEXPECTED_EOL "; ");
    case EUNKNOWN_TOK:
    case EUNEXPECTED_TOK:
      print_errstr_unexpectedtok(error);
      break;
  }
}


/* Pretty-prints the error to stdout.
 **/
void print_error(Error_t error, int line_ind)
{
  int i;
  printf(COLOR_WHITE_B"%s:%d:%d:"COLOR_RED_B" error:"COLOR_RESET" ",
         filename, line_ind, error.tok.ind);
  print_errstr(error);
  printf("\n%4d | \t%s", line_ind, error.line);
  printf("     | \t");
  for (i=0; i<error.tok.ind-1; i++) printf(error.line[i] == '\t' ? "\t" : " ");
  printf(COLOR_PURPLE"^^^\n"COLOR_RESET);
}
