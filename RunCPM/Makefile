# Generic MinGW makefile (C source only)
# Modify variables/macros to fit your program

# make / make all | will compile your program.
# make clean      | deletes all compiled object and executable files.
# make depend     | rebuilds the dependancy list
# make run        | compiles (if needed) and runs your program

# Compiler command
CC = gcc
#CC = gcc -DDEBUG=1 -DDEBUGLOG=1

# Linker command
LD = gcc

# Flags to pass to the compiler - add "-g" to include debug information
CFLAGS = -Wall -O0
#CFLAGS = -Wall -O0 -g

# Flags to pass to the linker
LDFLAGS = -lncurses

# Command used to delete files
RM = rm -f

# List your object files here
OBJS = main.o z80.o

# List your source files here
SRCS = main.c z80.c

# Define your compile target here.
PROG = runcpm

# Link the program
$(PROG): $(OBJS)
	$(LD) $(OBJS) -o $(PROG) $(LDFLAGS)

# All .o files depend on their corresponding .c file
%.o: %.c
	$(CC) $(CFLAGS) -c $< 2>&1 | sed -e "s/:/(/" -e "s/:/,/" -e "s/:/):/"

# Compile everything.
all: clean $(PROG)

clean:
	$(RM) $(PROG)
	$(RM) *.o

run: all
	$(PROG)
