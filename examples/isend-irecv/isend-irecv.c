#include <mpi.h>
#include <stdio.h>

int main(int argc, char** argv) {
        int rank;
        float buf[100];
        float recv_buf[100];
        const int root=0;
        int i;
       // int request, status;

        MPI_Init(&argc, &argv);
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Request request;
        MPI_Status  status;
       
        for(i=0;i<20;i++)
            recv_buf[i]=rank*1.0;

        printf("[%d]: Before isend, %f %f %f %f %f %f %f %f %f %f\n", rank, 
                recv_buf[0],recv_buf[1],recv_buf[2],recv_buf[3],recv_buf[4],recv_buf[5],recv_buf[6],recv_buf[7],recv_buf[8],recv_buf[9]);
        
       
        MPI_Barrier(MPI_COMM_WORLD);

        if(rank == root) {
           for(i=0;i<5;i++)
            buf[i]= -51.0;

            for(i=5;i<10;i++)
                buf[i]= 21.2222;
        }

        

        init_crypto();  
        /* everyone calls bcast, data is taken from root and ends up in everyone's buf */
        if(rank == 0){
            MPI_SEC_Isend(buf, 10, MPI_FLOAT, 1, 0, MPI_COMM_WORLD, &request);
            MPI_Wait(&request, &status);
       } else if (rank == 1) {
         MPI_SEC_Irecv(recv_buf, 10, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, &request);
         MPI_Wait(&request, &status);
         printf("[%d]: After recv, %f %f %f %f %f %f %f %f %f %f\n", rank, 
                recv_buf[0],recv_buf[1],recv_buf[2],recv_buf[3],recv_buf[4],recv_buf[5],recv_buf[6],recv_buf[7],recv_buf[8],recv_buf[9]);
       }

        MPI_Finalize();
        return 0;
}