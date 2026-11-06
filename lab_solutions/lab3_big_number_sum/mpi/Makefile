all: build/main build/main_sync

build/main: main.cpp
	mkdir -p build
	mpic++ -std=c++17 -O2 -g -Wall main.cpp -o build/main

# same program with MPI_Ssend instead of MPI_Send, to test that variant 1 has no deadlock
build/main_sync: main.cpp
	mkdir -p build
	mpic++ -std=c++17 -O2 -g -Wall -DSYNC main.cpp -o build/main_sync

clean:
	rm -rf build
