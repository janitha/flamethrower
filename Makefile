CC=g++
CFLAGS=-Wall -Wunused -Wextra -g3 -std=c++0x -pedantic
LDFLAGS=
LIBS=-lev -lmsgpack
SOURCES=flamethrower.cc flamethrower_params.cc stream_work.cc tcp_worker.cc tcp_factory.cc
OBJECTS=$(SOURCES:.cc=.o)
EXECUTABLE=flamethrower

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS) $(LFLAGS) $(LIBS) -o $@

.cc.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf *.o flamethrower
