/*
 * A template for the 2019 MPI lab at the University of Warsaw.
 * Copyright (C) 2016, Konrad Iwanicki.
 * Refactoring 2019, Łukasz Rączkowski
 */

#include <cassert>
#include <mpi.h>
#include <iostream>
#include "graph-base.h"
#include "graph-utils.h"

int getFirstGraphRowOfProcess(int numVertices, int numProcesses, int myRank) {
    /* FIXME: implement */
    int rowsPerProcess = numVertices / numProcesses;
    int remainder = numVertices % numProcesses;
    int firstRow = myRank * rowsPerProcess + (myRank < remainder ? myRank : remainder);
    return firstRow;
}

Graph* createAndDistributeGraph(int numVertices, int numProcesses, int myRank) {
    assert(numProcesses >= 1 && myRank >= 0 && myRank < numProcesses);

    auto graph = allocateGraphPart(
            numVertices,
            getFirstGraphRowOfProcess(numVertices, numProcesses, myRank),
            getFirstGraphRowOfProcess(numVertices, numProcesses, myRank + 1)
    );

    if (graph == nullptr) {
        return nullptr;
    }

    assert(graph->numVertices > 0 && graph->numVertices == numVertices);
    assert(graph->firstRowIdxIncl >= 0 && graph->lastRowIdxExcl <= graph->numVertices);

    /* FIXME: implement */

    if (myRank == 0) { 
        int owner = 0;
        for (int k = 0; k < graph->numVertices; ++k) {
            int* buffer = new int[graph->numVertices];

            if (getFirstGraphRowOfProcess(graph->numVertices, numProcesses, owner + 1) <= k) {
                owner++;
            }

            if (owner != 0) {
                initializeGraphRow(buffer, k, graph->numVertices);
                MPI_Send(buffer, graph->numVertices, MPI_INT, owner, 0, MPI_COMM_WORLD);
            }
            else {
                initializeGraphRow(graph->data[k - graph->firstRowIdxIncl], k, graph->numVertices);
            }
        }
    }
    else {
        for (int i = graph->firstRowIdxIncl; i < graph->lastRowIdxExcl; ++i) {
            MPI_Recv(graph->data[i - graph->firstRowIdxIncl], graph->numVertices, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        } 
    }

    return graph;
}

void collectAndPrintGraph(Graph* graph, int numProcesses, int myRank) {
    assert(numProcesses >= 1 && myRank >= 0 && myRank < numProcesses);
    assert(graph->numVertices > 0);
    assert(graph->firstRowIdxIncl >= 0 && graph->lastRowIdxExcl <= graph->numVertices);

    /* FIXME: implement */
    int* buffer = new int[graph->numVertices];
    int* row = NULL;
    int owner = 0;
    int currentRow = 0;

    if (myRank == 0) {
        for (int k = 0; k < graph->numVertices; ++k) {
            if (getFirstGraphRowOfProcess(graph->numVertices, numProcesses, owner + 1) <= k) {
                owner++;
            }

            if (owner != 0) {
                MPI_Recv(buffer, graph->numVertices, MPI_INT, owner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                printGraphRow(buffer, k, graph->numVertices);
            }
            else {
                printGraphRow(graph->data[k - graph->firstRowIdxIncl], k, graph->numVertices);
            }
        }
    } else {
        for (int i = graph->firstRowIdxIncl; i < graph->lastRowIdxExcl; ++i) {
            MPI_Send(graph->data[i - graph->firstRowIdxIncl], graph->numVertices, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }
    }
}

void destroyGraph(Graph* graph, int numProcesses, int myRank) {
    freeGraphPart(graph);
}
