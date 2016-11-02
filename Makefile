BGPM = /bgsys/drivers/ppcfloor/bgpm

vulcan:
        mpixlc -c -std=c99 bench.c
        mpixlc -o bench bench.o bgqncl/libprofiler.a -L $(BGPM)/lib -lbgpm -lrt -lstdc++
