CC:= gcc
CFLAGS:= -O3 -D_GNU_SOURCE -m64 -falign-functions=64 -Wall
LIBS:= -lOpenCL
UTILS:=

all: run_opencl

run_opencl: run_opencl.o
	$(CC) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	del run_opencl.o run_opencl.exe

.PHONY: all clean
