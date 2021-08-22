/* ===== assembler.c ======================================
 * The main source file of the assembler.
 * - Handles cmdline parameter parsing (opening files).
 * - Defines the instruction and memory images & assembles them by calling "scan.c".
 * - Creates and writes the '.ob', '.ent', '.ext' files.
 */

/* ===== Includes ========================================= */
#include <stdio.h>
#include <error.h>
#include <errno.h>
#include <stdlib.h>
#include <libgen.h>
#include <string.h>
#include "parser.h"
#include "scan.h"
#include "tables.h"
#include "consts.h"


/* ===== CPP definitons =================================== */
#define HELP_TEXT   "usage: %s file1 [file2] [file3] ..."
#define NOARGS_ERR  "missing argument"
#define EXT_ERR     "%s: %s: source file extension must be .as"

/* ===== Declarations ===================================== */
char *filename, *filepath;
long IC, DC, ICF, DCF;
char inst_img[MAX_PROG_LINES * 4];  /* instruction image - 4 bytes per instruction */
char mem_img[MAX_PROG_MEMORY];

/* defined in tables.c */
extern SymbolEntry_t *symtable ;
extern int symtable_size;  
extern int error_occurred;  /* defined in "errors.c" */

/* ----- prototypes --------------------------------------- */
const char* get_file_ext(const char *path);
char* modify_file_ext(const char *path, const char *ext);
char get_img_byte(int n);
void write_ob_file();
void write_ext_file();
void write_ent_file();
int assemble(FILE *source);
int main(int argc, char** argv);

/* ===== Code ============================================= */

/*  
 * Returns the file extension of the file pointed to by path.
 * e.g: get_file_ext("/tmp/prog.as") -> ".as"
 */
const char*
get_file_ext(const char *path)
{
  const char *dot = strrchr(path, '.');
  if(!dot || dot == path) {
    return "";
  }
  return dot;
}

/* 
 * Returns a copy of path where the file addressed by it has new file extension 'ext'.
 * e.g: modify_file_ext("/tmp/prog.as", ".ob") -> "/tmp/prog.ob"
 * Prerequisite: path must end with a file extension.
 */
char*  /* the file extension */
modify_file_ext(const char *path, const char *ext)
{
  char *newpath = malloc(strlen(path) + strlen(ext));
  const char *dot = get_file_ext(filename);
  memcpy(newpath, path, dot - path);
  memcpy(&newpath[dot - path], ext, strlen(ext) + 1);
  return newpath;

}

/*
 * Returns the nth byte in the combined image of instructions and memory.
 */
char  /* the byte */
get_img_byte(int n) {
  if(n < ICF) {
    return inst_img[n];
  }
  return mem_img[n-ICF];
}

/*
 * Writes the .ob file according to the langauage specifications.
 */
void
write_ob_file()
{
  char *obfilename = modify_file_ext(filepath, ".ob");
  FILE *obfile = fopen(obfilename, "w");
  int i;
  free(obfilename);  /* was only required to open file */

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
  if(obfile) fclose(obfile);
}


/*
 * If applicable, Writes the .ext file according to the langauage specifications.
 */
void
write_ext_file()
{
  char *extfilename = modify_file_ext(filepath, ".ext");
  FILE *extfile = NULL;
  SymbolEntry_t symbol;
  int i;
  for(i=0; i<symtable_size; i++) {
    symbol = symtable[i];
    if((symbol.attr & SYM_REQUIRED) && (symbol.attr & SYM_EXTERN)) {
      if(extfile == NULL) /* only create file if relevant */ {
        extfile = fopen(extfilename, "w");
        free(extfilename);  /* was only required to open file */
      }
      fprintf(extfile, "%s %04ld\n", symbol.name,
          symbol.offset + INITIAL_IC + (symbol.attr & SYM_DATA ? ICF : 0)
      );
    }
  }
  if(extfile) fclose(extfile);
}

/*
 * If applicable, Writes the .ent file according to the langauage specifications.
 */
void
write_ent_file()
{
  char *entfilename = modify_file_ext(filepath, ".ent");
  FILE *entfile = NULL;
  SymbolEntry_t symbol;
  int i;
  for(i=0; i<symtable_size; i++) {
    symbol = symtable[i];
    if((symbol.attr & SYM_ENTRY) && !(symbol.attr & SYM_REQUIRED)) {
      if(entfile == NULL) /* only create file if relevant */ {
        entfile = fopen(entfilename, "w");
        free(entfilename);  /* was only required to open file */
      }
      fprintf(entfile, "%s %04ld\n", symbol.name,
          symbol.offset + INITIAL_IC + (symbol.attr & SYM_DATA ? ICF : 0)
      );
    }
  }
  if(entfile) fclose(entfile);
}

/* 
 * Assembles source file.
 * If the source code is valid:
 *    Writes .ob and .ext, .ent files if relevant.
 * Else:
 *  Prints all syntax errors in file, writes none files and returns 1.
 */
int  /* nonzero on failure */
assemble(FILE *source)
{
  /* init */
  Statement_t *statements;
  error_occurred = 0;
  statements = parse_file(source);
  IC = 0; DC = 0;
  memset(inst_img, 0, sizeof(inst_img));
  memset(mem_img, 0, sizeof(mem_img));
  init_symtable();

  write_memory_image(statements);
  ICF = IC; DCF = DC;
  IC = 0; DC = 0;
  write_instruction_image(statements);
  free(statements);
  /* can be set by any of the above calls */
  if(error_occurred) return 1;


  write_ob_file();
  write_ext_file();
  write_ent_file();
  return 0;
}

/*
 * Main.
 * Exit code is 0 if all files successfuly were successfuly assembled.
 * If atleast 1 file failed to assemble, the exit code is 1.
 */
int  /* nonzero on failure */
main(int argc, char** argv)
{

  int exit_status;
  size_t i;
  FILE *file;

  /*int n = 1;
  printf("integer: %ld\n", -1 * (1L << ((n)-1)));
  printf("integer: %d\n", (~0 << ((n)-1)));

  printf("integer: %ld\n", (1L << ((n)-1)));
  printf("integer: %d\n", ~(~0 << ((n)-1)));*/

  if (argc < 2)  /* no cmdline arguments - print error and exit */
    error(EXIT_FAILURE, 0, NOARGS_ERR"\n"HELP_TEXT, argv[0]);

  exit_status = 0;  /* =0 iff all files successfuly assembled, else 1 */
  for (i=1; i<argc; i++) {

    filepath = argv[i];
    filename = basename(filepath);
    if (NULL == (file = fopen(filepath, "r")))
      /* file won't open - print error message and skip it */
      error(0, errno, "%s", argv[i]);
    else {
      if(0 != strcmp(".as", get_file_ext(filename))) {
        fclose(file);
        printf(EXT_ERR"\n", argv[0], filepath);
        continue;
      }
      if(assemble(file) != 0)
        exit_status = 1;
    }
    if(file) fclose(file);
    cleanup_symtable();
  }

  return exit_status;
}
