CC=g++
all: sim

sim: Simulation.o cache.o memory.o Read_Elf.o
	$(CC)  -o $@ $^ 

Simulation.o: cache.h

cache.o: cache.h def.h

memory.o: memory.h

Read_Elf.o:Read_Elf.h

.PHONY: clean

clean:
	rm -rf sim *.o
