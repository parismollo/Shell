# Makefile
# > make [option]
# options are: clean, slash, run.
# To clean, compile and execute do:
# > make 

CC=gcc
CFLAGS=-Wall -lreadline
DEPS=slash.h
OBJ = slash.o

# Remove comment above if you wish to remove prints in terminal.
# $(VERBOSE).SILENT:

all: clean slash

# create executable file using obj file.
slash: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

# create object file for every .c file.
%.o: %.c $(DEPS)
	$(CC) -c -o $@  $< $(CFLAGS)

# run slash 
run:
	./slash

# clean files
clean:
	rm -f *.o
	rm -f slash
