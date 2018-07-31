// Author: Wes Kendall
// Copyright 2011 www.mpitutorial.com
// This code is provided freely with the tutorials on mpitutorial.com. Feel
// free to modify it for your own use. Any distribution of the code must
// either provide a link to www.mpitutorial.com or keep this header intact.
//
// MPI_Send, MPI_Recv example. Communicates the number -1 from process 0
// to process 1.
//
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#define sz 1000
int main(int argc, char** argv) {
  // Initialize the MPI environment
  MPI_Init(NULL, NULL);
  // Find out rank, size
  int world_rank,i;
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
  int world_size;
  MPI_Request request;
  MPI_Status status;
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  // We are assuming at least 2 processes for this task
  if (world_size < 2) {
    fprintf(stderr, "World size must be greater than 1 for %s\n", argv[0]);
    MPI_Abort(MPI_COMM_WORLD, 1);
  }
 // If we are rank 0, set the number to -1 and send it to process 1

  //char  ltr[sz];
  char ltr;
  //for(i=0;i<sz;i++)
    // ltr[i]='a';
   ltr ='a';
  if (world_rank == 0) {   
    for(i=0;i<sz;i++)
     ltr='b';
    MPI_SEC_Send(&ltr, 1, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
    //MPI_Wait(&request, &status);
  } else if (world_rank == 1) {
    MPI_SEC_Recv(&ltr, 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &status);
    //MPI_Wait(&request, &status);
    printf("Process 1 received letter %c from process 0\n",ltr);
    //for(i=0;i<sz;i++)
    // printf("%c",ltr[i]);
    //printf("\n");
  }

    /*ltr = 'd';
  if (world_rank == 0) {
    // If we are rank 0, set the number to -1 and send it to process 1
    ltr = 'e';
    MPI_Send(&ltr, 1, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
  } else if (world_rank == 1) {
    MPI_Recv(&ltr, 1, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    printf("Process 1 received letter %c from process 0\n", ltr);
  }*/		
  MPI_Finalize();
}
