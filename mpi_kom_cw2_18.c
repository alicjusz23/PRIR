   #include "mpi.h"
   #include <stdio.h>
   #include <string.h>
   #include <stdlib.h>
   #define LEFT  0
   #define RIGHT 1
   #define CZESCI 1000	//po ile wysylamy
   #define PORCJE 1000 	//ile razy wysylamy
//to do: przy czesciach 10 x wiekszych jest segmentation fault

//sumuje elementy wektora - funkcja uzywana przy kontroli
int sum(int w[], int n){
	int i, suma=0;
	for (i=0;i<n;i++){
		suma+=w[i];
	}
	return suma;
}


//tworzy wektor o rozmiarze PORCJE*CZESCI
void tworzWektor(int w[]){
	int i, j;
	for (i=0; i<CZESCI; i++){
		for(j=1; j<=PORCJE; j++)
		//to do: różne wartości wstawić w wektor
    			w[i*PORCJE +j-1] = i;
	}
}

void wydzielPorcje(int w_d[], int w[], int porcja){
	int k;
	for(k=1; k<=CZESCI; k++){	
		w[k-1] = w_d[k*porcja];
	}
}


//aby zainicjowac array, w ktorym beda odbierane wiadomosci
// wartosciami MPI_PROC_NULL tyle razy, ile jest CZESCI
void zainicjujInbuf(int inbuf[][CZESCI]){
	int i, j;	
	for(i=0;i<2;i++){
		for(j=0;j<CZESCI;j++)
			inbuf[i][j]=MPI_PROC_NULL;
	}
}


//funkcja main
void  main(int argc, char *argv[])  {
   int numtasks, rank, source, dest, outbuf[CZESCI], i, j,
      tag=1, 
	//lepiej array zamiast wskaznikow 
      inbuf[2][CZESCI], 
      nbrs[2],  
      periods[1]={0}, 
      reorder=0, 
      coords[1],
	wektor_duzy[PORCJE*CZESCI],
	wektor[CZESCI],
	suma_k=0;

	zainicjujInbuf(inbuf);

   	MPI_Init(&argc,&argv);
   	MPI_Comm_size(MPI_COMM_WORLD, &numtasks);

   	int dims[1]={numtasks};

   	MPI_Request reqs[2];
   	MPI_Status stats[2];
   	MPI_Comm cartcomm;   // required variable


	if (numtasks > 0) {
      // topologia kartezjańska
      		MPI_Cart_create(MPI_COMM_WORLD, 1, dims, periods, reorder, &cartcomm);
      		MPI_Comm_rank(cartcomm, &rank);
      		MPI_Cart_coords(cartcomm, rank, 2, coords);
      		MPI_Cart_shift(cartcomm, 0, 1, &nbrs[LEFT], &nbrs[RIGHT]);


	if(rank==0){
		tworzWektor(wektor_duzy);
	}
      // ta petla - by wyslac dane tyle razy ile wynosi PORCJE
	for(j=0;j<PORCJE;j++){
		//przy rozpoczeciu kazdej nowej porcji 
		//wydziel porcje do wyslania i zapisz w wektor
		if (rank==0)
			wydzielPorcje(wektor_duzy, wektor, j);
		//ta petla - bo wysylamy w 2 kierunkach
		for (i=0; i<2; i++) {
			if (rank==0){
				memcpy(outbuf, wektor, CZESCI * sizeof(int));
			}else{
				//printf("rank %d inbuf %d %d \n", rank, wektor[0], wektor[1]);
				memcpy(outbuf, inbuf[LEFT], CZESCI * sizeof(int));
			}
         		dest = nbrs[i];
         		source = nbrs[i];
		//komunikacja blokujaca i asynchroniczna - dlaczego?
			if (i==1) //w prawo - wysylamy
         			MPI_Send(&outbuf, CZESCI, MPI_INT, dest, tag, MPI_COMM_WORLD);
			if (i==0) //w lewo - odbieramy
         			MPI_Recv(&inbuf[i], CZESCI, MPI_INT, source, tag, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
         	}
      		//MPI_Waitall(2, reqs, stats);
   
      		//printf("\trank= %d                  inbuf(l1, l2, r1, r2)= %d %d %d %d \n", rank, inbuf[LEFT][0], inbuf[LEFT][1], inbuf[RIGHT][0], inbuf[RIGHT][1]); 
		if(rank==(numtasks-1)) 
			suma_k+=sum(inbuf[LEFT], CZESCI);
		}
 
	}else
      		printf("Utwórz przynajmniej 1 proces.\n");
   	if(rank==(numtasks-1)) 
		printf("Kontrola: \tsuma w rank %d = %d\n", numtasks-1, suma_k);
	if(rank==0){
		printf("Kontrola: \tsuma wektora duzego = %d\n", sum(wektor_duzy, (PORCJE*CZESCI)));
	}
   	MPI_Finalize();
}
