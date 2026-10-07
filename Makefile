CXX ?= g++
CXXFLAGS ?= -std=c++17 -O3
LDFLAGS ?= -static-libgcc -static-libstdc++
INSTANCE ?= .\instancias\eil51.tsp

all: tsp

tsp:
	$(CXX) $(CXXFLAGS) *.cpp $(LDFLAGS) -o tsp.exe

debug:
	$(CXX) -std=c++17 -g -O0 *.cpp -o tsp-debug.exe

run:
	.\tsp.exe $(INSTANCE)

clean:
	del /Q tsp.exe tsp-debug.exe 2>NUL || exit 0