CXX = g++
CXXFLAGS = -Wall -Wextra

OPTFLAGS = -O3 -march=native -ffast-math -fopenmp-simd

TARGET = program

SRCS = $(shell find . -name '*.cpp')

OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean