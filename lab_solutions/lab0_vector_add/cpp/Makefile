CXX = g++
CXXFLAGS = -std=c++17 -O2 -g -pthread -Wall

build/main: main.cpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) main.cpp -o build/main

clean:
	rm -rf build
