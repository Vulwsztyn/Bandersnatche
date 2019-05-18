all: bank

bank: main.o init.o
	~/openmpi/bin/mpicc main.o init.o -o bank

init.o: init.c 
	~/openmpi/bin/mpicc init.c -c -Wall

main.o: main.c main.h
	~/openmpi/bin/mpicc main.c -c -Wall

clear: 
	rm *.o bank
kill:
	ps aux|grep bank|tr -s ' '|cut -d " " -f2
	ps aux|grep bank|tr -s ' '|cut -d " " -f2|xargs kill -9
run6:
	~/openmpi/bin/mpirun -n 6 --oversubscribe ./bank 

run4:
	~/openmpi/bin/mpirun -n 4 --oversubscribe ./bank 

run2:
	~/openmpi/bin/mpirun -n 2 --oversubscribe ./bank 
	
run3:
	~/openmpi/bin/mpirun -n 3 --oversubscribe ./bank 