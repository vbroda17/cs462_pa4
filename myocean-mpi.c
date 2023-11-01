#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

enum {BLACK, RED};

typedef struct Node {
    struct Node* north;
    struct Node* south;
    struct Node* east;
    struct Node* west;
    double temp;
    int node_class;
} Node;

int main(int argc, char **argv) {
    int yMax, xMax, steps, numThreads;
    int i, j, step;
    Node ***grid;  // 2D array that holds pointers to nodes
    Node *tmp_node;
    double convergenceEpsilon = 0.001;
    double maxChange = 0;
    double globalMaxChange = 0;

    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc != 5) {
        if (rank == 0) {
            printf("Usage: mpirun -np <num_procs> ./myocean-mpi xMax yMax steps threads\n");
        }
        MPI_Finalize();
        return 1;
    }

    if (rank == 0) {
        xMax = atoi(argv[1]);
        yMax = atoi(argv[2]);
        steps = atoi(argv[3]);
        numThreads = atoi(argv[4]);
    }

    MPI_Bcast(&xMax, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&yMax, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&steps, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&numThreads, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Determine grid size for each process
    int localYMax = yMax / size;
    int localYStart = rank * localYMax;
    int localYEnd = localYStart + localYMax;

    // Allocate memory for the local grid
    grid = (Node***)malloc(localYMax * sizeof(Node**));
    for (i = 0; i < localYMax; i++) {
        grid[i] = (Node**)malloc(xMax * sizeof(Node*));
    }

    // Initialize local cells and grid
    for (i = 0; i < localYMax; i++) {
        for (j = 0; j < xMax; j++) {
            // Initialize cells and neighbors here
            // You may need to adjust the logic to distribute data among processes
        }
    }

    // Main simulation loop
    for (step = 0; step < steps; step++) {
        // Calculate local maxChange for each process
        maxChange = 0;

        // Perform computation here, including communication between neighbors
        // You need to adapt your original computation loop for MPI

        // Use MPI_Allreduce to find the global maxChange
        MPI_Allreduce(&maxChange, &globalMaxChange, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

        if (rank == 0 && step % 30 == 0) {
            printf("\nStep: %d\n", step);
            // Print grid values here
        }

        if (globalMaxChange <= convergenceEpsilon) {
            if (rank == 0) {
                printf("Convergence within %lf achieved in %d steps\n", convergenceEpsilon, step);
            }
            break;
        }
    }

    // Clean up and finalize MPI
    for (i = 0; i < localYMax; i++) {
        for (j = 0; j < xMax; j++) free(grid[i][j]);
        free(grid[i]);
    }
    free(grid);

    MPI_Finalize();
    return 0;
}
