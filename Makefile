#compiler
CC = gcc
#compiler flags
CFLAGS = -Wall -g
#source files
SRC = main.c 
#output binary name
OUT = diskusage
#build rule
all:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

#clean rule (optional)
clean:
	rm -f $(OUT)