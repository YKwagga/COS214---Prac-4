CXX      = g++
CXXFLAGS = -std=c++11 -Wall -Werror
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

ifeq ($(OS),Windows_NT)
clean:
	@if exist $(TARGET) del /Q $(TARGET)
	@if exist $(TARGET).exe del /Q $(TARGET).exe
else
clean:
	rm -f $(TARGET) $(TARGET).exe
endif

.PHONY: all run memory clean