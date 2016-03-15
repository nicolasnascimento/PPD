#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>


// Defines the portion of the array to send to slaves
typedef struct ArrayPiece {
	int *array;
	int length;
} ArrayPiece;

// Initializes a given array piece with the parameters passed
void initArrayPiece(struct ArrayPiece* piece, int* array, int length) {
	piece->array = array;
	piece->length = length;
}

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
	int array[arrayLength], sortedArray[arrayLength];
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
		int arrayPortion = arrayLength/(4*size);
		int amountSent = 0, processorsUsed = 0;
		ArrayPortion portion;

		// Assures there will be no missing parts
		//int divisionRest = arrayLength % (4*size);
		while( amountSent < arrayLength ) {
			if( processorsUsed < size ) {
				// TODO
				/*initArrayPiece(&portion, array, length);
				MPI_Send()*/			
			}
		}
		

		time(&finalTime);
		printf("difftime = %d\n", difftime(finalTime, initialTime));
	// Slave
	}else {
			
	}

  	MPI_Finalize();
  	return 0;
}
