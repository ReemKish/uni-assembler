CFLAGS += -Wall -ansi -pedantic -g

assembler: assembler.c scan1.c scan1.h scan2.c scan2.h tokenizer.c tokenizer.h parser.c parser.h consts.h tables.c tables.h types.h errors.c errors.h
scan1: tokenizer.c tokenizer.h tables.c tables.h types.h consts.h parser.h errors.c errors.h
scan2: tokenizer.c tokenizer.h tables.c tables.h types.h consts.h parser.h errors.c errors.h
clean:
	rm assembler
