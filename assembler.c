#include <stdio.h>
#include <error.h>
#include <errno.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>
#include "scan1.h"
#include "parser.h"
#include "consts.h"
#include "tables.h"


#define HELP_TEXT   "usage: %s file1 [file2] [file3] ..."
#define NOARGS_ERR  "missing argument"

char *filename, *filepath;
long IC, DC, ICF, DCF;
char inst_img[MAX_PROG_LINES * 4];  /* instruction image - 4 bytes per instruction */
char mem_img[MAX_PROG_MEMORY];
extern SymbolEntry_t *symtable ;
extern int symtable_size;

const char *get_file_ext(const char *path)
{
  const char *dot = strrchr(path, '.');
  if(!dot || dot == path) {
    return "";
  }
  return dot;
}

/* Returns a copy of path where the file addressed by it has new file extension 'ext'.
 * e.g: modify_file_ext("/tmp/prog.as", ".ob") -> "/tmp/prog.ob"
 * Prerequisite: path must end with a file extension.
 **/
char *modify_file_ext(const char *path, const char *ext)
{
  char *newpath = malloc(strlen(path) + strlen(ext));
  const char *dot = get_file_ext(filename);
  memcpy(newpath, path, dot - path);
  memcpy(&newpath[dot - path], ext, strlen(ext) + 1);
  return newpath;

}

/* returns the nth byte in the combined image of instructions and memory.
 **/
char get_img_byte(int n) {
  if(n < ICF) {
    return inst_img[n];
  }
  return mem_img[n-ICF];
}

void write_ob_file()
{
  char *obfilename = modify_file_ext(filepath, ".ob");
  FILE *obfile = fopen(obfilename, "w");
  int i;

  /* header line */
  fprintf(obfile, "     %lu %lu\n%04d ", ICF, DCF, INITIAL_IC);
  /* bytes */
  for(i=0; i < DCF + ICF - 1; i++) {
    fprintf(obfile, "%02X", get_img_byte(i) & 0xFF);
    if(i % 4 == 3) /* 4th and last byte in line */
      fprintf(obfile, "\n%04d ", i+INITIAL_IC+1);
    else fprintf(obfile, " ");
  }
  fprintf(obfile, "%02X\n", get_img_byte(ICF+DCF-1) & 0xFF);
}


void write_ext_file()
{
  char *extfilename = modify_file_ext(filepath, ".ext");
  FILE *extfile = fopen(extfilename, "w");
  SymbolEntry_t symbol;
  int i;
  for(i=0; i<symtable_size; i++) {
    symbol = symtable[i];
    if((symbol.attr & SYM_REQUIRED) && (symbol.attr & SYM_EXTERN)) {
      fprintf(extfile, "%s %04ld\n", symbol.name,
          symbol.offset + INITIAL_IC + (symbol.attr & SYM_DATA ? ICF : 0)
          );
    }
  }

}

void write_ent_file()
{
  char *entfilename = modify_file_ext(filepath, ".ent");
  FILE *entfile = fopen(entfilename, "w");
  SymbolEntry_t symbol;
  int i;
  for(i=0; i<symtable_size; i++) {
    symbol = symtable[i];
    if((symbol.attr & SYM_ENTRY) && !(symbol.attr & SYM_REQUIRED)) {
      fprintf(entfile, "%s %04ld\n", symbol.name,
          symbol.offset + INITIAL_IC + (symbol.attr & SYM_DATA ? ICF : 0)
          );
    }
  }

}

void assemble(FILE *source)
/* assembles source file.
 * if the source code is valid:
 *    writes .ob and .ext, .ent files if relevant. *
 * else:
 *  prints all syntax errors in file and writes none files
 */
{
  /* init */
  Statement_t *statements = parse_file(source);
  IC = 0; DC = 0;
  memset(inst_img, 0, sizeof(inst_img));
  memset(mem_img, 0, sizeof(mem_img));

  write_memory_image(statements);
  ICF = IC; DCF = DC;
  IC = 0; DC = 0;
  write_instruction_image(statements);

  write_ob_file(ICF, DCF);
  write_ext_file();
  write_ent_file();

}

int main(int argc, char** argv)
{

  int exit_status;
  size_t i;
  FILE *file;

  if (argc < 2)  /* no cmdline arguments - print error and exit */
    error(EXIT_FAILURE, 0, NOARGS_ERR"\n"HELP_TEXT, argv[0]);

  exit_status = 1;  /* =1 iff all files failed to open, else 0 */
  for (i=1; i<argc; i++) {

    filepath = argv[i];
    filename = basename(filepath);
    if (NULL == (file = fopen(filepath, "r")))
      /* file won't open - print error message and skip it */
      error(0, errno, "%s", argv[i]);
    else {
      exit_status = 0;
      assemble(file);
    }
  }

  return exit_status;
}

