FLAGS=-std=c++17 -O3 -march=native -pedantic -Wall

all: reorder

reorder: reorder.cpp rng.o fast_median.o distance_matrix.o
	g++ $(FLAGS) -I. -o reorder reorder.cpp rng.o fast_median.o distance_matrix.o

rng.o: rng.cpp rng.h
	g++ $(FLAGS) -I. -c rng.cpp

fast_median.o: fast_median.cpp fast_median.h
	g++ $(FLAGS) -I. -c fast_median.cpp

distance_matrix.o: distance_matrix.cpp distance_matrix.h
	g++ $(FLAGS) -I. -c distance_matrix.cpp

clean:
	rm *.o reorder
	