## older makefile
# cacheSim: cacheSim.cpp
# 	g++ -o cacheSim cacheSim.cpp
# 
# .PHONY: clean
# clean:
# 	rm -f *.o
# 	rm -f cacheSim


# 046267 Computer Architecture - HW #2
# Makefile for cache simulator

CXX = g++
CXXFLAGS = -std=c++11 -Wall

TARGET = cacheSim
SRC = cacheSim.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

.PHONY: clean

clean:
	rm -f $(TARGET)