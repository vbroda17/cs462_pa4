/*
Vincent Broda
cs462
Assignment 4
This is another version of myocean, this time with mpi. See assignment two for more details.
This program was hevily based on my previous version, however due to the structure of message passiging, their might be quite a few changes
*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

enum { BLACK, RED };

typedef struct Node
{
    struct Node* north;
    struct Node* south;
    struct Node* east;
    struct Node* west;
    double temp;
    int node_class;
} Node;

int main(int argc, char* argv[])
{
  int yMax, xMax, steps, step;
  int i, j;
  Node*** grid;
  Node* tmp_node, *tmp_north, *tmp_south, *tmp_east, *tmp_west;
  int tmp_temp, tmp_class;
  double avg;
  double convergenceEpsilon = 0.001;
  double maxChange = 0;
  double globalMaxChange;

  int yStart, xStart, yEnd, xEnd;

  int rank, size;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
    
  if(argc != 4)
  {
    if (rank == 0) printf("Usage: mpirun -np <num_procs> ./myocean-mpi xMax yMax steps\n");
    MPI_Finalize();
    return 1;
  }

  xMax = atoi(argv[1]);
  yMax = atoi(argv[2]);
  steps = atoi(argv[3]);

  // for rank - initialiing the grid 
  if (rank == 0)
  {
    printf("This is rank zero startring to do stuff\n");
    // Allocate memory for the grid and initialize it
    grid = (Node***)malloc(yMax * sizeof(Node**));
    for (i = 0; i < yMax; i++)
    {
      grid[i] = (Node**)malloc(xMax * sizeof(Node*));
      for (j = 0; j < xMax; j++) grid[i][j] = (Node*)malloc(sizeof(Node));
    }
    printf("Allocated grid\n");

    // Initialize grid on process 0
    for (i = 0; i < yMax; i++)
    {
      for (j = 0; j < xMax; j++)
      {
        if (scanf("%d", &tmp_temp) != 1) 
        {
            printf("Invalid input, see read me.\n");
            return 1;
        }
        tmp_node = grid[i][j];
        tmp_node->temp = (double)tmp_temp;

        // Initialize edges, neighbors are null if the node isn't in range. This way worked when a bunch of if statments didn't for some reason
        tmp_node->north = (i > 0) ? grid[i - 1][j] : NULL;
        tmp_node->south = (i < yMax - 1) ? grid[i + 1][j] : NULL;
        tmp_node->east = (j < xMax - 1) ? grid[i][j + 1] : NULL;
        tmp_node->west = (j > 0) ? grid[i][j - 1] : NULL;

        // setting Node as read or black
        if((i + j) % 2 == 0) tmp_node->node_class = BLACK;
        else tmp_node->node_class = RED;
      }
    }
    printf("Initialized grid\n");
  }
  else
  {
    // just allocating memory for other ranks
    printf("Rank %d doing grid allocation\n", rank);
      grid = (Node***)malloc(yMax * sizeof(Node**));
      for (i = 0; i < yMax; i++) {
        grid[i] = (Node**)malloc(xMax * sizeof(Node*));
        for (j = 0; j < xMax; j++) grid[i][j] = (Node*)malloc(sizeof(Node));
    }
  }
  // doing a random sync here, just to be safe, since im not checking this ones preformace.
  MPI_Barrier(MPI_COMM_WORLD);

  // broadcasting the grid from rank 0 to all others
  for (i = 0; i < yMax; i++) {
    for (j = 0; j < xMax; j++) {
      Node* tmp_node = grid[i][j];
      MPI_Bcast(&(tmp_node->temp), 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
      MPI_Bcast(&(tmp_node->node_class), 1, MPI_INT, 0, MPI_COMM_WORLD);
    }
  }
  // I am also going to keep all my dedug print statements here, but I will coment them out
  // MPI_Barrier(MPI_COMM_WORLD);
  // printf("made it to reading lol - From %d of %d\n", rank, size);
  // MPI_Barrier(MPI_COMM_WORLD);

  // dividing up the threads work loads
  int rows_per_rank = yMax / size;

  yStart = rank * rows_per_rank;
  yEnd = (rank == size - 1) ? yMax : (rank + 1) * rows_per_rank;

  xStart = 0;
  xEnd = xMax - 1;
  // MPI_Barrier(MPI_COMM_WORLD);
  // printf("Rank %d    x: %d-%d    y: %d-%d\n", rank, xStart, xEnd, yStart, yEnd);
  // MPI_Barrier(MPI_COMM_WORLD);

  // getting the timer started
  double start, end;
  if (rank == 0) {
    start = MPI_Wtime();
  }

  // This is the part that is actually paralized
  for (step = 0; step < steps; step++)
  {
    // if(rank == 0) printf("\nstep %d\n", step);
    maxChange = 0;
    if(step % 2 == 0) tmp_class = RED;
    else tmp_class = BLACK;
    // Perform the computation on the local portion of the grid
    for(i = yStart; i < yEnd; i++)
    {
      for(j = xStart; j < xEnd; j++)
      {
        // printf("trying at %d %d %d\n", rank, i, j);

        if(i == 0 || i == yMax - 1 || j == 0 || j == xMax) 
        {
          // printf("Continuing at %d %d %d\n", rank, i, j);
          continue;
        }
        //doing caclulation only if right step. I tried to do in smarter ways with some type of index thing but that tended to introduce bugs
        tmp_node = grid[i][j];
        if(tmp_class == tmp_node->node_class)
        {
          tmp_north = grid[i - 1][j];
          tmp_south = grid[i + 1][j];
          tmp_east = grid[i][j + 1];
          tmp_west = grid[i][j - 1];
          avg = tmp_north->temp + tmp_south->temp + tmp_east->temp + tmp_west->temp + tmp_node->temp;
          avg = avg / 5.0;
          // printf("Rank %d changing i,j %d,%d from %lf to %lf\n", rank, i, j, tmp_node->temp, avg);
          // storing max change, if it falls under a certain point then we will end the main loop
          if(fabs(avg - tmp_node->temp) > maxChange) maxChange = fabs(avg - tmp_node->temp);
          tmp_node->temp = avg;

        }
      }
    }

    // MPI_Barrier(MPI_COMM_WORLD);   This is not needed here lol
    MPI_Status status;

    // This is having all the ranks share their data with rank zero. The tag should be unique i belive
    if (rank != 0)
    {
      for (i = yStart; i < yEnd; i++)
      {
        for (j = xStart; j <= xEnd; j++)
        {
          double tmp_value = grid[i][j]->temp;
          // printf("sending data for %d, %d\n", i, j);
          MPI_Send(&tmp_value, 1, MPI_DOUBLE, 0, (j + 1) * (i + 1), MPI_COMM_WORLD);
        }
      }
    } 
    else 
    {
      for (int source_rank = 1; source_rank < size; source_rank++)
      {
        int end = (source_rank == size - 1) ? yMax : (source_rank + 1) * rows_per_rank;
        for (i = source_rank * rows_per_rank; i < end; i++)
        {
          for (j = xStart; j <= xEnd; j++)
          {
            double tmp_value;
            int source_tag = (j + 1) * (i + 1);
            // printf("reciving data for %d, %d\n", i, j);
            MPI_Recv(&tmp_value, 1, MPI_DOUBLE, source_rank, source_tag, MPI_COMM_WORLD, &status);
            grid[i][j]->temp = tmp_value;
          }
        }
      }

      // ocasionally printing tabel
      if(step % 30 == 0)
      {
        printf("Grid %d step-%d\n", rank, step);
        for(i = 0; i < yMax; i++)
        { 
          for(j = 0; j < xMax; j++)
          {
            tmp_node = grid[i][j];
            printf("%f ", tmp_node->temp);
          }
          printf("\n");
        }

      }
    }

    // broadcasting data back out, forgot this for a while and it was causing me a lot more trouble than i want to admit
    for (i = 0; i < yMax; i++) {
      for (j = 0; j < xMax; j++) {
        Node* tmp_node = grid[i][j];
        MPI_Bcast(&(tmp_node->temp), 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
        MPI_Bcast(&(tmp_node->node_class), 1, MPI_INT, 0, MPI_COMM_WORLD);
      }
    }

    //Calculate the global maximum change across all ranks
    MPI_Allreduce(&maxChange, &globalMaxChange, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);
    if (globalMaxChange <= convergenceEpsilon)
    {
      if (rank == 0) printf("Convergence within %lf achieved in %d steps\n", convergenceEpsilon, step);
      break;
    }
  }

  if (rank == 0) 
  {
    end = MPI_Wtime();
    printf("Time taken: %lf seconds\n", end - start);
    // printing grid of each
    printf("Grid %d\n", rank);
    for(i = 0; i < yMax; i++)
    { 
      for(j = 0; j < xMax; j++)
      {
        tmp_node = grid[i][j];
        printf("%f ", tmp_node->temp);
      }
      printf("\n");
    }
  }

  // freeing memory
  for (i = 0; i < yMax; i++)
  {
    for (j = 0; j < xMax; j++) free(grid[i][j]);
    free(grid[i]);
  }
  free(grid);

  MPI_Finalize();
  return 0;
}
