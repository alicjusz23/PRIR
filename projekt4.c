   #include <stdio.h>
   #include <string.h>
   #include <stdlib.h>

void print_matrix(int ma, int na, double A[ma][na]){
    int i, j;
    printf("[");
    for(i =0; i< ma; i++){
        for(j=0; j<na; j++)
        {
            printf("%lf ", A[i][j]);
        }
        printf("\n");
    }
    printf("]\n");
}

void print_matrix2(int ma, double A[]){
    int i;
    printf("[");
    for(i =0; i< ma; i++){
            printf("%lf ", A[i]);
    }
    printf("]\n");
}


void liczJacobi(int ma, int na, double A[ma][na], double b[], double C[]){
	int i,j, k, iter=2, num = 4;
	double **M;
	double *N = malloc(ma*sizeof(double));
	double *x2 = malloc(ma*sizeof(double));
	
	M = malloc(ma*sizeof(double));
	for(i=0; i< ma; i++){
		M[i] = malloc(na*sizeof(double));
	}
	
	// Calculate N = D^-1
   for (i=0; i<num; i++){
      N[i] = 1/A[i][i];
   }
   
   // Calculate M = -D^-1 (L + U)
   for (i=0; i<num; i++)
      for (j=0; j<num; j++)
         if (i == j)
            M[i][j] = 0;
         else
            M[i][j] = - (A[i][j] * N[i]);

   //Initialize x
   for (i=0; i<num; i++)
      C[i] = 0;

   for (k=0; k<iter; k++) {
      for (i=0; i<num; i++) {
         x2[i] = N[i]*b[i];
         for (j=0; j<num; j++)
            x2[i] += M[i][j]*C[j];
      }
      for (i=0; i<num; i++)
         C[i] = x2[i];
   }
}


void liczGaussSeid(int ma, int na, double A[ma][na], double b[], double C[]){
	int i, j, k, iter=3;
	double **U;
	double **D;
	double **L;
	double *x = malloc(ma*sizeof(double));
	
	//przydzial pamieci
	U = malloc(ma*sizeof(double));
	D = malloc(ma*sizeof(double));
	L = malloc(ma*sizeof(double));
	for(i=0; i< ma; i++){
		U[i] = malloc(na*sizeof(double));
	}
	for(i=0; i< ma; i++){
		D[i] = malloc(na*sizeof(double));
	}
	for(i=0; i< ma; i++){
		L[i] = malloc(na*sizeof(double));
	}
	
   // Divide A into L + D + U
   for (i=0; i<ma; i++)
      for (j=0; j<na; j++) {
         if (i < j) {
            U[i][j] = A[i][j];
         }
         else if (i > j) {
            L[i][j] = A[i][j];
         }
         else {
            D[i][j] = A[i][j];
         }
      }

   // Calculate D^-1
   for (i=0; i<ma; i++)
      D[i][i] = 1/D[i][i];

   // Calculate D^-1 * b
   for (i=0; i<ma; i++)
      b[i] *= D[i][i];

   //Calculate D^-1 * L
   for (i=0; i<ma; i++)
      for (j=0; j<i; j++)
         L[i][j] *= D[i][i];

   //Calculate D^-1 * U
   for (i=0; i<ma; i++)
      for (j=i+1; j<ma; j++)
         U[i][j] *= D[i][i];

   //Initialize x
   //for (i=0; i<ma; i++)
     // x[i] = 0;

   for (k=0; k<iter; k++)
      for (i=0; i<ma; i++) {
         C[i] = b[i];                       // x = D^-1*b -
         for (j=0; j<i; j++)
            C[i] -= L[i][j]*C[j];    // D^-1*L * x -
         for (j=i+1; j<ma; j++)
            C[i] -= U[i][j]*C[j];    // D^-1*U * x
      }
}


//funkcja main
void  main(int argc, char *argv[])  {
	int ma, mb, na, nb;
    double x;
	FILE *fpa;
	FILE *fpb;
	int i, j;

	fpa = fopen("A", "r");
	fpb = fopen("b", "r");
	if( fpa == NULL || fpb == NULL ){
		perror("błąd otwarcia pliku");
		exit(-10);
	}
	fscanf (fpa, "%d", &ma);
	fscanf (fpa, "%d", &na);
	fscanf (fpb, "%d", &mb);
	fscanf (fpb, "%d", &nb);
				
   	double A[ma][na];
	double b[mb];
	double C[ma];
	double D[ma];

	//inny warunek
		/*if(na != mb){
			printf("Złe wymiary macierzy!\n");
			return EXIT_FAILURE;
		}*/

	for(i =0; i< ma; i++) {
		for(j = 0; j<na; j++){
			fscanf( fpa, "%lf", &x );
			A[i][j] = x;
			if ((i == j) && (A[i][j] == 0)){
				printf("Wartosci na przekatnej musza byc rozne od 0\n");
				return;
			}
		}
	}
		
	for(i =0; i< mb; i++){
		fscanf( fpb, "%lf", &x );
		b[i] = x;
	}
		
	//sprawdz czy da sie rozwiazac układ

	printf("A:\n");
	print_matrix(ma, na, A);
	printf("b:\n");
	print_matrix2(mb, b);
	liczJacobi(ma, na, A, b, C);
	print_matrix2(ma, C);
	printf("\n");
	liczGaussSeid(ma, na, A, b, D);
	print_matrix2(ma, D);
}
