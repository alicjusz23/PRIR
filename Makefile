CC=mpicc #gcc
CFLAGS=-lmpi
RUN=mpirun -np 3
fscount: main9.c
	${CC} main9.c -g -o wcount ${CFLAGS}
run: wcount
	${RUN} ./wcount $(arg1) $(arg2)

