#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

//author: Alicja Puacz

int nwd(int a, int b){
	int c;
	while(b!=0){
		c = a%b;
		a=b;
		b=c;
	}
	return a;
}

void main(int argc, char** argv){

    // inicajlizacja MPI environment
    MPI_Init(NULL, NULL);
    int world_rank, world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	srand(time(NULL) + world_rank);
	int liczba = rand()%20;

	int i=0;
    	int doKtorego, zKtorego;

	if (floor(log(world_size)/log(2))!=(log(world_size)/log(2))){
		if (world_rank==0)
			printf("\nPodaj lb procesow bedaca potega lb 2!\n");
		MPI_Finalize();
		exit(0);
	}		
	while(pow(2,i)!= world_size){
		doKtorego = world_rank+pow(2,i);
		zKtorego = world_rank-pow(2,i);

		if(doKtorego>=world_size)
			doKtorego-=world_size;
		if(zKtorego<0)
			zKtorego+=world_size;
		MPI_Send(&liczba, 1, MPI_INT, doKtorego, 0, MPI_COMM_WORLD);
		printf("It. %d, wyslano z %d do procesu %d liczbe %d\n", i, world_rank, doKtorego, liczba);

		int liczba2;
		MPI_Recv(&liczba2, 1, MPI_INT, zKtorego, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		liczba = nwd(liczba,liczba2);
		i++;
	}
	if (world_rank==0)
        	printf("\nWynik: %d\n", liczba);
        MPI_Finalize();
}
