// execute with 10k ,50k, 100k
// execute sequecial, and then with 1 slave

/*

Gabriel Chiele
Nicolas Nascimento

Master-slave sorting implemetation - PPD

*/

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

// Copy a part of the array variable, starting at arrayInitialPosition into the buffer array
void copyArrayToBuffer(int* array, int* buffer, int arrayInitialPosition, int arrayLength) {
	int i;
	int j = 0;
	for( i = arrayInitialPosition; i < (arrayInitialPosition + arrayLength); i++  ) {
		buffer[j] = array[i];
		j++;
	}
}

// Populates array variable with 'numberOfItems' number of values from a given file
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

// Concatenate the sourceArray into targetArray
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

	// Sequential
	if( size == 1 ) {
		time(&initialTime);
		rankSort(sortedArray, array, arrayLength);
		time(&finalTime);
		printf("difftime = %d\n", difftime(finalTime, initialTime));
	// Master
	}else if( rank == 0 ) {
		// Array size to give to all slaves
		int pieceLength = arrayLength/(4*size);
		int amountSent = 0, firstExec = 1, i = 0;
		int amountRecv = 0;
		MPI_Status status;
		ArrayPortion portion;

		time(&initialTime);
		// Assures there will be no missing parts
		// int divisionRest = arrayLength % (4*size);
		while( amountSent < arrayLength ) {
			if( firstExec ) {
				// First Execution - Send a message to all slaves
				for( i = 1; i < size; i++ ) {
					MPI_Send(array, arrayLength, MPI_INT, i, NULL, MPI_COMM_WORLD);
					MPI_Send(&(amountSent),1, MPI_INT, i, NULL, MPI_COMM_WORLD);
					MPI_Send(&(arrayLength),1, MPI_INT, i, NULL, MPI_COMM_WORLD);
					amountSent += arrayPortion;
				}
				firstExec = 0;
			}else{
				// Wait to receive response from a slave
				MPI_Recv(buffer, pieceLength, MPI_INT, MPI_ANY_SOURCE, NULL, MPI_COMM_WORLD, &status);
				// Concatenate the array return by the slave into sortedArray
				concatenate(sortedArray, buffer, amountRecv, pieceLength);
				merge(sortedArray, 0, amountRecv, amountRecv + pieceLenth);
				amountRecv+= arrayPortion;
				int source = status.MPI_SOURCE;
				// Send a new part to the slave that just finished his work
				MPI_Send(array, arrayLength, MPI_INT, source, NULL, MPI_COMM_WORLD);
				MPI_Send(&(amountSent),1, MPI_INT, source, NULL, MPI_COMM_WORLD);
				MPI_Send(&(arrayLength),1, MPI_INT, source, NULL, MPI_COMM_WORLD);
				amountSent += arrayPortion;
			}
		}
		// Send a message to the slaves, informing that the sorting is finished
		int done = -1;
		for( i = 1; i < size; i++ ) {
			MPI_Send(array, arrayLength, MPI_INT, i, NULL, MPI_COMM_WORLD);
			MPI_Send(&(done),1, MPI_INT, i, NULL, MPI_COMM_WORLD);
			MPI_Send(&(done),1, MPI_INT, i, NULL, MPI_COMM_WORLD);
		}

		time(&finalTime);
		printf("difftime = %d\n", difftime(finalTime, initialTime));
	// Slave
	}else {
		int position, length;
		while(1) {
			// receive the array and the positions where he will start and end his operations
			MPI_Recv(buffer, arrayLength, MPI_INT, 0, NULL, MPI_COMM_WORLD, &status);
			MPI_Recv(&position, sizeof(int),arrayLength, MPI_INT, 0, NULL, MPI_COMM_WORLD, &status);
			MPI_Recv(&length, sizeof(int), MPI_INT, 0, NULL, MPI_COMM_WORLD, &status);
			// if positions received are -1, it means that the work is done
			if( position == -1 && length == -1 ) {
				break;
			}else{
				// copy the his part into a smaller array, then sort and return to the master
				int copyBuffer[length], sortedBuffer[length];
				copyArrayToBuffer(buffer, copyBuffer, position, length);
				rankSort(sortedArray, copyBuffer, length);
				MPI_Send(sortedArray, length, MPI_INT, 0, NULL, MPI_COMM_WORLD);
			}
		}
	}

  	MPI_Finalize();
  	return 0;
}
