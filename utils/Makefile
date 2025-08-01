CC:= gcc
CFLAGS:= -O3 -D_GNU_SOURCE -m64 -static -falign-functions=64 -Wno-implicit-function-declaration -Wall
LIBS:= -L. -lwinring0
UTILS:= ./util.o ./rdmsr/rdmsr.o ./dump/inpoutx64.o ./dump/dump.o

all: obj monitor1 monitor2 monitor_pstate

monitor1: obj/monitor1.o $(UTILS)
	$(CC) -o $@ $^ $(LIBS)

monitor2: obj/monitor2.o $(UTILS)
	$(CC) -o $@ $^ $(LIBS)

monitor_pstate: obj/monitor_pstate.o $(UTILS)
	$(CC) -o $@ $^ $(LIBS)

obj/%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

dump:
	$(MAKE) -C ./dump

rdmsr:
	$(MAKE) -C ./rdmsr

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
