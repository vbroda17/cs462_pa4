#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <math.h>

enum { BLACK, RED };

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
    Node ***grid;
    int tmp_temp, tmp_class;
    Node *tmp_node;
    double avg;
    double start, end;
    double convergenceEpsilon = 0.001;
    double maxChange = 0;

    if (argc != 4) exit(1);

    xMax = atoi(argv[1]);
    yMax = atoi(argv[2]);
    steps = atoi(argv[3]);


    grid = (Node***)malloc(yMax * sizeof(Node**));
    for (i = 0; i < yMax; i++) grid[i] = (Node**)malloc(xMax * sizeof(Node*));

    for (i = 0; i < yMax; i++) {
        for (j = 0; j < xMax; j++) {
            grid[i][j] = (Node*)malloc(sizeof(Node));
        }
    }

    for (i = 0; i < yMax; i++) {
        for (j = 0; j < xMax; j++) {
            if (scanf("%d", &tmp_temp) != 1) {
                printf("Invalid input, see read me.\n");
                return 1;
            }

            tmp_node = grid[i][j];
            tmp_node->temp = (double)tmp_temp;

            tmp_node->north = (i > 0) ? grid[i - 1][j] : NULL;
            tmp_node->south = (i < yMax - 1) ? grid[i + 1][j] : NULL;
            tmp_node->east = (j < xMax - 1) ? grid[i][j + 1] : NULL;
            tmp_node->west = (j > 0) ? grid[i][j - 1] : NULL;

            if ((i + j) % 2 == 0) tmp_node->node_class = BLACK;
            else tmp_node->node_class = RED;
        }
    }

    start = MPI_Wtime();

    for (step = 0; step < steps; step++) {
        if (step % 2 == 0) tmp_class = RED;
        else tmp_class = BLACK;

        maxChange = 0;

        // Parallelize the loop that calculates average temperatures
        #pragma omp parallel for private(tmp_node, j, avg) reduction(max:maxChange)
        for (i = 1; i < yMax - 1; i++) {
            for (j = 1; j < xMax - 1; j++) {
                tmp_node = grid[i][j];
                if (tmp_class == tmp_node->node_class) {
                    avg = (tmp_node->north->temp + tmp_node->south->temp + tmp_node->east->temp + tmp_node->west->temp + tmp_node->temp) / 5.0;
                    double change = fabs(avg - tmp_node->temp);
                    if (change > maxChange) maxChange = change;
                    tmp_node->temp = avg;
                }
            }
        }

        if (step % 30 == 0) {
            printf("\nStep: %d\n", step);
            if (xMax < 20) {
                for (i = 0; i < yMax; i++) {
                    for (j = 0; j < xMax; j++) {
                        tmp_node = grid[i][j];
                        printf("%.3lf ", tmp_node->temp);
                    }
                    printf("\n");
                }
            }
            printf("MAX CHANGE: %lf\n", maxChange);
            printf("\n");
        }

        if (maxChange <= convergenceEpsilon) {
            printf("Convergence within %lf achieved in %d steps\n", convergenceEpsilon, step);
            break;
        }
    }
    end = MPI_Wtime();

    printf("Final:\n");
    for (i = 0; i < yMax; i++) {
        for (j = 0; j < xMax; j++) {
            tmp_node = grid[i][j];
            printf("%f ", tmp_node->temp);
        }
        printf("\n");
    }

    printf("TIME %.5f s\n", end - start);

    for (i = 0; i < yMax; i++) {
        for (j = 0; j < xMax; j++) free(grid[i][j]);
        free(grid[i]);
    }
    free(grid);
    return 0;
}
