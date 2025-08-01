CC:= gcc
CFLAGS:= -O3 -D_GNU_SOURCE -m64 -static -falign-functions=64 -Wno-implicit-function-declaration -Wall

all: obj bin out stress monitor

stress: obj/stress.o
	$(CC) -o bin/$@ $^

obj/%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

monitor:
	$(MAKE) -C ../utils

obj:
	mkdir $@

bin:
	mkdir $@

out:
	mkdir $@

clean:
	cmd /C del /Q bin obj

.PHONY: all clean
