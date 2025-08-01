CC:= gcc
CFLAGS:= -O0 -D_GNU_SOURCE -m64 -g -Wno-implicit-function-declaration -Wall
LIBS:= -lpthread 

all: obj bin out class0 class1 class3

class0: obj/class0.o ../utils/util.o
	$(CC) -o bin/$@ $^ $(LIBS)

class1: obj/class1.o ../utils/util.o
	$(CC) -o bin/$@ $^ $(LIBS)

class3: obj/class3.o ../utils/util.o
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

