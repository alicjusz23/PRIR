	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <ctype.h>
	#include <stdbool.h>

	#include <mpi.h>

	//#define debug printf // odkomentuj, aby zobaczyc komunikaty diagnostyczne
	#define debug // odkomentuj, aby ukryc komunikaty diagnostyczne
	#define die(msg, err) do { perror(msg); return err; } while(0)
	#define master 0

	int world_rank; /* identyfikator w komunikatorze globalnym */
	int world_size; /* liczba procesow */

	/* Liczy roznice w bajtach pomiedzy wartosciami w indexes */
	void bytes(int size, int* indexes, int* count, int* skip, int* out_count_bytes, int* out_skip_bytes) {
	  int i;
	  /*            0123456789  */
	  /* text    = "oto tekst!" */
	  /* indexes = [3, 9]       */
	  /* indexes zawiera indeksy koncow podciagow w text */
	  for (i=0; i<world_size; i++) {
		if (i>0) out_skip_bytes[i] = indexes[skip[i]-1];
		out_count_bytes[i] = indexes[skip[i]+count[i]-1];
	  }

	  for (i= --size; i>0; i--)
		out_count_bytes[i] -= out_count_bytes[i-1];
	}

	/* generowanie tablic count i skip dla MPI_Scatter/Gather na podstawie ilosci */
	/* slow nwords i liczby procesow size */
	void divide(int size, int nwords, char* words, int* out_count, int* out_skip) {
	  int i, j;
	  int part = nwords / size; /* calkowita czesc podzialu */
	  int reminder = nwords % size; /* reszta podzialu */

	  debug("part %d, reminder %d\n", part, reminder);
	  for (i=0; i<size; i++) {
		out_count[i] = part;

		if (reminder-- > 0) /* jesli zostala reszta to dodaj ja do tego procesu */
		  out_count[i]++;

		out_skip[i] = 0;
		/* dla count = [0, 2, 4, 5] => skip = [0, 0+2, 0+2+4, 0+2+4+5] */
		for (j=0; j<i; j++)
		  out_skip[i] += out_count[j];
	  }
	}

	/* redukuje {kN: [v1, v2, v3 ...]} do {kN: sum([v1, v2, v3 ...])} */
	void reduce(int nwords, char* words, int* indexes, int* list) {
	  int nbytes;
	  int i, j;
	  int* count = NULL;
	  int* skip = NULL;
	  int* count_bytes = NULL;
	  int* skip_bytes = NULL;
	  
	  if (world_rank == master) {
		count = calloc(world_size, sizeof(int));
		skip = calloc(world_size, sizeof(int));
		count_bytes = calloc(world_size, sizeof(int));
		skip_bytes = calloc(world_size, sizeof(int));

		divide(world_size, nwords, words, count, skip);
		bytes(world_size, indexes, count, skip, count_bytes, skip_bytes);

		for (i=nwords-1, j=world_size-1; i>=0; i--) {
		  if (i<skip[j])
			 j--;
		  indexes[i] -= indexes[skip[j]-1];
		}
		for (i=0; i<world_size; i++)
		  debug("R count[%d] = %d, count_bytes[%d] = %d, skip[%d] = %d, skip_bytes[%d] = %d\n", i, count[i], i, count_bytes[i], i, skip[i], i, skip_bytes[i]);
		for (i=0; i<nwords; i++)
		  debug("R indexes[%d] = %d\n", i, indexes[i]);
	  }

	  /* rozsylanie ilosci slow przydzielonych procesom */
	  /* zapisane w tablicy count, w procesie zapisywane do zmiennej nwords */
	  MPI_Scatter(count, 1, MPI_INT, &nwords, 1, MPI_INT, master, MPI_COMM_WORLD);
	  debug("[%d] Got %d words to reduce\n", world_rank, nwords);
	  if (world_rank != master) /* jesli proces nie jest master, to trzeba mu przygotowac indexes */
		indexes = calloc(nwords, sizeof(int));

	  MPI_Scatter(count_bytes, 1, MPI_INT, &nbytes, 1, MPI_INT, master, MPI_COMM_WORLD);
	  if (world_rank != master)
		list = calloc(nbytes, sizeof(int));
	  debug("[%d] Got %d bytes of list to reduce\n", world_rank, nbytes);

	  MPI_Scatterv(list, count_bytes, skip_bytes, MPI_INT, list, nbytes, MPI_INT, master, MPI_COMM_WORLD);
	  MPI_Scatterv(indexes, count, skip, MPI_INT, indexes, nwords, MPI_INT, master, MPI_COMM_WORLD);
	  for (i=0; i<nwords; i++)
		debug("[%d] Index %d\n", world_rank, indexes[i]);

	  for (i=0, j=0; i<nwords; i++) {
		int sum = 0;
		for (; j<indexes[i]; j++)
		  sum += list[j];
		indexes[i] = sum;
		debug("[%d] SUM %d = %d\n", world_rank, i, indexes[i]);
	  }
	  MPI_Gatherv(indexes, nwords, MPI_INT, indexes, count, skip, MPI_INT, master, MPI_COMM_WORLD);
	}

	/* redukuje mapuje slowa [k1, k2, k1, k3...] z words na [{k1: v1}, {k2: v2}, {k1: v3}, {k3: v4} ...] */
	void map(int nwords, char* words, int* indexes, int* out_nwords, char* out_words, int* out_indexes, int* out_occurs) {
	  int  all_words = 0;
	  int* count = NULL;
	  int* count_bytes = NULL;
	  int* skip = NULL;
	  int* skip_bytes = NULL;
	  char* w = NULL;
	  int i, j, len;
	  int nbytes;

	  indexes++;
	  if (world_rank == master) {
		all_words = nwords;

		count = calloc(world_size, sizeof(int));
		count_bytes = calloc(world_size, sizeof(int));
		skip = calloc(world_size, sizeof(int));
		skip_bytes = calloc(world_size, sizeof(int));

		divide(world_size, nwords, words, count, skip);
		bytes(world_size, indexes, count, skip, count_bytes, skip_bytes);

		for (i=0; i<world_size; i++)
		  debug("M count[%d] = %d, count_bytes[%d] = %d, skip[%d] = %d, skip_bytes[%d] = %d\n", i, count[i], i, count_bytes[i], i, skip[i], i, skip_bytes[i]);
		for (i=0; i<nwords; i++)
		  debug("M indexes[%d] = %d\n", i, indexes[i]);
	  }

	  /* nwords */
	  MPI_Scatter(count, 1, MPI_INT, &nwords, 1, MPI_INT, master, MPI_COMM_WORLD);
	  debug("[%d] Got %d words to map\n", world_rank, nwords);

	  /* indexes */
	  MPI_Scatter(count_bytes, 1, MPI_INT, &nbytes, 1, MPI_INT, master, MPI_COMM_WORLD);
	  if (world_rank != master)
		words = calloc(nbytes, sizeof(char));

	  /* words */ 
	  MPI_Scatterv(words, count_bytes, skip_bytes, MPI_CHAR, words, nbytes, MPI_CHAR, master, MPI_COMM_WORLD);

	  {
		int i, j; int len = 0;
		int* occurs = NULL;

		/* words */
		MPI_Gatherv(words, nbytes, MPI_CHAR, words, count_bytes, skip_bytes, MPI_CHAR, master, MPI_COMM_WORLD);
		occurs = calloc((world_rank == master)? all_words : nwords, sizeof(int));
		for (i=0; i<nwords; i++)
		  occurs[i] = 1;

		/* occurs */
		MPI_Gatherv(occurs, nwords, MPI_INT, occurs, count, skip, MPI_INT, master, MPI_COMM_WORLD);

		if (world_rank == master) {
		  int p, q;
		  char* w = words;
		  char* nw = out_words;

		  indexes--;
		  for (i=0, p=0, q=0; i<all_words; i++, p++) {
			char* a = words+indexes[i];
			if (*a == '\0') {
			  p--;
			  continue;
			}
			out_indexes[p] = p > 0? out_indexes[p-1] : 0;

			for (j=i; j<all_words; j++) {
			  char* b = words+indexes[j];
			  if (strcmp(a, b) == 0) {
				out_indexes[p]++;
				out_occurs[q++] = occurs[j];
				if (j==i) {
				  strcpy(nw, a);
				  nw += strlen(a) + 1;
				} else {
				  *b = '\0';
				}
			  }
			}
		  }
		  *out_nwords = p;
		}
	  }
	}

//zwraca true jesli napis jest IP
bool porownajIP(char * z){
		int i=0;
		int l = strlen(z);
		char * znak = z;
		while (l>0){
			if(znak[l]== '.')
				i++;
			l--;
		}
		//strstr - czy zawiera znak ':'
		if (i==3 && (strstr(z, ":")==NULL) && (strstr(z, "-")==NULL) && (strstr(z, "/")==NULL)){
			return true;
		}
		return false;
	}
	
	
//funkcja substring
void substring(char * z, int pocz, int koniec){
	int i, j;
	int dl = (koniec-pocz);
	char *kopia = malloc(dl*sizeof(char));
	for(i=pocz, j=0; i<=koniec; i++, j++){
		kopia[j] = z[i];
	}
	memcpy(z, kopia, dl );
	z[dl] = '\0';
}


//sprawdza czy string jest liczbą
bool isStrANumber(char * z){
	int i;
	for(i=0; i<strlen(z); i++){
		if(!isdigit(z[i]))
			return false;
	}
	return true;
}

	
//zwraca true jesli napis jest data i skraca do minut
bool porownajDate(char * z){
	char * kopia_z = z;
	if (z[0]!='['){
		return false;
	}
	//te są ok
	substring(z, 1, 18);
	return true;
}


//zwraca true jesli napis jest statusem
bool porownajStatus(char * z){
	int zi=NULL;
	if(isStrANumber(z)){
		zi = atoi(z);
		if ((zi>99 && zi<112) || (zi>199 && zi<207) || 
			(zi>299 && zi<311) || (zi>399 && zi<419) || 
			(zi>499 && zi<512)){
				return true;
		}else	
			return false;
	}else
		return false;
}


int main(int argc, char** argv) {
	  int i, j;
	  int nwords = 0;
	  int out_nwords = 0;
	  char* words = NULL;
	  char* w2 = NULL;
	  char* out_words = NULL;
	  int* indexes = NULL;
	  int* out_indexes = NULL;
	  int* out_occurs = NULL;
	  int buffer_size = 1024*64;
	  MPI_Init(NULL, NULL);
	  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	  MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	  
	  if (world_rank == master) {
		if (argc == 1 ){
			printf("Podaj argumenty\nUruchom program:\n make run arg1=\"stat/addr/time\"  arg2=\"<<nazwa pliku>>\" ");
			exit(1);
		}else if (argc != 3 ){
			printf("Podaj argumenty\nUruchom program:\n make run arg1=\"stat/addr/time\"  arg2=\"<<nazwa pliku>>\" ");
			exit(1);
		}
		char * k = argv[1];
		char * p = argv[2];
		char komenda[4];
		char plik[strlen(p)];
		memcpy(komenda, k, 4);
		memcpy(plik, p, strlen(p));
		
		int bytes, bytes2=0;
		char c;
		int len; char* w;
		FILE* f = fopen(plik,"r");
		words = calloc(buffer_size, sizeof(char)); /* "The last time I was"... */
		w2 = calloc(buffer_size, sizeof(char)); /* "The time I "... */
		indexes = calloc(buffer_size, sizeof(int)); /* "3 8 13 15... */

		if (!f) die("Nie udalo sie otworzyc pliku", 1);

		/* pozbieraj slowa */
		for (bytes = 0; (c = getc(f)) != EOF; bytes++) {
		  if (!isgraph(c)) {
			  if (!words[bytes-1]) bytes--;
			  continue;
		  }
		  if (bytes > buffer_size) {
			buffer_size *= 2;
			printf("Powiekszam bufor do %d bajtow.\n", buffer_size);
			words = realloc(words, buffer_size * sizeof(char));
			indexes = realloc(indexes, buffer_size * sizeof(int));
		  }  
		  words[bytes] = c;
		}
		
		/* policz slowa */
		char * w_poprz = NULL;
		int pocz = w2;
		w = words;
		len = strlen(w);
		while(len) {
		  w_poprz = w;
		  len = strlen(w);
		  if (len == 0) break;
		  if (strcmp(komenda, "addr")== 0){
					if(!porownajIP(w)){
						w += len + 1;
						continue;
					};
		  } else if (strcmp(komenda, "time")== 0){
					if(!porownajDate(w)){
						w += len + 1;
						continue;
					};
		  } else if (strcmp(komenda, "stat")== 0){
					if(!porownajStatus(w)){
						w += len + 1;
						continue;
					};
		  }	//else exit(1);
			
		  indexes[++nwords] = bytes2 + len+1*nwords;
		  debug("%s on position %d\n", w, indexes[nwords]);
		  strncpy(w2, w, len);
		  w += len + 1;
		  w2 += len + 1;
		  bytes2 += len;
		}
		w2 = pocz;
		
		debug("Wczytano %d bajtow (%d slow)\n", bytes2, nwords);
		out_words = calloc(bytes,sizeof(char));
		out_indexes = calloc(nwords, sizeof(int));
		out_occurs = calloc(nwords, sizeof(int));
	  }
	  
	  if (world_rank == master) printf("Faza mapowania %d slow na %d procesorach\n", nwords, world_size);
	  map(nwords, w2, indexes,
		  &out_nwords, out_words, out_indexes, out_occurs);

	  /* wyswietl wynik mapowania */
	  if (world_rank == master) {
		char* w = out_words;
		for (i=0; i<out_nwords; i++, w += strlen(w) + 1) {
		  int a = i>0? out_indexes[i-1] : 0;
		  int b = out_indexes[i];
		  debug("Mapped %s %d %d => [", w, a, b);
		  for (j=a; j<b; j++)
			debug("%d ", out_occurs[j]);
		  debug("]\n");
		}
	  }

	  if (world_rank == master) printf("Faza redukcji %d kluczy na %d procesorach\n", out_nwords, world_size);

	  reduce(out_nwords, out_words, out_indexes, out_occurs);

	  if (world_rank == master) {
		char* w = out_words;
		for (i=0; i<out_nwords; i++, w += strlen(w) + 1) {
		  printf("Reduced %s => %d\n", w, out_indexes[i]);
		}
	  }

	  if (world_rank == master) {
		free(words);
		free(indexes);
		free(out_words);
		free(out_indexes);
		free(out_occurs);
	  }

	  MPI_Finalize();
	  return 0;
	}

