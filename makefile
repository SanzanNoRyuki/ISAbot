# Compiler
CXX = g++
CXXFLAGS =-std=c++11

# Libraries
LDLIBS =-lssl -lcrypto

# Files
SRC = $(shell echo *.cpp)
INC = $(shell echo *.h)
OBJ = $(patsubst %.cpp,%.o,$(SRC))

.PHONY: all
all: isabot
isabot: $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o isabot $(LDLIBS)

.PHONY: test
test:
	cd test && true