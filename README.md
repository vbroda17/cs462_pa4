# cs462_pa4
my ocean but in mpi, for cs462 class

compile:
mpicc -o myocean-mpi myocean-mpi.c

to run
su - base
mpirun -np 4 ./myocean-mpi