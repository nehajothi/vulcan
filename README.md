# MPI Contention Benchmark
Author: Neha Jothi

## Purpose
This code measures the slowdown due to network contention from jobs running near each other on a supercomputer. It is currently configured for BG/Q (Vulcan).


## Method
Two communicators are created and assigned to some ranks in your job. A single communicator is run to get a baseline for performance. Then both are run to determine the slowdown. The main communication is All-to-All, run a user-specified number of times. 

## Example
Look in `moab-vulcan.sh` for an example of how to run it. Check the Files section below for the meaning of the output.
A mapping file for the rank placement should be specified. This should be somewhere globally-writable, such as tmp2. 

## Map Files
A collection of map files is included in the `maps` directory. They are in a format that can be supplied to the included moab command. 
Format dimensions of - A B C D E core
Dimensions for BG/Q - 4 * 4 * 4 * 4 * 2

## Options
-s message size (in bytes) 
-l specifies number of timesteps (iterations)
-r specifies number of ranks in inner communicator
-i index of the job (will be used to identify different runs in the output file)                                                                                                                            

## Compile
`$make vulcan` 
The bgqncl folder (required for network counters) is available at - https://github.com/LLNL/bgqncl.git


##Running
msub moab-vulcan.sh

## Files:
bench.c:     contains the benchmark

## Output
-net counters from each run will be placed in net/
-timing info for each run is in timings.out (Output format is {index,Time taken by Job1 without contention, Time taken by Job1 with contention})
-squeue-*.out contains info on what other jobs are running at the time you start the job
-slurm-*.out has the stdio output
	

