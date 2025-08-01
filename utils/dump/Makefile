CC:= gcc
CFLAGS:= -O3 -D_GNU_SOURCE -m64 -falign-functions=64 -municode -static -static-libgcc -Wall
LIBS:= 
UTILS:=

all: dump.o inpoutx64.o

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

obj:
	mkdir $@

bin:
	mkdir $@

out:
	mkdir $@

clean:
	cmd /C del /Q bin obj

.PHONY: all clean
