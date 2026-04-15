/*
 * MPI ping-pong benchmark for throughput and latency.
 */

#include <mpi.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_EXPERIMENTS 30
#define DEFAULT_DISCARD_EACH_SIDE 2
#define THROUGHPUT_SIZE_BYTES (1)

static int compare_doubles(const void *a, const void *b) {
  double da = *(const double *)a;
  double db = *(const double *)b;
  if (da < db) {
    return -1;
  }
  if (da > db) {
    return 1;
  }
  return 0;
}

static double trimmed_mean(double *values, int n, int discardEachSide) {
  qsort(values, (size_t)n, sizeof(double), compare_doubles);

  int maxDiscard = (n - 1) / 2;
  if (discardEachSide > maxDiscard) {
    discardEachSide = maxDiscard;
  }

  int from = discardEachSide;
  int to = n - discardEachSide;
  double sum = 0.0;
  int count = 0;

  for (int i = from; i < to; ++i) {
    sum += values[i];
    ++count;
  }

  return (count > 0) ? (sum / (double)count) : 0.0;
}

static void run_ping_pong(int rank, int experiments, int messageSize,
                          double *roundTripTimesSec) {
  char *buffer = (char *)malloc((size_t)messageSize);
  if (buffer == NULL) {
    fprintf(stderr, "Allocation failed for messageSize=%d\n", messageSize);
    MPI_Abort(MPI_COMM_WORLD, 1);
  }
  memset(buffer, 0xAB, (size_t)messageSize);

  if (rank == 0) {
    for (int i = 0; i < experiments; ++i) {
      double startTime = MPI_Wtime();

      MPI_Send(buffer, messageSize, MPI_BYTE, 1, 0, MPI_COMM_WORLD);
      MPI_Recv(buffer, messageSize, MPI_BYTE, 1, 0, MPI_COMM_WORLD,
               MPI_STATUS_IGNORE);

      double endTime = MPI_Wtime();
      roundTripTimesSec[i] = endTime - startTime;

      printf("size=%dB trial=%d round-trip=%.6f ms\n", messageSize, i + 1,
             roundTripTimesSec[i] * 1000.0);

        MPI_Barrier(MPI_COMM_WORLD);
    }
  } else if (rank == 1) {
    for (int i = 0; i < experiments; ++i) {
      MPI_Recv(buffer, messageSize, MPI_BYTE, 0, 0, MPI_COMM_WORLD,
               MPI_STATUS_IGNORE);
      MPI_Send(buffer, messageSize, MPI_BYTE, 0, 0, MPI_COMM_WORLD);

        MPI_Barrier(MPI_COMM_WORLD);
    }
  }

  free(buffer);
}

int main(int argc, char *argv[]) {
  MPI_Init(&argc, &argv);

  int numProcesses = 0;
  int myRank = -1;
  MPI_Comm_size(MPI_COMM_WORLD, &numProcesses);
  MPI_Comm_rank(MPI_COMM_WORLD, &myRank);

  int experiments = DEFAULT_EXPERIMENTS;
  if (argc >= 2) {
    experiments = atoi(argv[1]);
    if (experiments <= 4) {
      if (myRank == 0) {
        fprintf(stderr, "Experiments must be > 4 to allow trimming.\n");
      }
      MPI_Finalize();
      return 1;
    }
  }

  if (numProcesses < 2) {
    if (myRank == 0) {
      fprintf(stderr, "Need at least 2 MPI processes.\n");
    }
    MPI_Finalize();
    return 1;
  }

  const int discardEachSide = DEFAULT_DISCARD_EACH_SIDE;

  if (myRank == 0 || myRank == 1) {
    double *times = NULL;
    if (myRank == 0) {
      times = (double *)malloc((size_t)experiments * sizeof(double));
      if (times == NULL) {
        fprintf(stderr, "Allocation failed for times array\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
      }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    run_ping_pong(myRank, experiments, THROUGHPUT_SIZE_BYTES, times);

    if (myRank == 0) {
      double avgRoundTripSec =
          trimmed_mean(times, experiments, discardEachSide);
      double throughputMBps =
          (2.0 * (double)THROUGHPUT_SIZE_BYTES) / (avgRoundTripSec * 1024.0 * 1024.0);
      double latencyMs = (avgRoundTripSec * 1000.0) / 2.0;

      printf("\n=== Throughput benchmark ===\n");
      printf("message size: %d B\n", THROUGHPUT_SIZE_BYTES);
      printf("experiments: %d, discarded min/max each side: %d\n", experiments,
             discardEachSide);
      printf("avg round-trip time: %.6f ms\n", avgRoundTripSec * 1000.0);
      printf("estimated throughput: %.2f MB/s\n", throughputMBps);
      printf("estimated one-way latency from large msg: %.6f ms\n\n", latencyMs);
    }

    const int latencySizes[] = {1, 10, 100};
    const int latencyCount = (int)(sizeof(latencySizes) / sizeof(latencySizes[0]));

    for (int i = 0; i < latencyCount; ++i) {
      MPI_Barrier(MPI_COMM_WORLD);

      run_ping_pong(myRank, experiments, latencySizes[i], times);

      if (myRank == 0) {
        double avgRoundTripSec =
            trimmed_mean(times, experiments, discardEachSide);
        double latencyMs = (avgRoundTripSec * 1000.0) / 2.0;

        printf("=== Latency benchmark ===\n");
        printf("message size: %d B\n", latencySizes[i]);
        printf("experiments: %d, discarded min/max each side: %d\n", experiments,
               discardEachSide);
        printf("avg round-trip time: %.6f ms\n", avgRoundTripSec * 1000.0);
        printf("estimated one-way latency: %.6f ms\n\n", latencyMs);
      }
    }

    if (myRank == 0) {
      free(times);
    }
  }

  MPI_Finalize();
  return 0;
}
