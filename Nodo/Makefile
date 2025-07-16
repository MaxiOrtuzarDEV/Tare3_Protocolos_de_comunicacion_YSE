CXX            ?= g++
CXXFLAGS       := -Wall -Wextra -std=c++0x -Iinclude
LDFLAGS        :=
TARGET         := bin/app

SRCDIR         := src
OBJDIR         := obj
SOURCES        := $(wildcard $(SRCDIR)/*.cpp)
OBJECTS        := $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SOURCES))

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJECTS) | bin
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | obj
	$(CXX) $(CXXFLAGS) -c $< -o $@

bin:
	mkdir -p $@

obj:
	mkdir -p $@

run: $(TARGET)
	@echo "== Ejecutando $(TARGET) =="
	@./$(TARGET)

clean:
	$(RM) -r $(OBJDIR) $(TARGET)
