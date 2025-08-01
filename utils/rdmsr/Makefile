CC:= gcc
CFLAGS:= -O3 -D_GNU_SOURCE -I. -m64 -falign-functions=64 -municode -static -static-libgcc -Wall
LIBS:= -L. -lwinring0
UTILS:=

all: rdmsr.o

rdmsr: rdmsr.o
	$(CC) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	cmd /C del /Q bin obj

.PHONY: all clean
