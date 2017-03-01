#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <sys/time.h>
#include <unistd.h>
#include <ctype.h>
#include <getopt.h>

#define JOB1 0
#define JOB2 1
#define NUM_NEIGHBORS 4

double nearestNeighbor(MPI_Comm comm, int num_ranks, int rank, int msg_size, int  num_time_steps, int xi, int y){
  int tag =1;
  int total_ranks = 0;
  //declare, initialize message space
  //Each rank has 4 neighbors
  char send[msg_size*NUM_NEIGHBORS];
  char recv[msg_size*NUM_NEIGHBORS];
  //MPI request handlers
  MPI_Request request[NUM_NEIGHBORS];
  //To identify the rank with the corresponding MPI_Request
  //int rankRequest[num_ranks];
  //to indentify the rank the message was received from wrt MPI_Waitany
  int index = -1;
  MPI_Status status;
  int neighbors[NUM_NEIGHBORS] = {0};
  int x=0;
  x = xi;
  //Find_Neighbors
  for(int i = 0; i < msg_size*NUM_NEIGHBORS; i++)
     //send[i] = (char) rank; 
     send[i] = rank;
  //to find the neighbors of rank
  //Each job has 256 ranks ( i.e. 2D 16x16 (XxY) in application space)

  total_ranks = x * y;

  //if the ranks are along the left boundary, then have to include wrap around for neighbor[1]
  if( rank%x ==0 ) {
    neighbors[0] = (rank+1) % (x*(1+rank/x));
    neighbors[1] = (rank + (x-1)) % (x*(1+rank/x));
  }
  //if the ranks are along the right boundary, then have to include wrap around for neighbor[0]
  else if((rank+1)%x==0) {
    neighbors[0] = (rank-(x-1)) % (x*(1+rank/x));
    neighbors[1] = (rank-1) % (x*(1+rank/x));
   }
  //the remaining ranks are interior
  else {
    neighbors[0] = (rank+1) % (x*(1+rank/x));
    neighbors[1] = (rank-1 + (x*(1+rank/x))) % (x*(1+rank/x));
  }
  neighbors[2] = (rank+y) % (total_ranks);
  //printf(" rank= %d; x = %d; y= %d ; total_ranks = %d;\n",rank,x,y, total_ranks);
  neighbors[3] = (rank-y + total_ranks) % (total_ranks);
  //printf("Neighbors of %d are %d, %d, %d and %d \n", rank, neighbors[0], neighbors[1], neighbors[2], neighbors[3]);
  //End Find_Neighbors

  //send to nearest neighbors
  double start = MPI_Wtime();
  for(int i = 0; i < num_time_steps; i++) {
    for(int j = 0; j < NUM_NEIGHBORS ; j++) {
      MPI_Irecv(&recv[msg_size*j],msg_size,MPI_CHAR,neighbors[j],tag,comm,&request[j]);
    }
    for(int j = 0; j < NUM_NEIGHBORS ; j++) {
      //printf("Rank %d sending message %d to rank %d\n",rank,send[msg_size*j],neighbors[j]);
      MPI_Send(&send[msg_size*j],1,MPI_INT,neighbors[j],tag,comm);
      //MPI_Isend(&send[msg_size*j],msg_size,MPI_CHAR,neighbors[j],tag,comm,&request);
      //MPI_Wait(&request,&status);
    }
    //setting up Wait
    for(int j = 0; j < NUM_NEIGHBORS; j++){
      MPI_Waitany(NUM_NEIGHBORS,request,&index,&status);
    }
    //receive from nearest neighbors
    //for(int j = 0; j < NUM_NEIGHBORS ; j++) { 
      //MPI_Wait(&request2,&status);
      //printf("Rank %d received message %d from rank %d\n",rank,recv[msg_size*j],neighbors[j]);
    //}
  }
  double end = MPI_Wtime();
  return end - start;
}


int main(int argc, char**argv){

  int num_ranks, rank, split_num_ranks, split_rank;
  int outer_ranks, inner_ranks;
  int new_comm_id;
  int msg_size, loops, app_dim_x, app_dim_y;
  int slurm_id, run_index;
  MPI_Comm split_comm;
  FILE * timings;

  //Parse options
  char c;
  while ((c = getopt (argc, argv, "s:r:l:i:x:y:")) != -1){
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
      case 'x':
        sscanf(optarg, "%d", &app_dim_x);
        break;
      case 'y':
        sscanf(optarg, "%d", &app_dim_y);
        break;
      default:
        //printf("Unrecognized option: %c\n", optopt);
        break;
      }
    if(c != 's' && c != 'i' && c != 'l' && c != 'r' && c != 'x' && c != 'y')
      {break;}
  }
  //printf("Successfully parsed options as: \n");
  //printf("\tmsg_size: %d, inner_ranks: %d, loops: %d, run_index: %d, app_dim_x: %d, app_dim_y: %d\n", msg_size, inner_ranks, loops, run_index, app_dim_x, app_dim_y);

  //Open timings.out for writing
  timings = fopen("timings_NN.out", "a");
  if(timings == NULL){
    printf("Error: cannot open timings_NN.out\n");
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
  if(rank==0)
    printf("\tmsg_size: %d, inner_ranks: %d, loops: %d, run_index: %d, app_dim_x: %d, app_dim_y: %d\n", msg_size, inner_ranks, loops, run_index, app_dim_x, app_dim_y);

  int * splitter = (int*)malloc(sizeof(int)*num_ranks);
  for(int i = inner_ranks; i < num_ranks; i++)
    splitter[i] = JOB2;
  for(int i = 0; i < inner_ranks; i++)
    splitter[i] = JOB1;

  //split communicator
  MPI_Comm_split(MPI_COMM_WORLD, splitter[rank], 1, &split_comm);
  MPI_Comm_size(split_comm, &split_num_ranks);
  MPI_Comm_rank(split_comm, &split_rank);
  MPI_Barrier(MPI_COMM_WORLD);


  //run the inner communicator as a warm-up, seems to reduce variance
  if(splitter[rank] == JOB1){
    //Alltoall(split_comm, split_num_ranks, split_rank, msg_size, loops);
    nearestNeighbor(split_comm, split_num_ranks, split_rank, msg_size, loops, app_dim_x, app_dim_y);
  }
  MPI_Barrier(MPI_COMM_WORLD);

  //start network counters region 1
  MPI_Pcontrol(1);

  //run the inside alone, as a baseline
  float run1;
  if(splitter[rank] == JOB1){
    //run1 = Alltoall(split_comm, split_num_ranks, split_rank, msg_size, loops);
    run1 = nearestNeighbor(split_comm, split_num_ranks, split_rank, msg_size, loops, app_dim_x, app_dim_y);
  }
  MPI_Barrier(MPI_COMM_WORLD);

  //start network counters region 2
  MPI_Pcontrol(2);

  //run both communicators
  float run2;
  if(splitter[rank] == JOB1){
    //run2 = Alltoall(split_comm, split_num_ranks, split_rank, msg_size, loops); 
    run2 = nearestNeighbor(split_comm, split_num_ranks, split_rank, msg_size, loops, app_dim_x, app_dim_y);
  }else{
    //Alltoall(split_comm, split_num_ranks, split_rank, msg_size, loops);
    nearestNeighbor(split_comm, split_num_ranks, split_rank, msg_size, loops, app_dim_x, app_dim_y);
  }

  //stop network counters
  MPI_Pcontrol(0);

  //print timings
  if(splitter[rank] == JOB1 && split_rank==0) fprintf(timings, "%d,%f,%f\n", run_index, run1, run2);

  //free(recv);
  free(splitter);
  MPI_Finalize();
  exit(0);
}



