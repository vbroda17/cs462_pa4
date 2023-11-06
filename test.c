#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

enum { BLACK, RED };

typedef struct Node {
    struct Node* north;
    struct Node* south;
    struct Node* east;
    struct Node* west;
    double temp;
    int node_class;
} Node;

int main(int argc, char* argv[]) {
    int yMax, xMax, steps;
    int i, j, step;
    Node*** grid;
    int tmp_temp, tmp_class;
    Node* tmp_node;
    double avg;
    double convergenceEpsilon = 0.001;
    double maxChange = 0;
    double globalMaxChange;

    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc != 4) {
      if (rank == 0) printf("Usage: mpirun -np <num_procs> ./myocean-mpi xMax yMax steps\n");
      MPI_Finalize();
      return 1;
    } else {
      xMax = atoi(argv[1]);
      yMax = atoi(argv[2]);
      steps = atoi(argv[3]);
    }
    // Initialize grid
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
    
  if(rank == 0)
  {
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
    MPI_Bcast(&grid[0][0], yMax * xMax, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  }
  else MPI_Bcast(&grid[0][0], yMax * xMax, MPI_DOUBLE, 0, MPI_COMM_WORLD);

  MPI_Barrier(MPI_COMM_WORLD);

  printf("made it to reading lol - From %d of %d\n", rank, size);

  
    printf("Final:\n");
    for(i = 0; i < yMax; i++)
    { 
        for(j = 0; j < xMax; j++)
        {
            tmp_node = grid[i][j];
            printf("%f ", tmp_node->temp);
        }
        printf("\n");
    }

  //if (rank == 0)
  //{
    for (i = 0; i < yMax; i++) {
      for (j = 0; j < xMax; j++) free(grid[i][j]);
      free(grid[i]);
    }
    free(grid);
  //}

  

  MPI_Finalize();
  return 0;
    // Main computation loop
    for (step = 0; step < steps; step++) {
        maxChange = 0;
        if (step % 2 == 0) tmp_class = RED;
        else tmp_class = BLACK;

        // Distribute data to local grids using MPI_Scatter
        //MPI_Scatter(&grid[rank * localYMax][0], localYMax * localXMax, MPI_DOUBLE, &localGrid[0][0], localYMax * localXMax, MPI_DOUBLE, 0, MPI_COMM_WORLD);

        // Parallel computation of local grids
        for(i = 0; i < yMax - 1; i++)
        {
          for(j = 0; j < xMax - 1; j++)
          {
            tmp_node = grid[i][j];
            if(tmp_class == tmp_node->node_class)
            {
              avg = (tmp_node->north->temp + tmp_node->south->temp + tmp_node->east->temp + tmp_node->west->temp + tmp_node->temp) / 5.0;
              if(fabs(avg - tmp_node->temp) > maxChange) maxChange = fabs(avg - tmp_node->temp);
              tmp_node->temp = avg;
            }
          }
        }
        // Gather local grids back to the master process using MPI_Gather
        //MPI_Gather(&localGrid[0][0], localYMax * localXMax, MPI_DOUBLE, &grid[0][0], localYMax * localXMax, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        // Synchronize processes using MPI_Barrier
        MPI_Barrier(MPI_COMM_WORLD);

        // Calculate global maxChange using MPI_Allreduce
        MPI_Allreduce(&maxChange, &globalMaxChange, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
        maxChange = globalMaxChange;

        if (rank == 0) {
          // printing every 30 steps
          if(step % 30 == 0)
          //if(0 == 0) // this is if i want to check every step
          {
              //printf("\nStep: %d\n", step);
              if(xMax < 20)       // so terminal isnt flooded on bigger inputs, will only print out updates us grid on smaller sizes
              {
                  for(i = 0; i < yMax; i++)
                  {   // temp
                      for(j = 0; j < xMax; j++)
                      {
                          tmp_node = grid[i][j];
                          printf("%.3lf ", tmp_node->temp);
                      }
                      printf("\n");
                  }
              }
              // will always update the maxChange, so you have one way of knowing
              printf("MAX CHANGE: %lf\n", maxChange);
              printf("\n");
          }

          // exit check for convergene
          if(maxChange <= convergenceEpsilon)
          {
              printf("Convergence within %lf achived in %d steps\n", convergenceEpsilon, step);
              break;
          }

        }
    }

    // Deallocate memory and finalize MPI

    printf("Final:\n");
    for(i = 0; i < yMax; i++)
    { 
        for(j = 0; j < xMax; j++)
        {
            tmp_node = grid[i][j];
            printf("%f ", tmp_node->temp);
        }
        printf("\n");
    }

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
