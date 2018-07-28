CC=mpicxx
CFLAGS = -g -Wall -Wextra -fopenmp -O3

DEPS = tree.h compress.h decompress.h
OBJ = tree.o compress.o decompress.o main.o
BIN = main

#first target
main : $(OBJ)
	$(CC) $(CFLAGS) -o main $^

test : clean main
	mpirun -np 4 ./main
	diff -q lorem_ipsum.txt uncompressed.txt

#each object depends on the c-file with the same name
%.o : %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c $<

clean :
	rm -rf $(BIN) $(OBJ)
	rm -f lorem_ipsum.bin
	rm -f uncompressed.txt
