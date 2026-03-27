# Kompilátor a standard
CXX = g++
CXXFLAGS = -Wall -Wextra

# Výkonnostní optimalizace pro SDR a SIMD (AVX/SSE)
OPTFLAGS = -O3 -march=native -ffast-math -fopenmp-simd

# Název výsledného programu (z CMakeLists.txt)
TARGET = program

# Automatické nalezení všech zdrojových souborů .cpp ve složce src a jejích podsložkách
SRCS = $(shell find . -name '*.cpp')

# Vytvoření seznamu objektových souborů (.o) ze zdrojových (.cpp)
OBJS = $(SRCS:.cpp=.o)

# Výchozí pravidlo (co se stane, když napíšete jen "make")
all: $(TARGET)

# Pravidlo pro slinkování výsledného programu
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -o $(TARGET) $(OBJS)

# Pravidlo pro kompilaci jednotlivých .cpp souborů na .o soubory
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(OPTFLAGS) -c $< -o $@

# Pravidlo pro smazání zkompilovaných souborů (vyčistění projektu)
clean:
	rm -f $(OBJS) $(TARGET)

# Deklarace cílů, které nejsou skutečnými soubory
.PHONY: all clean