#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <sys/time.h>
#include <unistd.h>
#include <ctype.h>
#include <getopt.h>

//JOB1
#define INNER_COMM 0
//JOB2
#define OUTER_COMM 1

double Alltoall(MPI_Comm comm, int num_ranks, int rank, int msg_size, int  num_time_steps){

  //declare, initialize message space
  char send[msg_size*num_ranks];
  char recv[msg_size*num_ranks];
  if( send == NULL || recv == NULL ){
    printf("Error: cannot allocate buffer for all to all in rank %d\n", rank);
  }
  for(int i = 0; i < msg_size*num_ranks; i++)
    send[i] = rand() % 256;
  //perform all-to-all 
  double start = MPI_Wtime();
  for(int i = 0; i < num_time_steps; i++){
    MPI_Alltoall(send, msg_size, MPI_CHAR, recv, msg_size, MPI_CHAR, comm);
  }
  double end = MPI_Wtime();
  //printf("rank %d completed!!\n",rank);

  return end - start;
}
int main(int argc, char**argv){

  int num_ranks, rank, split_num_ranks, split_rank;
  int outer_ranks, inner_ranks;
  int new_comm_id;
  int msg_size, loops;
  int slurm_id, run_index;
  MPI_Comm split_comm;
  FILE * timings;

  //Parse options
  char c;
  while ((c = getopt (argc, argv, "s:r:l:i:")) != -1){
    switch (c)
      {
      case 's':
        sscanf(optarg, "%d", &msg_size);
        break;
      case 'r':
        sscanf(optarg, "%d", &inner_ranks);
        break;
      case 'l':
        sscanf(optarg, "%d", &loops);
        break;
      case 'i':
        sscanf(optarg, "%d", &run_index);
        break;
      default:
        //printf("Unrecognized option: %c\n", optopt);
        //printf("Done parsing!\n");
        break;
      }
    if(c != 's' && c != 'i' && c != 'l' && c != 'r' ){break;}
  }

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
  int splitter[num_ranks];
  //Splitting into JOB1 and JOB2
  //int * splitter = (int*)malloc(sizeof(int)*num_ranks);
  for(int i = inner_ranks; i < num_ranks; i++)
    splitter[i] = OUTER_COMM;
  for(int i = 0; i < inner_ranks; i++)
    splitter[i] = INNER_COMM;

  //split communicator
  MPI_Comm_split(MPI_COMM_WORLD, splitter[rank], 1, &split_comm);
  MPI_Comm_size(split_comm, &split_num_ranks);
  MPI_Comm_rank(split_comm, &split_rank);
  MPI_Barrier(MPI_COMM_WORLD);


  //run the inner communicator as a warm-up, seems to reduce variance
  if(splitter[rank] == INNER_COMM){
    Alltoall(split_comm, split_num_ranks, split_rank, msg_size, loops);
  }
  MPI_Barrier(MPI_COMM_WORLD);
  //start network counters region 1
  MPI_Pcontrol(1);

  //run the inside alone, as a baseline
  float run1;
  if(splitter[rank] == INNER_COMM){
    run1 = Alltoall(split_comm, split_num_ranks, split_rank, msg_size, loops);
  }
  MPI_Barrier(MPI_COMM_WORLD);
  //start network counters region 2
  MPI_Pcontrol(2);

  //run both communicators
  float run2;
  if(splitter[rank] == INNER_COMM){
    run2 = Alltoall(split_comm, split_num_ranks, split_rank, msg_size, loops);
  }else{
    Alltoall(split_comm, split_num_ranks, split_rank, msg_size, loops);
  }
  //stop network counters
  MPI_Pcontrol(0);

  //print timings
  if(splitter[rank] == INNER_COMM && split_rank==0) fprintf(timings, "%d,%f,%f\n", run_index, run1, run2);

  //free(recv);
  //free(splitter);
  MPI_Finalize();
  exit(0);
}

