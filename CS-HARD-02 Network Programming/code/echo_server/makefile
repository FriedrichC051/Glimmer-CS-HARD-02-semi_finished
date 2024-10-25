SRC = $(wildcard *.c)
TAR = $(patsubst %.c,%.o,$(SRC))
CXX = gcc

ALL: $(TAR)

% : %.c
	$(CXX) $< -o $@

.PHONY: clean
clean:
	rm $(TAR)