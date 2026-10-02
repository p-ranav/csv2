CXX ?= g++
CXXFLAGS ?= -I../include -O3 -std=c++11
TARGET := main
SRC := main.cpp

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

# Usage: make run FILE=files/vehicles.csv
run: $(TARGET)
	./$(TARGET) $(FILE)

clean:
	rm -f $(TARGET)
