CC=g++
CFLAGS=-Wall -Wunused -Wextra -g3 -std=c++0x -pedantic
LDFLAGS=
LIBS=-lev -lrt -lmsgpack -lzmq
SOURCES=main.cc common.cc flamethrower.cc params.cc stats.cc tcp_factory.cc tcp_worker.cc payload.cc http_worker.cc
OBJECTS=$(SOURCES:.cc=.o)
EXECUTABLE=flamethrower

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) $(LFLAGS) $(LIBS) -o $@

.cc.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf *.o flamethrower
