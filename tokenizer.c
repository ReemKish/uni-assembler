/* ===== tokenizer.c ======================================
 * This module is responsible for processing the raw input (lines of the source file)
 * into meaningful tokens. The way this is done is via the main function next_token which
 * receives a line of assembly source code and outputs the next token in it with each
 * successive call.
 * Each token has a type, and stores a value corresponding to that type.
 * e.g: a token of type REG_TOK stores the id of a register.
 * The Token_t data structure and the enumeration of the token types are defined in "types.h".
 */

/* ===== Includes ========================================= */
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>
#include "tables.h"
#include "types.h"
#include "consts.h"


/* ===== CPP definitons =================================== */
#define COMMENT_CHAR ';'
#define WSPACE_CHARS " \f\n\r\t\v"

/* ===== Declarations ===================================== */
static long expect;

/* ----- prototypes --------------------------------------- */
Token_t next_token(char *line);
static Token_t tokenize_term(char *term);
static char *  next_term(char *line);
static char * next_string(char *line);
static char* next_array_item(char *line);
static char* strip_wspace(char *term);

static int tokenize_op(char *term, TokVal_t *tokval);
static int tokenize_labeldef(char *term, TokVal_t *tokval);
static int tokenize_label(char *term, TokVal_t *tokval);
static int tokenize_reg(char *term, TokVal_t *tokval);
static int tokenize_immed(char *term, TokVal_t *tokval);

static int is_string(char *term);
static int is_label(char *term);
static int is_labeldef(char *term);
static int parse_reg_term(char *term);


/* ===== CPP Definitions ================================== */
/* expect values, see flags handling of next_term */
#define EXP_STRING  (1)
#define EXP_ARRAY   (2)

/* ===== Code ============================================= */

/* 
 * Attempts to process the term as an operation token,
 * on success updates tokval with the operation id and returns 0.
 * On failure tokval is not modified and -1 is returned.
 */
static int /* nonzero on failure */
tokenize_op(char *term, TokVal_t *tokval)
{
  enum OpId opid;
  if(-1 == (opid = search_op(term))) {
    return -1;
  }
  tokval->opid = opid;
  if(opid != OP_STOP) expect = EXP_ARRAY;
  return 0;
}


/*
 * Attempts to process the term as a directive token,
 * all directives start with '.', hence, if the term doesn't
 * start with '.', -1 is returned and the token isn't modified.
 * Else if the term isn't a recognized directive, -2 is returned
 *  and the token's dirid is set to DIR_INVALID.
 * Else, the term is a recognized directive and therefore 0 is returned.
 */
static int  /* nonzero on failure */
tokenize_dir(char *term, TokVal_t *tokval)
{
  enum DirId dirid;
  if(term[0] != '.') {
    /* term doesn't start with a dot and hence isn't a directive at all */
    return -1;
  }
  if(-1 == (dirid = search_dir(&term[1]))) {
    /* term isn't a recognized directive */
    tokval->dirid = DIR_INVALID;
    return -2;
  }
  /* term is a recognized directive */
  tokval->dirid = dirid;
  if(dirid == DIR_ASCIZ) {
    expect = EXP_STRING;
  } else if(dirid == DIR_DB || dirid == DIR_DW || dirid == DIR_DH) {
    expect = EXP_ARRAY;
  }
  return 0;
}


/* 
 * Attempts to process the term as a string token,
 * on success updates tokval with the string (without the quotes) and returns 0.
 * On failure tokval is not modified and -1 is returned.
 */
static int  /* nonzero on failure */
tokenize_string(char *term, TokVal_t *tokval)
{
  int len = strlen(term);
  if (!is_string(term)) {
    return -1;
  }
  term[len-1] = '\0';
  tokval->str = malloc(len-1);
  strcpy(tokval->str, &term[1]);
  term[len-1] = '"';
  return 0;
}


/*
 * Attempts to process the term as a labeldef token,
 * on success updates tokval with the label string and returns 0.
 * On failure tokval is not modified and -1 is returned.
 */
static int  /* nonzero on failure */
tokenize_labeldef(char *term, TokVal_t *tokval)
{
  if (!is_labeldef(term)) {
    return -1;
  }
  tokval->label = malloc(strlen(term));
  strncpy(tokval->label, term, strlen(term)-1);
  return 0;
}


/*
 * Attempts to process the term as a label token,
 * on success updates tokval with the label string and returns 0.
 * On failure tokval is not modified and -1 is returned.
 */
static int  /* nonzero on failure */
tokenize_label(char *term, TokVal_t *tokval)
{
  if (!is_label(term)) {
    return -1;
  }
  tokval->label = malloc(strlen(term)+1);
  strcpy(tokval->label, term);
  return 0;
}


/* 
 * Attempts to process the term as a register token,
 * on success updates tokval with the register id and returns 0.
 * On failure tokval is not modified and -1 is returned.
 */
static int  /* nonzero on failure */
tokenize_reg(char *term, TokVal_t *tokval)
{
  int reg;
  if(-1 == (reg = parse_reg_term(term))) {
    return -1;
  }
  tokval->reg = reg;
  return 0;
}


/* 
 * Attempts to process the term as an immediate token,
 * on success updates tokval with the immediate value and returns 0.
 * On failure tokval is not modified and -1 is returned.
 */
static int  /* nonzero on failure */
tokenize_immed(char *term, TokVal_t *tokval)
{

  long immed;
  char *endptr;
  immed = strtol(term, &endptr, 10);
  if(*endptr != '\0') {
    return -1;
  }

  tokval->immed = immed;
  return 0;
}


/* 
 * A string begins and ends with a double quotation mark and consists only of printable chars.
 * Returns 0 iff the term is not a valid string.
 */
static int  /* nonzero on failure */
is_string(char *term)
{

  int i, len = strlen(term);
  if (len < 2 || !(term[0] == '"') || !(term[len-1] == '"')) {
    return 0;
  }
  for(i=0; i<len; i++) {
    if(!isprint(term[i])) {
      return 0;
    }
  }
  return 1;
}


/* 
 * A label begins with an alphabet letter followed by a sequence of alphanumeric characters.
 * Returns 0 iff the term is not a valid label.
 */
static int  /* nonzero on failure */
is_label(char *label)
{
  int i;
  if(!isalpha(label[0])) {
    return 0;
  }
  for(i=1; i<strlen(label); i++) {
    if(!isalnum(label[i])) {
      return 0;
    }
  }
  return 1;
}


/* 
 * A label definition consists of a label and a colon (':') at the end.
 * Returns 0 iff the term is not a valid init label.
 */
static int  /* nonzero on failure */
is_labeldef(char *term)
{
  int len = strlen(term);
  /* a label token must end with ':'*/
  if(term[len-1] != ':') {
    return 0;
  }

  term[len-1] = '\0';
  if(!is_label(term)) {
    term[len-1] = ':';
    return 0;
  }
  term[len-1] = ':';
  return 1;
}


/* 
 * A register consists of the char '$' followed by a number.
 *  e.g.: $0, $1, $2, ..., $31
 * If `reg` does not begin with a '$', returns -1.
 * Else returns its id (the number after the '$') if it is valid, or -2 otherwise.
 */
static int  /* nonzero on failure */
parse_reg_term(char *term)
{
  /* a register must begin with '$' */
  if(term[0] != '$') {  /* not a register term */
    return -1;
  } else if(strlen(term) == 2) {
    return term[1] - '0';
  } else if(strlen(term) == 3 && term[1] != '0') {
    return (term[1] - '0') * 10 + term[2] - '0';
  } else {  /* a register term with invalid id. */
    return -2;
  }
}


/*
 * Returns the appropriate token for the given term.
 * If no other token type matches the term, a token with
 * token type TOK_ERR is returned.
 */
static Token_t  /* the token that matches the term */
tokenize_term(char *term)
{
  Token_t token;
  token.type = TOK_ERR;
  if(term == NULL) {
    token.type = TOK_END; 
  } else if(*term == '\0') {  /* empty string */
    token.type = TOK_EMPTY; 
  } else if(term[0] == COMMENT_CHAR) {
    token.type = TOK_COMMENT;
  } else if(-1 != tokenize_op(term, &token.value)) {
    token.type = TOK_OP;
  } else if(-1 != tokenize_dir(term, &token.value)) {
    token.type = TOK_DIR;
  } else if(-1 != tokenize_reg(term, &token.value)) {
    token.type = TOK_REG;
  } else if(-1 != tokenize_immed(term, &token.value)) {
    token.type = TOK_IMMED;
  } else if(-1 != tokenize_string(term, &token.value)) {
    token.type = TOK_STRING;
  } else if(-1 != tokenize_label(term, &token.value)) {
    token.type = TOK_LABEL;
  } else if(-1 != tokenize_labeldef(term, &token.value)) {
    token.type = TOK_LABELDEF;
  }

  return token;
}


/* 
 * Returns the next token in line, which can be then
 * used to parse an assembly statement from that line.
 * Usage:
 *  Like strtok, the first call should provide the actual line pointer
 *  and all successive calls that operate on the same line should
 *  provide NULL as the line pointer.
 *  Returns the next token (if there are no more tokens, a token of type TOK_END is returned).
 */
Token_t /* the next token in line */
next_token(char *line)
{
  static char *next;
  static char *source;
  char *term;
  Token_t tok;
  int ind;

  if(line != NULL) {
    source = line;
    next = source;
    expect = 0;
  }

  ind = next - source;
  term = next_term(next);
  if(term != NULL) {
    ind = term - source;
    next = term + strlen(term) + 1;
  }

  tok = tokenize_term(strip_wspace(term));
  tok.ind = ind;

  return tok;
}


/* 
 * Obtains and returns one term from the beginning of the line.
 * If none flags are set, the default delimiter is "\t \n" (tabs, spaces & newlines)
 * returns NULL when there are no terms (line containts only delimiter characters).
 * flags affect the returned term as follows:
 * EXP_STRING - Respects quotation marks by altering the default delimiter.
 *      i.e., '"hello world"' is recognized as one term - '"hello world"',
 *      instead of two terms: '"hello' and 'world"'
 * EXP_ARRAY - Changes the delimiter to ','. array syntax is as follows:
 *      terms are separated by exactly one comma (',') optionally surrounded by whitespace
 *      characters.
 */
static char*  /* the next term in line */
next_term(char *line)
{
  line = strip_wspace(line);
  /* expect string */
  if(expect == EXP_STRING) {
    return next_string(line);
  }
  /* expect array item */
  else if(expect == EXP_ARRAY) {
    return next_array_item(line);
  }

  return strtok(line, WSPACE_CHARS);
}


/*
 * Returns the next term under the assumption that it is a string.
 * See the handling of EXP_STRING flag in function next_term.
 */
static char*  /* the next string */
next_string(char *line)
{
  int i;
  if(line[0] != '"') {
    return NULL;
  }
  for(i=strlen(line)-1; i>0 && line[i] != '"'; i--) {}
  if(i <= 0  || (line[i+1] != '\0' && !isspace(line[i+1]))) {
    return NULL;
  } else {
    line[i+1] = '\0';
    return line;
  }
}


/*
 * Returns the next term under the assumption that it is part of an array.
 * See the handling of EXP_ARRAY flag in function next_term.
 */
static char*  /* the next array item */
next_array_item(char *line)
{
  int j;
  if(*line == '\0') {
    expect = 0;
    return line;
  }
  for(j=0; line[j] != ',' && line[j] != '\0'; j++) {}  /* index of the first ',' char */
  if(line[j] == '\0') {
    expect = 0;
  }
  line[j] = '\0';
  return line;
}


/*
 * Strips the term of both trailing and leading whitespace characters.
 */
static char*  /* the stripped term */
strip_wspace(char *term)
{
  int i,j;
  if(term == NULL) {
    return NULL;
  }
  for(i=0; isspace(term[i]) ; i++);
  for(j=strlen(term)-1; isspace(term[j]); j--);
  term[++j] = '\0';
  return term + i;
}
