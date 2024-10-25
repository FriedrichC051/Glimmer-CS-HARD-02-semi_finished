TAR = server
OBJ = server_with_threadpool.o threadpool.o
TAR0 = client
CXX = gcc
CXXFLAGS = -c -Wall 

$(TAR): $(OBJ)
	$(CXX) -o $@ $^

%.o: %.c
	$(CXX) $(CXXFLAGS) $< -o $@

.PHONY: clean
clean:
	rm *.o $(TAR) $(TAR0)
