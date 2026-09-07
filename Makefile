CXX      = g++
CXXFLAGS = -std=c++11 -Wall -g
SRCDIR   = src
INCDIR   = include
SRCS     = $(wildcard $(SRCDIR)/*.cpp)
TARGET   = taskforge

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -I$(INCDIR) $(SRCS) -o $(TARGET)

run: all
	./$(TARGET)

memory: all
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run memory clean