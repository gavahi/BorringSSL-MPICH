/*
This program will send 10
int, double and char to
process 1, from process 0.

*/
#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#define sz 1000

int main(int argc, char** argv) {
  
  MPI_Init(NULL, NULL);
  
  int world_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

  int world_size;
  MPI_Request request;
  MPI_Status status;
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);

  // We are assuming at least 2 processes for this task
  //if (world_size < 2) {
    //fprintf(stderr, "World size must be greater than 1 for %s\n", argv[0]);
    //MPI_Abort(MPI_COMM_WORLD, 1);
  //}
 
  int send_buf[sz];
  int recv_buf[sz];
  int i,j;

  if(world_rank == 0){
    printf("int value test\n");
     for(i=0;i<5;i++)
       send_buf[i]=105;

     for(i=5;i<10;i++)
       send_buf[i]= -99;  
  }
  
  init_crypto();
  
  if (world_rank == 0) {   
    MPI_SEC_Send(send_buf, 10, MPI_INT, 1, 0, MPI_COMM_WORLD);
    //MPI_Wait(&request, &status);
  } else if (world_rank == 1) {
    MPI_SEC_Recv(recv_buf, 10, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
    //MPI_Wait(&request, &status);
    printf("Process 1 received: %d %d %d %d %d %d %d %d %d %d\n",
    recv_buf[0], recv_buf[1], recv_buf[2], recv_buf[3], recv_buf[4], recv_buf[5],
    recv_buf[6], recv_buf[7], recv_buf[8], recv_buf[9]);
  }

  MPI_Barrier(MPI_COMM_WORLD);

  double d_send_buf[sz];
  double d_recv_buf[sz];

  if(world_rank == 0){
     printf("double value test\n"); 
     for(i=0;i<5;i++)
       d_send_buf[i]=2.33;

     for(i=5;i<10;i++)
       d_send_buf[i]= -4.56;  
  }

  if (world_rank == 0) {   
    MPI_SEC_Send(d_send_buf, 10, MPI_DOUBLE, 1, 0, MPI_COMM_WORLD);
    //MPI_Wait(&request, &status);
  } else if (world_rank == 1) {
    MPI_SEC_Recv(d_recv_buf, 10, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, &status);
    //MPI_Wait(&request, &status);
    printf("Process 1 received: %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf\n",
    d_recv_buf[0], d_recv_buf[1], d_recv_buf[2], d_recv_buf[3], d_recv_buf[4], d_recv_buf[5],
    d_recv_buf[6], d_recv_buf[7], d_recv_buf[8], d_recv_buf[9]);
  }


  MPI_Barrier(MPI_COMM_WORLD);

  char c_send_buf[sz];
  char c_recv_buf[sz];

  if(world_rank == 0){
     printf("char value test\n"); 
     for(i=0;i<5;i++)
       c_send_buf[i]='a';

     for(i=5;i<10;i++)
       c_send_buf[i]= 'A';  
  }

  if (world_rank == 0) {   
    MPI_SEC_Send(c_send_buf, 10, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
    //MPI_Wait(&request, &status);
  } else if (world_rank == 1) {
    MPI_SEC_Recv(c_recv_buf, 10, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &status);
    //MPI_Wait(&request, &status);
    printf("Process 1 received: %c %c %c %c %c %c %c %c %c %c\n",
    c_recv_buf[0], c_recv_buf[1], c_recv_buf[2], c_recv_buf[3], c_recv_buf[4], c_recv_buf[5],
    c_recv_buf[6], c_recv_buf[7], c_recv_buf[8], c_recv_buf[9]);
  }
  	
  MPI_Finalize();
}
