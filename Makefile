CFLAGS += -Wall -ansi -pedantic -g

assembler: 	 assembler.o parser.o tokenizer.o scan.o tables.o errors.o types.h consts.h

assembler.o: assembler.c parser.o 					  scan.o tables.o 					  		 consts.h
parser.o: 	 parser.c 	 parser.h tokenizer.o 			 tables.o errors.o types.h consts.h				
tokenizer.o: tokenizer.c          tokenizer.h 			 tables.o 				 types.h consts.h
scan.o: 		 scan.c	     											scan.h tables.o errors.o types.h consts.h
tables.o: 	 tables.c																 tables.h 				 types.h consts.h
errors.o: 	 errors.c		 parser.h 														errors.h

clean:
	rm assembler assembler.o parser.o tokenizer.o scan.o tables.o errors.o
