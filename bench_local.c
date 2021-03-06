//  Program  - This code measures the slowdown due to network contention from jobs running
//           near each other on a supercomputer. It is currently configured for BG/Q (Vulcan).
//  The MPI_Alltoall is implemented locally by using Irecv, send and Waitany
//  Created by Neha Jothi
//
//Local implementation of MPI_Alltoall using XOR

//Algorithm - For long messages and power-of-two number of processes
//If rank is Even, then start sending to rank+1, rank+2 ,.., and wrap around till rank-1 (To The Right)
//Else if rank is Odd, then send to rank-1,rank-2,...,and wrap around till rank+1 (To The Left)

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <sys/time.h>
#include <unistd.h>
#include <ctype.h>
#include <getopt.h>

//JOB1 color
#define JOB1 0
//JOB2 color
#define JOB2 1

//Function to parse input parameters
void parseInput(int argc, char** argv, int* msg_size, int* inner_ranks, int* loops, int* run_index){
  char c;
  while ((c = getopt (argc, argv, "s:r:l:i:")) != -1){
    switch (c){
      case 's':
        sscanf(optarg, "%d", msg_size);
        break;
      case 'r':
        sscanf(optarg, "%d", inner_ranks);
        break;
      case 'l':
        sscanf(optarg, "%d", loops);
        break;
      case 'i':
        sscanf(optarg, "%d", run_index);
        break;
      default:
        break;
    }
    if(c != 's' && c != 'i' && c != 'l' && c != 'r' ){
      break;
    }
  }
  return;
}

//Local implementation of MPI_Alltoall using XOR

//Algorithm - For long messages and power-of-two number of processes
//If rank is Even, then start sending to rank+1, rank+2 ,.., and wrap around till rank-1 (To The Right)
//Else if rank is Odd, then send to rank-1,rank-2,...,and wrap around till rank+1 (To The Left)

double Alltoall(MPI_Comm comm, int num_ranks, int split_rank, int rank, int msg_size, int  num_time_steps, int total_ranks, int splitter[]){
    //declare, initialize message space
    char send[msg_size*total_ranks];
    char recv[msg_size*total_ranks];
    //message tag
    int tag = 1;
    //the destination rank
    int to = -1;
    //The rank the message is received from
    int from = -1;
    //MPI request handlers
    MPI_Request request[num_ranks-1];
    //To identify the rank with the corresponding MPI_Request
    int rankRequest[num_ranks];
    //to indentify the rank the message was received from wrt MPI_Waitany
    int index = -1;
    MPI_Status status;
    if( send == NULL || recv == NULL ){
        printf("Error: cannot allocate buffer for all to all in rank %d\n", rank);
    }
    
    //populate the send buffer
    for(int i = 0; i < msg_size*total_ranks; i++){
        send[i] = split_rank;
    }
    
    //perform All-to-all implemented locally, num_time_steps times
    //Start overall A2A time
    double start = MPI_Wtime();
    for(int i = 0; i < num_time_steps; i++){
        //Setting up Irecv
        for( int j = 1; j < num_ranks; j++) {
            //identify the rank to receive from
            from = split_rank ^ j;
            MPI_Irecv(&recv[msg_size*from],msg_size,MPI_CHAR,from,tag,comm,&request[j-1]);
            //record the rank associated with the current request handle
            rankRequest[j-1]=from;
        }
        //Setting up dummy in the array
        int count = 0;
        //Send to every other rank
        for( int j = 1; j < num_ranks; j++) {
            //identify the destination rank
            to = split_rank ^ j;
            MPI_Send(&send[msg_size*to],msg_size,MPI_CHAR,to,tag,comm);
        }
        //setting up Wait
        for(int j = 1; j < num_ranks; j++){
            MPI_Waitany(num_ranks-1,request,&index,&status);
        }

        //Barrier to ensure all ranks are done with this timestep
        MPI_Barrier(comm);
        //Local implementation of MPI_Alltoall using XOR - end
    }
    //End Overall A2A time
    double end = MPI_Wtime();
    return end - start;
}

//main
int main(int argc, char** argv){
  //keep track of the ranks within new the comunicators
  int num_ranks, rank, split_num_ranks, split_rank;
  //Number of ranks in JOB2 nad JOB1 respectively
  int outer_ranks, inner_ranks;
  int msg_size, loops;
  int run_index;
  MPI_Comm split_comm;
  //output file
  FILE * timings;
    
  //Parse input parameters
  parseInput(argc, argv, &msg_size, &inner_ranks, &loops, &run_index);

  //Open timings.out for writing
  timings = fopen("timings.out", "a");
  if(timings == NULL){
    printf("Error: cannot open timings.out\n");
  }

  //Start MPI, get num_ranks
  MPI_Init(NULL, NULL);
  MPI_Comm_size(MPI_COMM_WORLD, &num_ranks);
  if(num_ranks == 0){
    printf("MPI_Comm_size failure\n");
    exit(1);
  }

  //Calculate comm sizes
  outer_ranks = num_ranks - inner_ranks;
  if( (outer_ranks < 0 || inner_ranks < 0) && (rank == 0) ){
    printf("Error: bad comm sizes. They should be positive\n");
  }
    
  //Get global rank
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if(rank == 0)
    printf("\tmsg_size: %d, inner_ranks: %d, loops: %d, run_index: %d\n", msg_size, inner_ranks, loops, run_index);
    
  //Assigning ranks to JOB1 and JOB2 (Rank 0-255 -> JOB1, Rank 56-511 -> JOB2)
  int splitter[num_ranks];
  for(int i = inner_ranks; i < num_ranks; i++)
    splitter[i] = JOB2;
  for(int i = 0; i < inner_ranks; i++)
    splitter[i] = JOB1;

  //split MPI_COMM_WORLD into split_comm representing JOB1 and JOB2
  MPI_Comm_split(MPI_COMM_WORLD, splitter[rank], 1, &split_comm);
  MPI_Comm_size(split_comm, &split_num_ranks);
  MPI_Comm_rank(split_comm, &split_rank);
  MPI_Barrier(MPI_COMM_WORLD);

  //start of benchmark
  //start network counters region 1
  MPI_Pcontrol(1);

  //run the inside alone, as a baseline
  float run1;
  if(splitter[rank] == JOB1){
      run1 = Alltoall(split_comm, split_num_ranks, split_rank, rank, msg_size, loops, num_ranks, splitter);
  }
  MPI_Barrier(MPI_COMM_WORLD);
    
  //start network counters region 2
  MPI_Pcontrol(2);

  //run both communicators
  float run2;
  if(splitter[rank] == JOB1){
      run2 = Alltoall(split_comm, split_num_ranks, split_rank, rank, msg_size, loops, num_ranks, splitter);
  }
  else{
      Alltoall(split_comm, split_num_ranks, split_rank, rank, msg_size, loops, num_ranks, splitter);
  }
  MPI_Barrier(MPI_COMM_WORLD);
  //end of benchmark

  //stop network counters
  MPI_Pcontrol(0);
   
  //print timings to output file
  if(splitter[rank] == JOB1 && split_rank==0) 
    fprintf(timings, "%d,%f,%f\n", run_index, run1, run2);

  MPI_Finalize();
  exit(0);
}

