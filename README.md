# MPI Contention Benchmark
Author: Neha Jothi

## Purpose
This code measures the slowdown due to network contention from job running near each other on large clusters. It is currently configured for a BG/Q cluster, Vulcan.


## Method
Two communicators are created and assigned to some ranks in your job. A single communicator is run, to get a baseline for performance. Then both are run to determine the slowdown. This program does not currently have a comm phase. Instead, an All-to-All is run a user-specified number of times. 

## Example
Look in `moab-vulcan.sh` for an example of how to run it. Check the Files section below for the meaning of the output.
A mapping file for the rank placement should be specified. This should be somehwere globally-writable, such as tmp2. 

## Map Files
A collection of map files is included in the `maps` directory. They are in a format that can be supplied to the included moab command. 
Format dimensions of - A B C D E core
Dimensions for BG/Q - 4 * 4 * 4 * 4 * 2

## Options
-s messsage size (in bytes) 
-l specifies number of all to all loops
-r specifies number of ranks in inner commnicator
-i index of the job (will be used for names of output)                                                                                                                            

## Compile
`$make vulcan` 
The bgqncl folder is available at (required for network counters) - https://github.com/LLNL/bgqncl.git


##Running
msub moab-vulcan.sh

## Output
-net counters from each run will be placed in net/
-timing info for each run is in timings.out (Output format is {index,Time taken by Job1 without contention, Time  taken by Job2 with contention)
-squeue-*.out contains info on what other jobs are running at the time you start the job
-slurm-*.out has the stdio output
	

## Files:
bench.c:     contains the benchmark
