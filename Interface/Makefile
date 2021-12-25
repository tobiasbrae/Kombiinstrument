CC=gcc
PROGNAME=kombiInterface
BINFILE=$(basename $(PROGNAME)).exe

OBJ_ALL=main.c
OBJ_LIN=serialCommunication_linux.c
OBJ_WIN=serialCommunication_windows.c

CFLAGS=-std=c99 -Wall

all:

windows: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ_ALL) $(OBJ_WIN) -o $(BINFILE)
	
linux: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ_ALL) $(OBJ_LIN) -o $(PROGNAME)
