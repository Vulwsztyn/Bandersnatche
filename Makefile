all: bank

bank: main.o init.o
	~/openmpi/bin/mpicc main.o init.o -o bank

init.o: init.c 
	~/openmpi/bin/mpicc init.c -c -Wall

main.o: main.c main.h
	~/openmpi/bin/mpicc main.c -c -Wall

clear: 
	rm *.o bank
