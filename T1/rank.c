#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

// Uses Rank Sort to sort a given array and stores it in the targetArray
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
		targetArray[i] = sourceArray[index];
	}
}
void copyArrayToBuffer(int* array, int* buffer, int arrayInitialPosition, int arrayLength) {
	int i;
	int j = 0;
	for( i = arrayInitialPosition; i < (arrayInitialPosition + arrayLength); i++  ) {
		buffer[j] = array[i];
		j++;	
	}
}

void getArrayFromFileWithName( int* array, char* fileName, int numberOfItems ) {
	FILE* fp = fopen(fileName, "r");
	if( !fp ) {
		printf("error while opening file = %s\n", fileName);	
	}else{
		int currentItem = 0;
		while( currentItem < numberOfItems ) {
			fscanf(fp,"%d",&(array[currentItem]));
			printf("array[%d] = %d\n", currentItem, array[currentItem]);			
			currentItem++;		
		}
	}
	fclose(fp); 
}

void concatenate(int* targetArray, int* sourceArray, int position, int length) {
	int i, j;
	for( i = position, j = 0; i < ( i + length); i++ ) {
		target[i] = source[j];
		j++;
	}
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

	if( argc != 3 ) {
		printf("Incorrent number Of Arguments, expected 3\n");
		return -1;		
	}

	char* fileName = argv[2];
	int arrayLength = atoi(argv[1]);
	int array[arrayLength], sortedArray[arrayLength], buffer[arrayLength];
	getArrayFromFileWithName(array, fileName, arrayLength);
	time_t initialTime, finalTime;

	int rank,size;
	MPI_Init(&argc, &argv);
  	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  	MPI_Comm_size(MPI_COMM_WORLD, &size);
	
	// Sequencial
	if( size == 1 ) {
		time(&initialTime);
		rankSort(sortedArray, array, arrayLength);
		time(&finalTime);
		printf("difftime = %d\n", difftime(finalTime, initialTime));
	// Master
	}else if( rank == 0 ) {
		time(&initialTime);
	
		// Array size to give to all slaves
		int pieceLength = arrayLength/(4*size);
		int amountSent = 0, firstExec = 1, i = 0;
		int amountRecv = 0;
		MPI_Status status;
		ArrayPortion portion;

		// Assures there will be no missing parts
		//int divisionRest = arrayLength % (4*size);
		while( amountSent < arrayLength ) {
			if( firstExec ) {
				// First Execution - Send a message to all slaves
				for( i = 1; i < size; i++ ) {
					MPI_Send(array, arrayLength, MPI_INT, i, NULL, MPI_COMM_WORLD);
					MPI_Send(&(amountSent), MPI_INT, i, NULL, MPI_COMM_WORLD);
					MPI_Send(&(arrayLength), MPI_INT, i, NULL, MPI_COMM_WORLD);
					amountSent += arrayPortion;
				}
				firstExec = 0;		
			}else{
				MPI_Recv(buffer, pieceLength, MPI_INT, MPI_ANY_SOURCE, NULL, MPI_COMM_WORLD, &status);
				concatenate(sortedArray, buffer, amountRecv, pieceLength);
				merge(sortedArray, 0, amountRecv, amountRecv + pieceLenth);
				amountRecv+= arrayPortion;
				int source = status.MPI_SOURCE;
				MPI_Send(array, arrayLength, MPI_INT, source, NULL, MPI_COMM_WORLD);
				MPI_Send(&(amountSent), MPI_INT, source, NULL, MPI_COMM_WORLD);
				MPI_Send(&(arrayLength), MPI_INT, source, NULL, MPI_COMM_WORLD);
				amountSent += arrayPortion;	
			}
		}
		int done = -1;
		for( i = 1; i < size; i++ ) {
			MPI_Send(array, arrayLength, MPI_INT, i, NULL, MPI_COMM_WORLD);
			MPI_Send(&(done), MPI_INT, i, NULL, MPI_COMM_WORLD);
			MPI_Send(&(done), MPI_INT, i, NULL, MPI_COMM_WORLD);
		}
		

		time(&finalTime);
		printf("difftime = %d\n", difftime(finalTime, initialTime));
	// Slave
	}else {
		int position, length;
		while(1) {
			MPI_Recv(buffer, arrayLength, MPI_INT, 0, NULL, MPI_COMM_WORLD, &status);
			MPI_Recv(&position, sizeof(int),arrayLength, MPI_INT, 0, NULL, MPI_COMM_WORLD, &status);
			MPI_Recv(&length, sizeof(int), MPI_INT, 0, NULL, MPI_COMM_WORLD, &status);
			if( position == -1 && length == -1 ) {
				break;			
			}else{
				// TODO - Copy Array received to buffer, sort and send back;			
			}
		}
	}

  	MPI_Finalize();
  	return 0;
}
