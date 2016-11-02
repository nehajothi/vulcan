#!/bin/bash

#MSUB -l partition=vulcan
#MSUB -l nodes=512
#MSUB -l walltime=12:00:00
#MSUB -l gres=ignore
#MSUB -q psmall
#MSUB -j oe
#MSUB -N bench
### To specify the bank to be used--- MSUB -A asccasc
### To explicitly ensure deterministic routing--- MSUB -v PAMI_M2M_ROUTING="DETERMINISTIC"
### To disable MPI collective optimization--- MSUB -v PAMID_COLLECTIVES=0

## -s specifies messsage size, 
## -l specifies number of all to all loops,
## -r specifies number of ranks in inner communicator
## -i index of the job (will be used for names of output)

# This is a simplified version of the moab script. A typical run of the benchmark
# is on 512 nodes, with 256 in the comm1 (-r 256). (comm2 gets  num_ranks - [value of r]).

# Additionally you should use srun with a mapfile (commented out below). 

#loctaion to save your network counter files
mkdir net
squeue >> squeue-$SLURM_JOB_ID.out
((count=0))

for idx in `seq 1 2`
do
  for msg in 16384 #2048 8192 16384 32768 #4096 6144 8192 16384 32768 65536
  do
    for map in `seq 0 31`
    do
      #export BGQ_COUNTER_FILE=net/net-A2A-xor2-$map-$idx.out
      #If you want the program to use customized mapping - the job can only access it from the shared file system
      cp /nfs/tmp2/nehajothi/maps/comm-$map /nfs/tmp2/nehajothi/map
      srun -N512 --runjob-opts='--mapping /nfs/tmp2/nehajothi/map' ./bench -s $msg -l 1000 -r 256 -i $count
      #If you want the program to use default mapping  
      #srun -N512 ./bench -s $msg -l 1000 -r 256 -i $count 
      ((count=count+1))
    done
    ((count=100))
  done
  ((count=0))
done

# echo "email body" | mailx -s "Subject of email - Job Completed" <user@email.com>

