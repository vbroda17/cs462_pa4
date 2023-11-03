// Vincent Broda
// CS462 Assignment 4
// This program is another version of myocean, this time done with mpi. Because of this, I will be using my previous code as a heavy reference

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

enum {BLACK, RED};

typedef struct Node
{
    struct Node* north;
    struct Node* south;
    struct Node* east;
    struct Node* west;
    double temp;
    int node_class;
} Node;


int main(int argc, char *argv[])
{
  // int rank;
  // int size;
  // MPI_Init(&argc, &argv);
  // MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  // MPI_Comm_size(MPI_COMM_WORLD, &size);
  // printf("Hello World! From %d of %d\n", rank, size);
  // MPI_Finalize();
  // return 0;

  int yMax, xMax, steps, numThreads;
  int i, j, step;
  Node ***grid;   // Basically a 2d array that holds pointers to nodes
  int tmp_temp, tmp_class;
  Node *tmp_node;
  double avg;
  double start, end; 
  double convergenceEpsilon = 0.001;  // picked kinda arbitrarily, I feel like this is a small number but doesn't take to long. I also changed this often 
  double maxChange = 0;

  int rank, size;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if (argc != 5) {
    if (rank == 0) printf("Usage: mpirun -np <num_procs> ./myocean-mpi xMax yMax steps threads\n");
    MPI_Finalize();
    return 1; // This will aborte the rest of the jobs since it is a non zero return value
  }

  if(rank == 0)
  {
    xMax = atoi(argv[1]);
    yMax = atoi(argv[2]);
    steps = atoi(argv[3]);
    numThreads = atoi(argv[4]);

    grid = (Node***)malloc(yMax * sizeof(Node**));
    for(i = 0; i < yMax; i++) grid[i] = (Node**)malloc(xMax * sizeof(Node*));
    for(i = 0; i < yMax; i++)
    {
        for(j = 0; j < xMax; j++)
        {
            // allocate individual node
            grid[i][j] = (Node*)malloc(sizeof(Node));
        }
    }

    // making initializing cells and grid
    for(i = 0; i < yMax; i++)
    {
        for(j = 0; j < xMax; j++)
        {
            // get its starting temp, error check here to
            if (scanf("%d", &tmp_temp) != 1) 
            {
                printf("Invalid input, see read me.\n");
                return 1;
            }

            tmp_node = grid[i][j];
            tmp_node->temp = (double)tmp_temp;

            // Initialize edges, neighbors are null if the node isn't in range
            tmp_node->north = (i > 0) ? grid[i - 1][j] : NULL;
            tmp_node->south = (i < yMax - 1) ? grid[i + 1][j] : NULL;
            tmp_node->east = (j < xMax - 1) ? grid[i][j + 1] : NULL;
            tmp_node->west = (j > 0) ? grid[i][j - 1] : NULL;

            // setting Node as read or black. making assumption of size being 2^n + 2
            if((i + j) % 2 == 0) tmp_node->node_class = BLACK;
            else tmp_node->node_class = RED;
        }
    }
  }

  MPI_Barrier(MPI_COMM_WORLD);

  printf("made it to reading lol - From %d of %d\n", rank, size);



  if (rank == 0)
  {
    for (i = 0; i < yMax; i++) {
      for (j = 0; j < xMax; j++) free(grid[i][j]);
      free(grid[i]);
    }
    free(grid);
  }
  MPI_Finalize();
  return 0;
}