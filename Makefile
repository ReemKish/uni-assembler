$(shell mkdir -p bin)
IDIR = src
ODIR = bin
CC=gcc
CFLAGS += -I$(IDIR) -Wall -ansi -pedantic -g

_DEPS = types.h consts.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = assembler.o parser.o tokenizer.o scan.o tables.o errors.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: $(IDIR)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

assembler: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm assembler -f $(ODIR)/*.o
