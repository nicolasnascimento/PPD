
#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

// Flag utilizada para utilização de prints
// Comentar esta linha evita que os prints sejam compilados
#define ENABLE_DEBUG_PRINTS

// Realiza o Insertion Sort no vetor 'sourceArray' armazenando o resultado em 'targetArray'
void insertionSort( int* targetArray, int targetArrayInitialPosition, int* sourceArray, int size ) {
	int i = 0, j = 0;
	for( i = targetArrayInitialPosition; i < size; i++ ) {
		int currentValue = sourceArray[i];
		for( j = targetArrayInitialPosition; j < i; j++ ) {
			if( currentValue < targetArray[j] ) {
				int aux = targetArray[j];
				targetArray[j] = currentValue;
				currentValue = aux;
			}		
		}
		targetArray[i] = currentValue;	
	}
}

// Retorna o tempo atual em milisegundos
double getCurrentTimeMS(){
	struct timeval time;
	gettimeofday(&time, NULL);
	return  (time.tv_sec) * 1000 + (time.tv_usec) / 1000;
}

// Função auxiliar que mostar os valores em um array
void printArray(int* array, int length) {
	int i = 0;
    printf("----------------------------------\n");
	for(i = 0; i < length; i++) {
		printf("%d\n", array[i]);
	}
	printf("----------------------------------\n");
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

int main(int argc, char **argv) {

	// Confere a existência dos parâmetros necessários
	if( argc != 3 ) {
		printf("Incorrent number Of Arguments, expected 3\n");
		return -1;
	}

	// Inicialização de variáveis usadas em todos os processos
	char* fileName = argv[2];
	int arrayLength = atoi(argv[1]);
	int array[arrayLength], sortedArray[arrayLength], buffer[arrayLength];
	getArrayFromFileWithName(array, fileName, arrayLength);
	getArrayFromFileWithName(sortedArray, fileName, arrayLength);
	
	// Inicialização do MPI
	int rank,size;
	MPI_Init(&argc, &argv);
  	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  	MPI_Comm_size(MPI_COMM_WORLD, &size);

	double initialTime, finalTime;
	MPI_Status status;
	int valueReceived = 0;
	int doneMessage = -1;
	int elementsSorted = 0;


	// Variaveis especificas por processo
	int pieceLength = arrayLength/size;
	int sortedArrayPosition = rank*pieceLength;
	
	#ifdef ENABLE_DEBUG_PRINTS
		printf("%d initializing\n",rank);
	#endif	
    
    // Sequencial
    if( size == 1 ) {
        initialTime = getCurrentTimeMS();
        #ifdef ENABLE_DEBUG_PRINTS
            printf("Initing - Sequential\n");
        #endif
        insertionSort(array, 0, array, arrayLength);
        finalTime = getCurrentTimeMS();
        printArray(array, arrayLength);
        printf("totalTime: %lf ms\n", finalTime - initialTime);
	// Primeiro Estágio do Pipe
    }else if( rank == 0 ) {
        initialTime = getCurrentTimeMS();
        elementsSorted = pieceLength;
        insertionSort(sortedArray, 0, array, pieceLength);
		int lastValue = 0;
		for( int i = elementsSorted; i < arrayLength; i++/*, elementsSorted++*/ ) {

			lastValue = sortedArray[elementsSorted - 1];
			
			// Ordena o vetor enquanto não estiver cheio			
			if( elementsSorted < pieceLength ) {

				#ifdef ENABLE_DEBUG_PRINTS
					printf("%d sorting array\n",rank);		
				#endif
	
				sortedArray[i] = array[i];
				insertionSort(sortedArray, 0, sortedArray, elementsSorted);
				elementsSorted++;
			// Se último valor de 'sortedArray' for menor do que o valor testado, envia para o próximo estágio
			}else if( lastValue < array[i]  ) {
				#ifdef ENABLE_DEBUG_PRINTS
					printf("%d sending value to next\n",rank);
				#endif

				//sortedArray[elementsSorted] = array[elementsSorted];
				MPI_Send(&(array[i]), sizeof(int), MPI_INT, rank + 1, 0, MPI_COMM_WORLD);	
			// Senão, ordena e manda o último valor para o pŕoximo estágio			
			}else {
				#ifdef ENABLE_DEBUG_PRINTS
					printf("%d sending current last value to next\n",rank);
				#endif
				sortedArray[elementsSorted] = array[i];
				insertionSort(sortedArray, 0, sortedArray, elementsSorted + 1);
				MPI_Send(&(lastValue), sizeof(int), MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
			}		
		}
		#ifdef ENABLE_DEBUG_PRINTS
			printf("%d waiting for done message\n",rank);		
		#endif

		// Imprime o array ordenado e manda a mensagem de "finalizado"
		MPI_Recv(&valueReceived, sizeof(int), MPI_INT, size - 1, 0, MPI_COMM_WORLD, &status);
		if( valueReceived == -1 ) {
			#ifdef ENABLE_DEBUG_PRINTS
				printf("%d printing\n",rank);		
			#endif

			printArray(sortedArray, elementsSorted);
            finalTime = getCurrentTimeMS();
            printf("totalTime: %lf ms\n", finalTime - initialTime);
			#ifdef ENABLE_DEBUG_PRINTS
				printf("%d forwarding done message\n",rank);		
			#endif
			MPI_Send(&(doneMessage), sizeof(int), MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
		}

	// Estágios Intermediários
	}else if( rank + 1 < size ){
		elementsSorted = 0;
		int valueReceived = 0;
		int lastValue = 0;
		// Espera pela mensagem do estágio anterior
		while(1) {
			lastValue = elementsSorted == 0 ? 0 : sortedArray[elementsSorted - 1];

			#ifdef ENABLE_DEBUG_PRINTS
				printf("%d waiting\n",rank);
			#endif			
		
			MPI_Recv(&valueReceived, sizeof(int), MPI_INT, rank - 1, 0, MPI_COMM_WORLD, &status);
			#ifdef ENABLE_DEBUG_PRINTS
				printf("%d receiving %d from %d, elementsSorted = %d\n",rank, valueReceived, status.MPI_SOURCE, elementsSorted);
			#endif	

			// Se recebeu a mensagem de fim, finaliza o loop
			if( valueReceived == -1 ) {
				#ifdef ENABLE_DEBUG_PRINTS
					printf("%d received done message\n",rank);		
				#endif
				break;
			// Ordena o vetor enquanto não estiver cheio
			}else if( elementsSorted < pieceLength ) {
				#ifdef ENABLE_DEBUG_PRINTS
					printf("%d sorting array\n",rank);
				#endif
		
				sortedArray[elementsSorted] = valueReceived;
				insertionSort(sortedArray, 0, sortedArray, elementsSorted);
				elementsSorted++;
			// Se último valor de 'sortedArray' for menor do que o valor testado, envia para o próximo estágio
			}else if( lastValue < valueReceived  ) {
				#ifdef ENABLE_DEBUG_PRINTS
					printf("%d sending value to next\n",rank);
				#endif

				//sortedArray[elementsSorted] = array[elementsSorted];
				MPI_Send(&(valueReceived), sizeof(int), MPI_INT, rank + 1, 0, MPI_COMM_WORLD);	
			// Senão, ordena e manda o último valor para o pŕoximo estágio			
			}else {
				#ifdef ENABLE_DEBUG_PRINTS
					printf("%d sending current last value to next\n",rank);
				#endif

				sortedArray[elementsSorted] = valueReceived;
				insertionSort(sortedArray, 0, sortedArray, elementsSorted + 1);
				MPI_Send(&(lastValue), sizeof(int), MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
			}
		}
		// Imprime o array de valores
		#ifdef ENABLE_DEBUG_PRINTS
			printf("%d printing\n",rank);		
		#endif

		printArray(sortedArray, elementsSorted);

		#ifdef ENABLE_DEBUG_PRINTS
			printf("%d sending done message\n",rank);		
		#endif
		MPI_Send(&(doneMessage), sizeof(int), MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
	// Ultimo Estágio	
	}else{
		// Espera pela mensagem do estágio anterior
		while(1) {
			#ifdef ENABLE_DEBUG_PRINTS
				printf("%d waiting\n",rank);		
			#endif			

			// Recebe e ordena o valor recebido
			MPI_Recv(&valueReceived, sizeof(int), MPI_INT, rank - 1, 0, MPI_COMM_WORLD, &status);
			
			// Insere e ordena
			if( elementsSorted < pieceLength ) {
				#ifdef ENABLE_DEBUG_PRINTS
				//	printf("%d sorting array\n",rank);
				#endif			

				sortedArray[elementsSorted] = valueReceived;
				insertionSort(sortedArray, 0, sortedArray, elementsSorted);
				elementsSorted++;
				// Se estiver cheio, inicia o processo de finalização
				if( elementsSorted == pieceLength ) {
					#ifdef ENABLE_DEBUG_PRINTS
						printf("%d sending done message\n",rank);
					#endif	
					MPI_Send(&(doneMessage), sizeof(int), MPI_INT, 0, 0, MPI_COMM_WORLD);

					#ifdef ENABLE_DEBUG_PRINTS
						printf("%d waiting for print message\n",rank);		
					#endif
					MPI_Recv(&valueReceived, sizeof(int), MPI_INT, rank - 1, 0, MPI_COMM_WORLD, &status);
					printArray(sortedArray, elementsSorted);
					break;			
				}
			}
		}
	}
	

	MPI_Finalize();	

}
