CXX      = clang++
CXXFLAGS = -std=c++17 -Wall
LIBS     = -lncurses

TARGET   = testbench
SOURCE   = testbench.cpp

all:
	$(CXX) $(CXXFLAGS) $(SOURCE) $(LIBS) -o $(TARGET)

clean:
	rm -f $(TARGET)
