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
	for( i = arrayInitialPosition; i < (arrayInitialPosition + arrayLength); i++, j++ ) {
		buffer[j] = array[i];
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
	for( i = position, j = 0; i < ( position + length); i++, j++ ) {
		targetArray[i] = sourceArray[j];
	}
}

// Função auxiliar que mostar os valores em um array
void printArray(int* array, int length) {
	int i = 0;
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

// Retorna a profundidade minima de um processo na hierarquia para que o mesmo possa ser executado
int minimumDepthForRank(int rank, int size) {
    int currentDepth = 0;
    int maxDepth = log2(size);
    int counter = 0;
    if( rank == 0 ) {
        return 0;
    }else{
        do{
            if( counter == rank ) {
                return currentDepth;
            }
            if( counter >= pow(2, currentDepth) - 1 ) {
                currentDepth++;
            }
            counter++;
        }while(counter < pow(2, maxDepth));
        
        return -1;
    }
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
	//	printf("Main\n");
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
            printf("%d - 0 initializing\n", rank);
        #endif

        int depth = minimumDepthForRank(rank, size);
        int pieceLength;// = arrayLength/(pow(2, depth));
        int childRank = -1;// = rank + pow(2, depth - 1);
        int parentRank = -1;// = rank - pow(2, depth - 1);
        int firstReception = 1;//

        if( rank == 0 ) {

            #ifdef ENABLE_DEBUG_PRINTS
            //    printf("%d reading file\n", rank);
            #endif

            getArrayFromFileWithName(array, fileName, arrayLength);
            depth = 1;
            pieceLength = arrayLength/(pow(2, depth));
            childRank = 1;
            parentRank = -1;
            
            #ifdef ENABLE_DEBUG_PRINTS
                printf("%d - 1 sending initial load of %d to child %d\n", rank, pieceLength, childRank);
            #endif

            //printArray(array, arrayLength);
            //printArray(array + pieceLength, pieceLength);

            MPI_Send((array + pieceLength), pieceLength, MPI_INT, childRank, 0, MPI_COMM_WORLD);
        }

		while(1) {
            

            if( rank != 0 && parentRank == -1 ) {
                parentRank = rank - pow(2, depth - 1);
            }
            childRank = rank + pow(2, depth);
            pieceLength = arrayLength/(pow(2, depth));

            int buffer[pieceLength];
            int receivedBuffer[pieceLength];
            //int concatenatedBuffer[arrayLength];
            
            if( rank == 0 && firstTime == 0 ) {
                copyArrayToBuffer(array, buffer, 0, arrayLength);
                #ifdef ENABLE_DEBUG_PRINTS
                    printf("%d - 2 copying array for self\n", rank);
                    printArray(buffer, pieceLength);
                #endif
            }

            if( rank != 0 && firstReception != 0 ) {
                
                #ifdef ENABLE_DEBUG_PRINTS
                    printf("%d - 3 waiting to receive load of %d from parent %d\n", rank, pieceLength, parentRank);
                #endif

                //while (1);
                MPI_Recv(buffer, pieceLength, MPI_INT, parentRank, 0, MPI_COMM_WORLD, &status);

                #ifdef ENABLE_DEBUG_PRINTS
                    printf("%d - 4 done receiving\n", rank);
                    printf("%d - 5 depth %d\n", rank, depth);
                    printf("%d - 5.2 ", rank);
                    printArray(buffer, pieceLength);
                    //printArray(buffer, pieceLength);
                #endif
                firstReception = 0;
            }

            if( pieceLength > arrayLength/size ) {

                depth++;
                pieceLength = arrayLength/(pow(2, depth));
                
                #ifdef ENABLE_DEBUG_PRINTS
                    printf("%d - 6 sending load of %d to child %d\n", rank, pieceLength, childRank);
                    printf("%d - 6.2 ", rank);
                    printArray((buffer + pieceLength), pieceLength);
                #endif
                
                MPI_Send((buffer + pieceLength), pieceLength, MPI_INT, childRank, 0, MPI_COMM_WORLD);
			}else{
                #ifdef ENABLE_DEBUG_PRINTS
                    printf("%d - 7 sorting\n", rank);
                    printf("%d - 7.1 buffer to sort ", rank);
                    printArray(buffer, pieceLength);
                #endif

				mergeSort(buffer, 0, pieceLength);
                
                #ifdef ENABLE_DEBUG_PRINTS
                    printf("%d - 8 done sorting\n", rank);
                    printf("%d - 8.8 final depth %d\n", rank, depth);
                #endif
                
                int firstTime = 0;
                int *concatenatedBuffer = NULL;
                int oldSize = 0;
                int i = 0;

				while( depth >= 0 ) {

					if( rank >= pow(2, depth - 1) ) {

                        #ifdef ENABLE_DEBUG_PRINTS
                            printf("%d.%d - 9 sending load of %d to parent %d\n", rank, i, firstTime == 0 ? pieceLength : 2*pieceLength, parentRank);
                            if( firstTime == 0 ) {
                                printf("%d.%d - 9.2 ", rank, i);
                                printArray(buffer, pieceLength);
                            }else{
                                printf("%d.%d - 9.2 ", rank, i);
                                printArray(concatenatedBuffer, 2*pieceLength);
                            }
                            //printArray(buffer, pieceLength);
                        #endif
                        if( firstTime == 0 ) {
                            MPI_Send(buffer, pieceLength, MPI_INT, parentRank, 0, MPI_COMM_WORLD);
                        }else{
                            MPI_Send(concatenatedBuffer, 2*pieceLength, MPI_INT, parentRank, 0, MPI_COMM_WORLD);
                            free(concatenatedBuffer);
                        }
                        break;
					} else {
                        depth--;
                        childRank = rank + pow(2, depth);
                        
                        //if( depth != 0 ) {
                            pieceLength = arrayLength/(pow(2, depth + 1));
                        //}
                        
                        #ifdef ENABLE_DEBUG_PRINTS
                            printf("%d.%d - 9.5 waiting to receive load of %d for child %d\n", rank, i, pieceLength, childRank);
                        #endif

						MPI_Recv(receivedBuffer, pieceLength, MPI_INT, childRank, 0, MPI_COMM_WORLD, &status);
                        
                        
                        #ifdef ENABLE_DEBUG_PRINTS
                            printf("%d.%d - 9.6 received buffer ", rank, i);
                            printArray(receivedBuffer, pieceLength);
                        #endif
                        
                        if( concatenatedBuffer == NULL ) {
                            concatenatedBuffer = (int*) malloc(sizeof(int)*2*pieceLength);
                            oldSize = sizeof(int)*2*pieceLength;
                        }else{
                            
                            #ifdef ENABLE_DEBUG_PRINTS
                                printf("%d.%d - 9.65 before reallocating buffer ", rank, i);
                                printArray(concatenatedBuffer, oldSize/(sizeof(int)));
                            #endif
                            
                            concatenatedBuffer = (int*)realloc(concatenatedBuffer, oldSize*2);
                            oldSize = oldSize*2;
                            #ifdef ENABLE_DEBUG_PRINTS
                                printf("%d.%d - 9.66 after reallocating buffer ", rank, i);
                                printArray(concatenatedBuffer, oldSize/(sizeof(int)));
                            #endif
                            
                        }
                        
                        //printArray(receivedBuffer, pieceLength);
                        if( firstTime == 0 ) {
                            copyArrayToBuffer(buffer, concatenatedBuffer, 0, pieceLength);
                        }
                        
                        #ifdef ENABLE_DEBUG_PRINTS
                            printf("%d.%d - 9.7 concatenated buffer before ", rank, i);
                            printArray(concatenatedBuffer, 2*pieceLength);
                        #endif
                        
						concatenate(concatenatedBuffer, receivedBuffer, pieceLength, pieceLength);
                        
                        #ifdef ENABLE_DEBUG_PRINTS
                            printf("%d.%d - 9.8 concatenated buffer after ", rank, i);
                            printArray(concatenatedBuffer, 2*pieceLength);
                        #endif
                        
						merge(concatenatedBuffer, 0, pieceLength, 2*pieceLength);
                        
                        #ifdef ENABLE_DEBUG_PRINTS
                            printf("%d.%d - 9.9 concatenated buffer merged ", rank, i);
                            printArray(concatenatedBuffer, 2*pieceLength);
                        #endif
                        firstTime = 1;
                        
                        if( depth == 0 && rank == 0 ) {
                            printArray(concatenatedBuffer, 2*pieceLength);
                            break;
                        }
					}
                    i++;
				}
				break;
			}
		}
	}

  	MPI_Finalize();
}
