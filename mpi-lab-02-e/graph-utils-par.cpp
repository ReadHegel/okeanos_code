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

    for (int i = graph->firstRowIdxIncl; i < graph->lastRowIdxExcl; ++i) {
        initializeGraphRow(graph->data[i - graph->firstRowIdxIncl], i, graph->numVertices);
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
    for (int i = 0; i < graph->numVertices; ++i) {

        if (getFirstGraphRowOfProcess(graph->numVertices, numProcesses, owner + 1) <= i) {
            owner++;
        }

        if (myRank == owner) {
            row = graph->data[i - graph->firstRowIdxIncl];
        } else {
            row = buffer;
        }
        MPI_Bcast(row, graph->numVertices, MPI_INT, owner, MPI_COMM_WORLD);
        printGraphRow(row, i, graph->numVertices);
    }
}

void destroyGraph(Graph* graph, int numProcesses, int myRank) {
    freeGraphPart(graph);
}
