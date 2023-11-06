CC = mpicc
CFLAGS = -O3 -Wall
SRC = myocean-mpi.c
OUTPUT = myocean-mpi

all: $(OUTPUT)

$(OUTPUT): $(SRC)
	$(CC) $(CFLAGS) -o $(OUTPUT) $(SRC)

clean:
	rm -f $(OUTPUT)
