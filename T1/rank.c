// execute with 10k ,50k, 100k
// execute sequecial, and then with 1 slave

/*
PROBLEMA:
	O objetivo do trabalho é implementar, usando a biblioteca MPI, uma versão paralela, usando o modelo mestre escravo, do algoritmo de ordenação Rank Sort.
	O programa deverá receber como parâmetros de entrada um número N, representando o número de valores a ser testado e o nome de um arquivo, que contém a lista de valores inteiros a serem ordenados. Utilizar 3 casos de teste para realização das medições no cluster.
	A saída que deve ser gerada é a lista ordenada (crescente) dos valores de entrada (1 valor por linha) e também o tempo de execução da aplicação.

SOLUÇÃO:
	??

Gabriel Chiele, Nicolas Nascimento - 29/03/2016

*/

#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

// Realiza o Ranksort no vetor 'sourceArray' armazenando o resultado em 'targetArray' 
void rankSort( int* targetArray, int* sourceArray, int size ) {
	int index = 0;
	int i, j;
	for( i = 0; i < size; i++ ) {
		index = 0;
		for( j = 0; j < size; j++ ) {
			if( sourceArray[i] > sourceArray[j] ) {
				index++;
			}
		}
		targetArray[index] = sourceArray[i];
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

// Concatena o vetor 'sourceArray' no vetor 'targetArray'
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
	for(i = 0; i < length; i++) {
		printf("%d ", array[i]);
	}
	printf("\n");
}

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

int main(int argc, char **argv) {

	// Confere a existência dos parâmetros necessários
	if( argc != 3 ) {
		printf("Incorrent number Of Arguments, expected 3\n");
		return -1;
	}

	// Inicialização de variáveis
	char* fileName = argv[2];
	int arrayLength = atoi(argv[1]);
	int array[arrayLength], sortedArray[arrayLength], buffer[arrayLength];
	getArrayFromFileWithName(array, fileName, arrayLength);
	time_t initialTime, finalTime;

	// Inicialização do MPI
	int rank,size;
	MPI_Init(&argc, &argv);
  	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  	MPI_Comm_size(MPI_COMM_WORLD, &size);

	printf("Main\n");
	// Execução sequencial
	if( size == 1 ) {
		printf("Sequential\n");
		time(&initialTime);
		rankSort(sortedArray, array, arrayLength);
		time(&finalTime);
		printf("difftime = %lf\n", difftime(finalTime, initialTime));

		// TODO - remover linhas abaixo, pois a saida deve ser em arquivo txt
		/*printArray(sortedArray, arrayLength);
		printf("---\n");
		printArray(array, arrayLength);*/

	// Código do Mestre
	}else if( rank == 0 ) {
		// Tamanho do vetor que será passado aos escravos
		int pieceLength = arrayLength/(4*size);
		// Variáveis auxiliares de controle
		int amountSent = 0, firstExec = 1, i = 0;
		int amountRecv = 0;
		MPI_Status status;

		time(&initialTime);
		while( amountSent < arrayLength ) {
			if( firstExec ) {
				// Na primeira execução, envia-se partes a todos os escravos
                		printf("sending first piece\n");
				for( i = 1; i < size; i++ ) {
					MPI_Send(array, arrayLength, MPI_INT, i, 0, MPI_COMM_WORLD);
					MPI_Send(&(amountSent),1, MPI_INT, i, 0, MPI_COMM_WORLD);
					MPI_Send(&(pieceLength),1, MPI_INT, i, 0, MPI_COMM_WORLD);
					amountSent += pieceLength;
				}
				firstExec = 0;
			}else{
               			printf("receiving piece\n");
				// Espera a resposta de um escravo
				MPI_Recv(buffer, pieceLength, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
				// Concatena o vetor retornado pelo escravo com o vetor 'sortedArray'
                		printf("Concatenating piece\n");
				concatenate(sortedArray, buffer, amountRecv, pieceLength);
				merge(sortedArray, 0, amountRecv, amountRecv + pieceLength);
				amountRecv+= pieceLength;
				int source = status.MPI_SOURCE;
                		printf("sending piece\n");
				// Envia novo pedaço ao escravo que completou sua tarefa
				if ((amountSent + pieceLength) > arrayLength){
					pieceLength = arrayLength % (4*size)
				}
				MPI_Send(array, arrayLength, MPI_INT, source, 0, MPI_COMM_WORLD);
				MPI_Send(&(amountSent),1, MPI_INT, source, 0, MPI_COMM_WORLD);
				MPI_Send(&(pieceLength),1, MPI_INT, source, 0, MPI_COMM_WORLD);
				amountSent += pieceLength;
			}
		}
		// Envia uma menssagem aos escravos informando que acabou o trabalho
		int done = -1;
        	printf("sending done message\n");
		for( i = 1; i < size; i++ ) {
			MPI_Send(array, arrayLength, MPI_INT, i, 0, MPI_COMM_WORLD);
			MPI_Send(&(done),1, MPI_INT, i, 0, MPI_COMM_WORLD);
			MPI_Send(&(done),1, MPI_INT, i, 0, MPI_COMM_WORLD);
		}

		time(&finalTime);
        
		printf("difftime = %lf\n", difftime(finalTime, initialTime));
		//printArray(sortedArray, arrayLength);
        writeDeltaTimeAndArrayToFileWithName(difftime(finalTime, initialTime), sortedArray, arrayLength, "output.txt");
	// Código do Escravo
	}else {
		int position, length;
		MPI_Status status;
		while(1) {
            		printf("receiving piece to sort\n");
			// Recebe o vetor e as posições que indicam em que parte do vetor ele deve trabalhar
			MPI_Recv(buffer, arrayLength, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
			MPI_Recv(&position, sizeof(int), MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
			MPI_Recv(&length, sizeof(int), MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
			// Se as posições recebidas forem -1, significa que o trabalho acabou
			if( position == -1 && length == -1 ) {
				break;
			}else{
				// Copia a parte ordenada em um vetor menor para retornar ao Mestre
				int copyBuffer[length], sortedBuffer[length];
				copyArrayToBuffer(buffer, copyBuffer, position, length);
				rankSort(sortedArray, copyBuffer, length);
                		printf("sending sorted piece\n");
				MPI_Send(sortedArray, length, MPI_INT, 0, 0, MPI_COMM_WORLD);
			}
		}
	}

  	MPI_Finalize();
}
