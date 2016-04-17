/*
PROBLEMA:
	O objetivo do trabalho é implementar, usando a biblioteca MPI, uma versão paralela, usando o modelo divisão e conquista, do algoritmo de ordenação Merge Sort.
	O programa deverá receber como parâmetros de entrada um número N, representando o número de valores a ser testado e o nome de um arquivo, que contém a lista de valores inteiros a serem ordenados. Utilizar 3 casos de teste para realização das medições no cluster.
	A saída que deve ser gerada é a lista ordenada (crescente) dos valores de entrada (1 valor por linha) e também o tempo de execução da aplicação.

SOLUÇÃO:
	??

Gabriel Chiele, Nicolas Nascimento - 14/04/2016

*/

#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <math.h>

#define ENABLE_DEBUG_PRINTS


/* Rotina que faz o merge de duas partes de um vetor.
 array[begin] até array[mid-1] e array[mid] até array[end-1].
 Coloca o resultado em b e depois coloca todos os elementos
 de b de volta para array (de begin até end).
 */
void merge(int array[], int begin, int mid, int end) {
    int ib = begin;
    int im = mid;
    int j;
    int size = end-begin;
    int b[size];
    
    /* Enquanto existirem elementos na lista da esquerda ou direita */
    for (j = 0; j < (size); j++)  {
        if (ib < mid && (im >= end || array[ib] <= array[im]))  {
            b[j] = array[ib];
            ib = ib + 1;
        }
        else  {
            b[j] = array[im];
            im = im + 1;
        }
    }
    
    for (j=0, ib=begin; ib<end; j++, ib++) array[ib] = b[j];
}

// Realiza o Merge Sort no vetor 'targetArray'
void mergeSort( int* array, int begin, int end ) {
	int mid;
	if( (begin == end) || (begin == end - 1) ) {
		return;	
	}
	mid = (begin + end)/2;
	mergeSort(array, begin, mid);
	mergeSort(array, mid, end);
	merge(array, begin, mid, end);
}

// Copia parte do vetor 'array', iniciando em 'arrayInitialPosition', e armazenando no vetor 'buffer'
void copyArrayToBuffer(int* array, int* buffer, int arrayInitialPosition, int arrayLength) {
	int i;
	int j = 0;
	for( i = arrayInitialPosition; i < (arrayInitialPosition + arrayLength); i++  ) {
		buffer[j] = array[i];
		j++;
	}
}

// Preenche o vetor 'array' com 'numberOfItems' valores a paritr de um arquivo
void getArrayFromFileWithName( int* array, char* fileName, int numberOfItems ) {
	FILE* fp = fopen(fileName, "r");
	if( !fp ) {
		printf("error while opening file = %s\n", fileName);
	}else{
		int currentItem = 0;
		while( currentItem < numberOfItems ) {
			fscanf(fp,"%d",&(array[currentItem]));
			currentItem++;
		}
	}
	fclose(fp);
}

// Escreve o tempo gasto e o vetor resultante para um arquivo
void writeDeltaTimeAndArrayToFileWithName(double delta, int* array, int arrayLength, char* fileName) {
    FILE* fp = fopen(fileName, "w");
    if( !fp ) {
        printf("Error while opening/creating file = %s\n", fileName);
    }else{
        int i = 0;
        for(i = 0; i < arrayLength; i++) {
            fprintf(fp, "%d\n", array[i]);
        }
        fprintf(fp, "difftime = %lf\n", delta);
    }
    fclose(fp);
}

// Concatena o vetor 'sourceArray' no vetor 'targetArray', 'targetArray' deve ser maior que 'sourceArray'
void concatenate(int* targetArray, int* sourceArray, int position, int length) {
	int i, j;
	for( i = position, j = 0; i < ( position + length); i++ ) {
		targetArray[i] = sourceArray[j];
		j++;
	}
}

// Função auxiliar que mostar os valores em um array
void printArray(int* array, int length) {
	int i = 0;
    printf("+++++++++++++++\n");
	for(i = 0; i < length; i++) {
		printf("%d ", array[i]);
	}
	printf("\n");
}

// Retorna o tempo atual em milisegundos
double getCurrentTimeMS(){
	struct timeval time;
	gettimeofday(&time, NULL);
	return  (time.tv_sec) * 1000 + (time.tv_usec) / 1000;
}


int main(int argc, char **argv) {

	// Confere a existência dos parâmetros necessários
	if( argc != 3 ) {
		printf("Incorrent number Of Arguments, expected 3\n");
		return -1;
	}

	char* fileName = argv[2];
	int arrayLength = atoi(argv[1]);
	int array[arrayLength];
	MPI_Status status;

	int rank,size;
	MPI_Init(&argc, &argv);
  	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  	MPI_Comm_size(MPI_COMM_WORLD, &size);

	double initialTime, finalTime;

	#ifdef ENABLE_DEBUG_PRINTS
		printf("Main\n");
	#endif
	
	if( size == 1 ) {
		#ifdef ENABLE_DEBUG_PRINTS
			printf("Sequential\n");
		#endif
		initialTime = getCurrentTimeMS();
		mergeSort(array, 0, arrayLength);
		finalTime = getCurrentTimeMS();
		#ifdef ENABLE_DEBUG_PRINTS
			printf("total time = %lf\n", finalTime - initialTime);
		#endif
	}/*else if( rank == 0 ) {
		
	}*/else{
        
        #ifdef ENABLE_DEBUG_PRINTS
            printf("%d initializing\n", rank);
        #endif
        
        int depth = 1;
        int pieceLength;// = arrayLength/(pow(2, depth));
        int childRank = -1;// = rank + pow(2, depth - 1);
        int parentRank = -1;// = rank - pow(2, depth - 1);
        
        if( rank == 0 ) {
            
            #ifdef ENABLE_DEBUG_PRINTS
                printf("%d reading file\n", rank);
            #endif
            
            getArrayFromFileWithName(array, fileName, arrayLength);
            depth = 1;
            pieceLength = arrayLength/(pow(2, depth));
            childRank = 1;
            parentRank = -1;
            #ifdef ENABLE_DEBUG_PRINTS
                printf("%d sending initial load of %d to child %d\n", rank, pieceLength, childRank);
            #endif
            
            //printArray(array, arrayLength);
            //printArray(array + pieceLength, pieceLength);
            
            MPI_Send((array + pieceLength), pieceLength, MPI_INT, childRank, 0, MPI_COMM_WORLD);
            MPI_Send(&depth, sizeof(depth),MPI_INT, childRank, 0, MPI_COMM_WORLD);
        }

		while(1) {
            
            if( rank != 0 && childRank == -1 && parentRank == -1 ) {
                parentRank = rank - pow(2, depth - 1);
                childRank = rank + pow(2, depth);
            }
            pieceLength = arrayLength/(pow(2, depth));

			int buffer[pieceLength];
			int receivedBuffer[pieceLength];
			int concatenatedBuffer[2*pieceLength];
			
            if( rank != 0 ) {
                #ifdef ENABLE_DEBUG_PRINTS
                    printf("%d waiting to receive load of %d from parent %d\n", rank, pieceLength, parentRank);
                #endif
            
                //while (1);
                MPI_Recv(buffer, pieceLength, MPI_INT, parentRank, 0, MPI_COMM_WORLD, &status);
                MPI_Recv(&depth, sizeof(depth),MPI_INT, parentRank, 0, MPI_COMM_WORLD, &status);
                
                #ifdef ENABLE_DEBUG_PRINTS
                    printf("done receiving\n");
                #endif
            }
		
			if( pieceLength > arrayLength/(pow(2, size - 1)) ) {
                
                #ifdef ENABLE_DEBUG_PRINTS
                    printf("%d sending to child %d\n", rank, childRank);
                #endif
                
				depth++;
				MPI_Send((buffer + pieceLength), pieceLength, MPI_INT, childRank, 0, MPI_COMM_WORLD);
				MPI_Send(&depth, sizeof(depth),MPI_INT, childRank, 0, MPI_COMM_WORLD);
			}else{
                #ifdef ENABLE_DEBUG_PRINTS
                    printf("%d sorting\n", rank);
                #endif
                
				mergeSort(buffer, 0, pieceLength);
			
				while( depth <= 0 ) {

					if( rank >= pow(2, depth - 1) ) {
                        
                        #ifdef ENABLE_DEBUG_PRINTS
                            printf("%d sending to parent %d\n", rank, parentRank);
                        #endif
                        
						MPI_Send(buffer, pieceLength, MPI_INT, parentRank, 0, MPI_COMM_WORLD);
						MPI_Send(&depth, sizeof(depth),MPI_INT, parentRank, 0, MPI_COMM_WORLD);
                        break;
					} else {
                        #ifdef ENABLE_DEBUG_PRINTS
                            printf("%d waiting for child %d\n", rank, childRank);
                        #endif
                        
						MPI_Recv(receivedBuffer, pieceLength, MPI_INT, childRank, 0, MPI_COMM_WORLD, &status);
						MPI_Recv(&depth, sizeof(int),MPI_INT, childRank, 0, MPI_COMM_WORLD, &status);
					
						copyArrayToBuffer(buffer, concatenatedBuffer, 0, pieceLength);
						concatenate(concatenatedBuffer, receivedBuffer, pieceLength, 2*pieceLength);
						merge(concatenatedBuffer, 0, pieceLength, 2*pieceLength);

						depth--;
					}
				}
				break;		
			}
		}
	}

  	MPI_Finalize();
}
