CXXFLAGS=-std=c++11 -O3 -Wall -pedantic -pthread
CXX=g++

all: generator main

generator.o: utils/generator.cpp utils/generator.h
	$(CXX) $(CXXFLAGS) utils/generator.cpp -c -o $@

main.o: main.cpp
	$(CXX) $(CXXFLAGS) main.cpp -c -o $@

splitter.o: splitter.cpp splitter.h
	$(CXX) $(CXXFLAGS) splitter.cpp -c -o $@

counter.o: counter.cpp counter.h
	$(CXX) $(CXXFLAGS) counter.cpp -c -o $@

generator: utils/generator.cpp utils/generator.h utils/generator_main.cpp
	$(CXX) $(CXXFLAGS) utils/generator.cpp utils/generator_main.cpp -o $@

main: main.o splitter.o counter.o
	$(CXX) $(CXXFLAGS) main.o splitter.o counter.o -o $@

.PHONY: clean

clean:
	rm -f generator main generator.o main.o splitter.o
	rm -rf tmp
