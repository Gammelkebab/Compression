CC=mpicxx
CFLAGS = -g -Wall -Wextra -fopenmp -O3

DEPS = tree.h compress.h decompress.h
OBJ = tree.o compress.o decompress.o main.o
BIN = main

#first target
main : $(OBJ)
	$(CC) $(CFLAGS) -o main $^

#each object depends on the c-file with the same name
%.o : %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c $<

clean :
	rm -rf $(BIN) $(OBJ)
