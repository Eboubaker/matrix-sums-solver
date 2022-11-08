CC=g++
INCDIRS=-I.
OPT=-O0
CFLAGS=-Wall -Wextra -g $(INCDIRS) $(OPT)

CFILES=mtrx_funcs.cpp utils.cpp
OBJECTS=mtrx_funcs.o utils.o

S_CFILES=solver.cpp $(CFILES)
S_OBJECTS=solver.o $(OBJECTS)

S_BINARY=solver

G_CFILES=generator.cpp $(CFILES)
G_OBJECTS=generator.o $(OBJECTS)

G_BINARY=generator

all: $(S_BINARY) $(G_BINARY)


$(S_BINARY): $(S_OBJECTS)
	$(CC) -o  $@ $^ -lcrypto

$(G_BINARY): $(G_OBJECTS)
	$(CC) -o  $@ $^ -lcrypto

# regular expression where % is a wildcard
%.o:%.c
	$(CC) $(CFLAGS) -c -o $@ $^ -lcrypto

clean:
	rm -rf $(S_BINARY) $(G_BINARY) *.o