CC:= gcc
CFLAGS:= -O3 -D_GNU_SOURCE -m64 -static -falign-functions=64 -Wno-implicit-function-declaration -Wall
UTILS:= ../utils/util.o 

all: obj bin out core_out sample monitor opencl

sample: obj/sample.o $(UTILS)
	$(CC) -o bin/$@ $^

obj/%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

monitor:
	$(MAKE) -C ../utils

opencl:
	$(MAKE) -C ../utils/opencl

obj:
	mkdir $@

bin:
	mkdir $@

out:
	mkdir $@

core_out:
	mkdir $@

clean:
	cmd /C del /Q bin obj

.PHONY: all clean
