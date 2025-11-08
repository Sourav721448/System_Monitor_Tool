CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall
LIBS = -lncurses
TARGET = system_monitor_tool
SRC = system_monitor_tool.cpp

all:
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LIBS)

run: all
	sudo ./$(TARGET)

clean:
	rm -f $(TARGET)
	@echo "Cleaned!"
