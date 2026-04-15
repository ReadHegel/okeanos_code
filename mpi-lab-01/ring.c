/*
 * A template for the 2016 MPI lab at the University of Warsaw.
 * Copyright (C) 2016, Konrad Iwanicki
 * Further modifications by Krzysztof Rzadca 2018
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  MPI_Init(&argc, &argv);

  int numProcesses, myRank;
  MPI_Comm_size(MPI_COMM_WORLD, &numProcesses);
  MPI_Comm_rank(MPI_COMM_WORLD, &myRank);

  int64_t message = 1;

  if (myRank == 0) {
    double startTime;

    double endTime;
    double executionTime;
    startTime = MPI_Wtime();

    MPI_Send(&message, 1, MPI_INT64_T, (myRank + 1) % numProcesses, 0,
             MPI_COMM_WORLD); /* pointer to the message */
    MPI_Recv(&message, 1, MPI_INT64_T, (myRank - 1 + numProcesses) % numProcesses,
             0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    endTime = MPI_Wtime();
    executionTime = endTime - startTime;

    printf("Hello from 0 operation result: %ld\n", message);
    printf("Execution time: %f seconds\n", executionTime);

  } else {
    MPI_Recv(&message, 1, MPI_INT64_T, (myRank - 1 + numProcesses) % numProcesses,
             0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    message *= myRank;

    MPI_Send(&message, 1, MPI_INT64_T, (myRank + 1) % numProcesses, 0,
             MPI_COMM_WORLD); /* pointer to the message */
  }

  MPI_Finalize(); /* mark that we've finished communicating */

  return 0;
}
