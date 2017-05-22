#include <stdio.h>
#include <mpi.h>
#include <stdlib.h>
#include <math.h>

//author: Alicja Puacz


    //Napisanie procedury całkowania
double integrate(double (*func)(double), double begin, double end, int num_points){
    double dx = (end-begin)/num_points;
    double wynik = ((*func)(begin) + (*func)(end))/2;
    double i=begin+dx;
    while (i<end){
        wynik += (*func)(i);
	i=i+dx;
    };
    //printf("Wyszlo %lf  %lf  %d\n", begin, end, num_points);
    return wynik*dx;
};

int sumuj_wek(int wek[], int n){
	int w=0;
	int i;
	for (i=0;i<n;n++){
		w+= wek[i];
	}
	return w;
}

void main(int argc, char** argv){
    /*if (argc<4){
        //return 1;
    }*/
    double begin = atof(argv[1]);
    double end = atof(argv[2]);
    int num_points = atoi(argv[3]);

    // inicajlizacja MPI environment
    MPI_Init(NULL, NULL);
    int world_rank, world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    int number;
    int i;
    int points;
    int args[3];    //argumenty dla funkcji w całce

//Podział liczby punktów na poszczególne procesy (możliwie równy!)
//po ile pkt każdy proces dostanie
    	int przydzial[world_size];
	int przydzial_ile = num_points/world_size;
    	int przydzial_reszta = num_points % world_size;
	for (i=0; i<world_size; i++){
	    if(i < przydzial_reszta){
		przydzial[i] = przydzial_ile + 1;
    	    }else 
		przydzial[i] = przydzial_ile;
	} 

 //Podział przedziałów całkowania dla poszczególnych procesów
    double zakres_k = (end-begin)/(num_points);
    double zakres_ar[world_size+1];
    zakres_ar[0] = begin;
    zakres_ar[world_size+1]=end;
    for(i=1; i<world_size+1;i++){
	zakres_ar[i] = begin + zakres_k*sumuj_wek(przydzial, i);
	//printf("cos %lf\n", zakres_ar[i]);
    }
    for(i=0; i<=world_size+1;i++){
	printf("cos %lf\n", zakres_ar[i]);
    }

    if (world_rank != 0) {
        MPI_Recv(&args, 3, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	double (*fp)(double);
	fp= sin;
    	double w = integrate(fp, args[0], args[1], args[2]);
        printf("Process %d received %lf\n",world_rank, w);
    }

    if (world_rank == 0){
        for(i=1;i<world_size;i++){
            args[0]=zakres_ar[i];	//begin
            args[1]=zakres_ar[i+1];	//end
            args[2]=(double) przydzial[i];	//num points
	    printf("Tu rank %d %lf  %lf  %d\n", i, zakres_ar[i], zakres_ar[i+1], przydzial[i]);
	//Rozesłanie do procesów „robotników” danych umożliwiających obliczenia cząstkowe.
            MPI_Send(&args, 3, MPI_DOUBLE, i, 0, MPI_COMM_WORLD);
            printf("Wysylam sygn do %d\n", i);
        };
    };


    //Odbiór danych przez procesy.

    //Wykonanie obliczeń w procesie głównym.

    //Wykonanie obliczeń w procesach „robotnikach”

    //Przesłanie obliczeń cząstkowych do procesu głównego.

    // Finalize the MPI environment.
        MPI_Finalize();
}
