TAR = main
SRC = $(wildcard *.c)
OBJ = $(patsubst %.c,%.o,$(SRC))
CXX = gcc
CXXFLAGS = -c -Wall

$(TAR): $(OBJ)
	$(CXX) -o $@ $^

%.o: %.c
	$(CXX) $(CXXFLAGS) $< -o $@

.PHONY: clean
clean:
	rm *.o $(TAR)