CC:= gcc
CFLAGS:= -O0 -D_POSIX_SOURCE -D_GNU_SOURCE -m64 -g
LIBS:= -lpthread 

all: obj bin out sender13 receiver0

receiver0: obj/receiver0.o ../utils/util.o 
	$(CC) -o bin/$@ $^ $(LIBS)

sender13: obj/sender13.o ../utils/util.o
	$(CC) -o bin/$@ $^ $(LIBS)

obj/%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

obj:
	mkdir obj

bin:
	mkdir bin

out:
	mkdir out

clean:
	cmd /C del /Q bin obj

.PHONY: all clean

