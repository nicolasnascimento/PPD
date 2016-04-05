
#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>


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
	for(i = 0; i < length; i++) {
		printf("%d ", array[i]);
	}
	printf("\n");
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
	getArrayFromFileWithName(sortedArray[arrayLength], fileName, arrayLength)
	double initialTime, finalTime;
	MPI_Status status;
	int valueReceived = 0;
	int doneMessage = -1;
	int elementsSorted = 0;

	// Inicialização do MPI
	int rank,size;
	MPI_Init(&argc, &argv);
  	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  	MPI_Comm_size(MPI_COMM_WORLD, &size);


	// Variaveis especificas por processo
	int pieceLength = arrayLength/size;
	int sortedArrayPosition = rank*pieceLength;
	
	// Primeiro Estágio do Pipe
	if( rank == 0 ) {
		for( int i = 0; i < arrayLength; i++, elementsSorted++ ) {
			int lastValue = sortedArray[sortedArrayPosition + pieceLength - 1];
			
			// Ordena o vetor enquanto não estiver cheio			
			if( elementsSorted < pieceLength ) {
				sortedArray[i] = array[i];
				insertionSort(sortedArray, sortedArrayPosition, sortedArray, elementsSorted);
				elementsSorted++;
			// Se último valor de 'sortedArray' for menor do que o valor testado, envia para o próximo estágio
			}else if( lastValue < array[i]  ) {
				//sortedArray[elementsSorted] = array[elementsSorted];
				MPI_Send(&(array[i]), sizeof(int), MPI_INT, rank + 1, 0, MPI_COMM_WORLD);	
			// Senão, ordena e manda o último valor para o pŕoximo estágio			
			}else {
				sortedArray[elementsSorted] = array[elementsSorted]
				insertionSort(sortedArray, sortedArrayPosition, sortedArray, elementSorted);
				MPI_Send(&(sortedArray[elementsSorted]), sizeof(int), MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
			}		
		}
		// Imprime o array ordenado e manda a mensagem de "finalizado"
		MPI_Recv(&valueReceived, sizeof(int), MPI_INT, size - 1, 0, MPI_COMM_WORLD, &status);
		if( valueReceived == -1 ) {
			printArray(sortedArray, elementsSorted);
			MPI_Send(&(doneMessage), sizeof(int), MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
		}

	// Estágios Intermediários
	}else if( rank + 1 < size ){
		// Espera pela mensagem do estágio anterior
		while(1) {
			int valueReceived = 0;
			MPI_Recv(&valueReceived, sizeof(int), MPI_INT, rank - 1, 0, MPI_COMM_WORLD, &status);
			// Se recebeu a mensagem de fim, finaliza o loop
			if( valueReceived == -1 ) {			
				break;
			// Ordena o vetor enquanto não estiver cheio
			}else if( elementsSorted < pieceLength ) {
				sortedArray[elementsSorted] = valueReceived;
				insertionSort(sortedArray, sortedArrayPosition, sortedArray, elementSorted);
				elementsSorted++;
			// Se último valor de 'sortedArray' for menor do que o valor testado, envia para o próximo estágio
			}else if( lastValue < valueReceived  ) {
				//sortedArray[elementsSorted] = array[elementsSorted];
				MPI_Send(&(valueReceived), sizeof(int), MPI_INT, rank + 1, 0, MPI_COMM_WORLD);	
			// Senão, ordena e manda o último valor para o pŕoximo estágio			
			}else {
				sortedArray[elementsSorted] = array[elementsSorted]
				insertionSort(sortedArray, sortedArrayPosition, sortedArray, elementSorted);
				MPI_Send(&(sortedArray[elementsSorted]), sizeof(int), MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
			}
		}
		// Imprime o array de valores
		printArray(sortedArray, elementsSorted);
		MPI_Send(&(doneMessage), sizeof(int), MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
	// Ultimo Estágio	
	}else{
		// Espera pela mensagem do estágio anterior
		while(1) {
			
			// Recebe e ordena o valor recebido
			MPI_Recv(&valueReceived, sizeof(int), MPI_INT, rank - 1, 0, MPI_COMM_WORLD, &status);
			
			// Insere e ordena
			if( elementsSorted < pieceLength ) {
				sortedArray[elementsSorted] = valueReceived;
				insertionSort(sortedArray, sortedArrayPosition, sortedArray, elementSorted);
				elementsSorted++;
				// Se estiver cheio, inicia o processo de finalização
				if( elementsSorted == pieceLength ) {
					MPI_Send(&(doneMessage), sizeof(int), MPI_INT, 0, 0, MPI_COMM_WORLD);
					MPI_Recv(&valueReceived, sizeof(int), MPI_INT, rank - 1, 0, MPI_COMM_WORLD, &status);
					printArray(sortedArray, elementsSorted);
					break;			
				}
			}
		}
	}

	MPI_Finalize();	

}
