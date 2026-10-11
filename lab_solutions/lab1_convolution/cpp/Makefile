CXX = g++
CXXFLAGS = -std=c++17 -O2 -g -pthread -Wall

all: build/main_static build/main_dynamic

build/main_static: main.cpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) -DSTATIC_ALLOC main.cpp -o build/main_static

build/main_dynamic: main.cpp
	mkdir -p build
	$(CXX) $(CXXFLAGS) main.cpp -o build/main_dynamic

clean:
	rm -rf build
