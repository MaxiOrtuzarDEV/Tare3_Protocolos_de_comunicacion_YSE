# Makefile para Nodo
CXX = g++
CXXFLAGS = -Wall -std=c++98 -Iinclude
LDFLAGS =

SRCDIR = src
INCDIR = include
BINDIR = bin

SOURCES = $(wildcard $(SRCDIR)/*.cpp)
OBJECTS = $(SOURCES:.cpp=.o)
TARGET = $(BINDIR)/nodo

all: $(TARGET)

$(TARGET): $(SOURCES)
	mkdir -p $(BINDIR)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(BINDIR) *.o
